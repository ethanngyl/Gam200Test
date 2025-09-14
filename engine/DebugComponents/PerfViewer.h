#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include "Trace.h" 

namespace eng::debug {

    // Record the time consumption of each subsystem in the last N frames, calculate the proportion and print it periodically; support export to CSV
    class PerfViewer {
    public:
        static void begin_frame() noexcept;
        static void end_frame() noexcept;

        // Called by ScopeTimer, unit: seconds
        static void record(Subsystem sys, double seconds) noexcept;

        // Export the most recent buffered frames to CSV
        static bool export_csv(const std::string& path);

        // Optional: Set the print interval (seconds). The default is 1s.
        static void set_print_interval(double seconds) noexcept;

    private:
        using clock = std::chrono::steady_clock;
        struct FrameSample {
            std::array<double, (size_t)Subsystem::COUNT> sysSec{};  // Seconds per system
            double frameSec = 0.0;                                  //Full frame seconds
        };

        static void ensure_init_() noexcept;
        static void print_if_due_() noexcept;
        static const char* sys_name_(Subsystem s) noexcept;

        // Ring Buffer
        static constexpr int kBuffer = 300; // Approximately 5 seconds @ 60fps
        static FrameSample s_ring_[kBuffer];
        static int s_head_;
        static bool s_inFrame_;
        static clock::time_point s_frameStart_;
        static clock::time_point s_lastPrint_;
        static double s_printIntervalSec_;
    };

} // namespace eng::debug
