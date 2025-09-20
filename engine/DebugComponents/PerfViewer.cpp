#include "PerfViewer.h"
#include "Log.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>

/*
===============================================================================
 PerfViewer.cpp
 ------------------------------------------------------------------------------
 Implementation details for PerfViewer.

 Key data structure: ring buffer
   - s_ring_ holds the last kBuffer frames (FrameSample).
   - s_head_ points to the "current" slot where the active frame is accumulating.
   - After end_frame() we advance s_head_ (circularly).
   - This allows export_csv(...) to dump a recent history window.

 Printing "Perf %"
   - print_if_due_() checks if s_printIntervalSec_ seconds have passed.
   - We print the breakdown for the last completed frame, to avoid partial data.
   - Percent for one subsystem = (sysSec / frameSec) * 100.

 Error handling and safety
   - Functions are noexcept where reasonable to keep perf profiling non-intrusive.
   - begin_frame() is defensive: if a previous frame did not end, it calls
     end_frame() to close it (helps during development).
===============================================================================
*/

namespace eng::debug {

    // Static storage definitions
    PerfViewer::FrameSample PerfViewer::s_ring_[PerfViewer::kBuffer]{};
    int   PerfViewer::s_head_ = 0;
    bool  PerfViewer::s_inFrame_ = false;
    PerfViewer::clock::time_point PerfViewer::s_frameStart_{};
    PerfViewer::clock::time_point PerfViewer::s_lastPrint_{};
    double PerfViewer::s_printIntervalSec_ = 1.0;

    // Set print interval (seconds). Values <= 0 default to 1.0.
    void PerfViewer::set_print_interval(double seconds) noexcept {
        s_printIntervalSec_ = (seconds <= 0.0) ? 1.0 : seconds;
    }

    // Initialize the static state once (lazy init). Safe to call multiple times.
    void PerfViewer::ensure_init_() noexcept {
        static bool inited = false;
        if (!inited) {
            s_head_ = 0;
            s_inFrame_ = false;
            s_lastPrint_ = clock::now();

            // Clear all ring buffer slots
            for (auto& f : s_ring_) {
                f.frameSec = 0.0;
                std::fill(f.sysSec.begin(), f.sysSec.end(), 0.0);
            }
            inited = true;
        }
    }

    // Begin a new frame: clear the current slot and mark start time.
    void PerfViewer::begin_frame() noexcept {
        ensure_init_();

        // If a previous frame did not end (e.g., early return), close it now.
        if (s_inFrame_) end_frame();

        s_inFrame_ = true;
        s_frameStart_ = clock::now();

        // Reset the subsystem accumulators for the current slot.
        auto& f = s_ring_[s_head_];
        f.frameSec = 0.0;
        std::fill(f.sysSec.begin(), f.sysSec.end(), 0.0);
    }

    // End the current frame: store total frame time, maybe print, advance head.
    void PerfViewer::end_frame() noexcept {
        if (!s_inFrame_) return;
        using namespace std::chrono;

        auto& f = s_ring_[s_head_];
        f.frameSec = duration_cast<duration<double>>(clock::now() - s_frameStart_).count();

        // Periodically print the last completed frame's percentages.
        print_if_due_();

        // Move to next slot in the ring (wrap around at kBuffer).
        s_head_ = (s_head_ + 1) % kBuffer;
        s_inFrame_ = false;
    }

    // Accumulate seconds for a given subsystem in the current frame.
    void PerfViewer::record(Subsystem sys, double seconds) noexcept {
        if (!s_inFrame_) return; // ignore if no frame is active
        auto& f = s_ring_[s_head_];
        const auto idx = static_cast<size_t>(sys);
        if (idx < f.sysSec.size()) {
            f.sysSec[idx] += seconds;
        }
    }

    // Convert enum to display name for printing/export.
    const char* PerfViewer::sys_name_(Subsystem s) noexcept {
        switch (s) {
        case Subsystem::Graphics: return "Graphics";
        case Subsystem::Physics:  return "Physics";
        case Subsystem::AI:       return "AI";
        case Subsystem::Audio:    return "Audio";
        case Subsystem::Gameplay: return "Gameplay";
        case Subsystem::IO:       return "IO";
        default:                  return "Other";
        }
    }

    // If enough time has passed, print one compact "Perf %" line for the
    // last completed frame. We avoid printing the current frame because
    // it may not be done yet.
    void PerfViewer::print_if_due_() noexcept {
        using namespace std::chrono;
        const auto now = clock::now();
        const auto due = duration_cast<duration<double>>(now - s_lastPrint_).count() >= s_printIntervalSec_;
        if (!due) return;
        s_lastPrint_ = now;

        // Snapshot the most recently completed frame:
        const int last = (s_head_ - 1 + kBuffer) % kBuffer;
        const auto& f = s_ring_[last];
        if (f.frameSec <= 0.0) return;  // nothing meaningful to print

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "Perf %: ";
        bool first = true;
        for (int i = 0; i < (int)Subsystem::COUNT; ++i) {
            const double sec = f.sysSec[(size_t)i];
            if (sec <= 0.0) continue;

            // Percent of frame time, clamped to [0, 100].
            const double pct = std::clamp(sec / f.frameSec * 100.0, 0.0, 100.0);
            if (!first) oss << " | ";
            first = false;
            oss << sys_name_((Subsystem)i) << " " << pct << "%";
        }
        if (first) {

            // No subsystems were measured in that frame (e.g., profiling disabled).
            oss << "(no subsystems measured)";
        }

        // Send a single clean line to the logging system.
        // We intentionally pass empty file/line so normal logs stay clean.
        Log::write(LogLevel::Info, "PERF", "", 0, oss.str());
    }

    // Export the ring buffer contents to a CSV file.
    // The CSV contains:
    //   frame, frame_ms, Graphics_ms, Physics_ms, ...
    // Only frames with frameSec > 0 are written.
    bool PerfViewer::export_csv(const std::string& path) {
        std::FILE* fp = std::fopen(path.c_str(), "w");
        if (!fp) return false;

        // Header row
        std::fprintf(fp, "frame,frame_ms");
        for (int i = 0; i < (int)Subsystem::COUNT; ++i) {
            std::fprintf(fp, ",%s_ms", sys_name_((Subsystem)i));
        }
        std::fprintf(fp, "\n");

        // Walk the ring starting from the current head (oldest to newest).
        int frameId = 0;
        for (int i = 0; i < kBuffer; ++i) {
            const int idx = (s_head_ + i) % kBuffer;
            const auto& f = s_ring_[idx];
            if (f.frameSec <= 0.0) continue; // Skip empty frames

            std::fprintf(fp, "%d,%.3f", frameId++, f.frameSec * 1000.0);

            for (int j = 0; j < (int)Subsystem::COUNT; ++j) {
                std::fprintf(fp, ",%.3f", f.sysSec[(size_t)j] * 1000.0);
            }
            std::fprintf(fp, "\n");
        }

        std::fclose(fp);

        // Let the user know where we wrote the file.
        Log::writef(LogLevel::Info, "PERF", "", 0, "Exported CSV: %s", path.c_str());
        return true;
    }

} // namespace eng::debug
