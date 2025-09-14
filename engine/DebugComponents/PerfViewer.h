#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <string>

#include "Trace.h" 

namespace eng::debug {

    // Record the time consumed by each subsystem in the last N frames, calculate the percentage and print it periodically; support exporting to CSV
    class PerfViewer {
    public:
        static void BeginFrame() noexcept;
        static void EndFrame() noexcept;

        // Called by ScopeTimer, unit: seconds
        static void Record(Subsystem sys, double seconds) noexcept;

        // Export the most recent buffered frames to CSV
        static bool ExportCSV(const std::string& path);

        // Optional: Set the print interval (seconds). The default is 1s.
        static void SetPrintInterval(double seconds) noexcept;

    private:
        using clock = std::chrono::steady_clock;
        struct FrameSample {
            std::array<double, (size_t)Subsystem::COUNT> sysSec{}; // Seconds per system
            double frameSec = 0.0;                                 // Full frame seconds
        };

        static void ensureInit_() noexcept;
        static void printIfDue_() noexcept;
        static const char* sysName_(Subsystem s) noexcept;

        // Ring Buffer
        static constexpr int kBuffer = 300; // Approximately 5 seconds @ 60fps
        static FrameSample s_ring_[kBuffer];
        static int s_head_;       // Write subscript
        static bool s_inFrame_;
        static clock::time_point s_frameStart_;
        static clock::time_point s_lastPrint_;
        static double s_printIntervalSec_;
    };

} // namespace eng::debug
