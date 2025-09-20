#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include "Trace.h" 

/*
===============================================================================
 PerfViewer.h
 ------------------------------------------------------------------------------
 Purpose
   PerfViewer aggregates per-frame time spent in each "Subsystem" (Graphics,
   Physics, etc.) and provides:
     - begin_frame() / end_frame(): mark the frame boundary.
     - record(sys, seconds): add time to a subsystem inside the current frame.
     - set_print_interval(seconds): print percentages once every N seconds.
     - export_csv(path): dump recent frames to a CSV file.

 High-level design
   - While a frame is "open", calls to record(...) add seconds to the current
     frame slot in a ring buffer.
   - end_frame() finalizes that slot by storing the total frame time.
   - At a fixed interval (default 1 second), we print the last completed
     frame's subsystem percentages, e.g. "Graphics 28.4% | Physics 5.2%".
   - A ring buffer stores the last kBuffer frames so we can export them later.

 Usage
   PerfViewer::begin_frame();
   {
       DBG_SCOPE_SYS("Graphics", eng::debug::Subsystem::Graphics);
       // draw calls...
   }
   PerfViewer::end_frame();

   // Optional: export recent data to CSV for analysis
   if (key_pressed_F2) {
       PerfViewer::export_csv("perf_recent.csv");
   }

 Notes
   - begin_frame() should be called at the start of your game loop, and
     end_frame() at the end, once per frame.
   - record(...) is usually called indirectly via ScopeTimer (DBG_SCOPE_SYS).
   - The ring buffer length (kBuffer) defines how many recent frames are kept.
===============================================================================
*/

namespace eng::debug {

    class PerfViewer {
    public:

        // Call at the very start of a frame. This marks a new slot in the ring
        // buffer and clears any previous subsystem accumulators for that slot.
        static void begin_frame() noexcept;

        // Call at the very end of a frame. This computes the full frame
        // duration and finalizes the slot. It may also trigger a periodic print.
        static void end_frame() noexcept;

        // Add 'seconds' to the accumulator for the given subsystem in the
        // current frame slot. Typically called by ScopeTimer's destructor.
        static void record(Subsystem sys, double seconds) noexcept;

        // Dump recent frames from the ring buffer to a CSV file.
        // Returns true on success, false if the file could not be opened.
        static bool export_csv(const std::string& path);

        // Change how often (in seconds) we print percentages to the log.
        // Default is 1.0s. Values <= 0 are clamped to 1.0.
        static void set_print_interval(double seconds) noexcept;

    private:
        using clock = std::chrono::steady_clock;

        // One frame's worth of timing data
        struct FrameSample {

            // Accumulated seconds for each subsystem in this frame
            std::array<double, (size_t)Subsystem::COUNT> sysSec{};

            // Total seconds for the whole frame (end_frame() fills this)
            double frameSec = 0.0;
        };

        // Internal helpers
        static void ensure_init_() noexcept;    // lazy init of static state
        static void print_if_due_() noexcept;   // periodic "Perf %" print
        static const char* sys_name_(Subsystem s) noexcept;

        // Ring buffer storing the last kBuffer frames.
        // Choose a size that is a good tradeoff for your analysis needs.
        static constexpr int kBuffer = 300; // ~5 seconds @ 60 FPS

        // Static state (one global instance)
        static FrameSample      s_ring_[kBuffer];  // circular storage
        static int              s_head_;           // index of the "current" slot
        static bool             s_inFrame_;        // true between begin/end_frame
        static clock::time_point s_frameStart_;    // timestamp at begin_frame
        static clock::time_point s_lastPrint_;     // last time we printed "Perf %"
        static double           s_printIntervalSec_; // seconds between prints
    };

} // namespace eng::debug
