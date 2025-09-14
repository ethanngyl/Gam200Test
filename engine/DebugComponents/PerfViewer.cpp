#include "PerfViewer.h"
#include "Log.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace eng::debug {

    PerfViewer::FrameSample PerfViewer::s_ring_[PerfViewer::kBuffer]{};
    int   PerfViewer::s_head_ = 0;
    bool  PerfViewer::s_inFrame_ = false;
    PerfViewer::clock::time_point PerfViewer::s_frameStart_{};
    PerfViewer::clock::time_point PerfViewer::s_lastPrint_{};
    double PerfViewer::s_printIntervalSec_ = 1.0;

    void PerfViewer::set_print_interval(double seconds) noexcept {
        s_printIntervalSec_ = (seconds <= 0.0) ? 1.0 : seconds;
    }

    void PerfViewer::ensure_init_() noexcept {
        static bool inited = false;
        if (!inited) {
            s_head_ = 0;
            s_inFrame_ = false;
            s_lastPrint_ = clock::now();
            for (auto& f : s_ring_) {
                f.frameSec = 0.0;
                std::fill(f.sysSec.begin(), f.sysSec.end(), 0.0);
            }
            inited = true;
        }
    }

    void PerfViewer::begin_frame() noexcept {
        ensure_init_();
        if (s_inFrame_) end_frame(); // Fault tolerance: End the frame even if the previous frame did not end normally

        s_inFrame_ = true;
        s_frameStart_ = clock::now();

        // Clear the current frame slot
        auto& f = s_ring_[s_head_];
        f.frameSec = 0.0;
        std::fill(f.sysSec.begin(), f.sysSec.end(), 0.0);
    }

    void PerfViewer::end_frame() noexcept {
        if (!s_inFrame_) return;
        using namespace std::chrono;

        auto& f = s_ring_[s_head_];
        f.frameSec = duration_cast<duration<double>>(clock::now() - s_frameStart_).count();

        // Print (if time is up)
        print_if_due_();

        // Circular Advance
        s_head_ = (s_head_ + 1) % kBuffer;
        s_inFrame_ = false;
    }

    void PerfViewer::record(Subsystem sys, double seconds) noexcept {
        if (!s_inFrame_) return;
        auto& f = s_ring_[s_head_];
        const auto idx = static_cast<size_t>(sys);
        if (idx < f.sysSec.size()) {
            f.sysSec[idx] += seconds;
        }
    }

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

    void PerfViewer::print_if_due_() noexcept {
        using namespace std::chrono;
        const auto now = clock::now();
        const auto due = duration_cast<duration<double>>(now - s_lastPrint_).count() >= s_printIntervalSec_;
        if (!due) return;
        s_lastPrint_ = now;

        // Take the last completed frame as a snapshot
        const int last = (s_head_ - 1 + kBuffer) % kBuffer;
        const auto& f = s_ring_[last];
        if (f.frameSec <= 0.0) return;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "Perf %: ";
        bool first = true;
        for (int i = 0; i < (int)Subsystem::COUNT; ++i) {
            const double sec = f.sysSec[(size_t)i];
            if (sec <= 0.0) continue;
            const double pct = std::clamp(sec / f.frameSec * 100.0, 0.0, 100.0);
            if (!first) oss << " | ";
            first = false;
            oss << sys_name_((Subsystem)i) << " " << pct << "%";
        }
        if (first) { // No subsystem records
            oss << "(no subsystems measured)";
        }

        // Output message
        Log::write(LogLevel::Info, "PERF", "", 0, oss.str());
    }

    bool PerfViewer::export_csv(const std::string& path) {
        std::FILE* fp = std::fopen(path.c_str(), "w");
        if (!fp) return false;

        std::fprintf(fp, "frame,frame_ms");
        for (int i = 0; i < (int)Subsystem::COUNT; ++i) {
            std::fprintf(fp, ",%s_ms", sys_name_((Subsystem)i));
        }
        std::fprintf(fp, "\n");

        // Output the kBuffer frame in the ring buffer (starting from the next bit after the current header)
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
        Log::writef(LogLevel::Info, "PERF", "", 0, "Exported CSV: %s", path.c_str());
        return true;
    }

} // namespace eng::debug
