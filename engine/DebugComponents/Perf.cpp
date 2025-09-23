#include "Perf.h"
#include "Log.h"
#include <chrono>
#include <cstdio>

/*
===============================================================================
 Perf.cpp
 ------------------------------------------------------------------------------
 Implementation details for FpsCounter.

 Key ideas
   - We measure time using steady_clock which is monotonic (not affected by
     system clock adjustments).
   - The "one-second window" logic:
       * We count frames and accumulate time since m_secStart.
       * When >= 1000 ms have passed, we compute averages and emit output.
   - The smoothed FPS is an exponential moving average (EMA) of instantaneous
     FPS (1 / dt). This makes the displayed number less jumpy.
===============================================================================
*/

namespace eng::debug {

    using namespace std::chrono;

    void FpsCounter::begin_frame() {

        // On the first call, initialize the one-second window anchor.
        auto now = clock::now();
        if (!m_inited) {
            m_inited = true;
            m_secStart = now;   // start counting the current 1 - second window
        }

        // Remember when this frame started so end_frame() can compute dt.
        m_frameStart = now;
    }

    void FpsCounter::end_frame() {
        if (!m_inited) return;

        // Count one more frame in the current one-second window.
        m_frameCount++;

        // Compute this frame's duration (seconds) and update smoothed FPS.
        const double dtSec = duration_cast<duration<double>>(clock::now() - m_frameStart).count();
        smooth_update_(dtSec);

        // Check if one second has elapsed since m_secStart.
        const auto now = clock::now();
        const auto secElapsedMs = duration_cast<milliseconds>(now - m_secStart).count();

        // Once per second, compute averages and emit.
        if (secElapsedMs >= 1000) {

            // Average ms per frame during the last second.
            m_lastAvgMs = static_cast<double>(secElapsedMs) / m_frameCount;

            // Average FPS for the last second (guard against divide-by-zero).
            m_lastFps = (m_lastAvgMs > 0.0) ? 1000.0 / m_lastAvgMs : 0.0;

            // Emit one compact output line and optional title update.
            emit_output_();

            // Reset the one-second window for the next round.
            m_secStart = now;
            m_frameCount = 0;
        }
    }

    void FpsCounter::tick_with_dt(double dtSeconds) {

        // If the engine provides dt directly, we do not need begin/end frame.
        if (!m_inited) {
            m_inited = true;
            m_secStart = clock::now();
        }

        // Update smoothed FPS with the provided dt and count a frame.
        m_frameCount++;
        smooth_update_(dtSeconds);

        // Same once-per-second logic as in end_frame().
        const auto now = clock::now();
        const auto secElapsedMs = duration_cast<milliseconds>(now - m_secStart).count();

        if (secElapsedMs >= 1000) {
            m_lastAvgMs = static_cast<double>(secElapsedMs) / m_frameCount;
            m_lastFps = (m_lastAvgMs > 0.0) ? 1000.0 / m_lastAvgMs : 0.0;
            emit_output_();
            m_secStart = now;
            m_frameCount = 0;
        }
    }

    void FpsCounter::emit_output_() {
        // 1) Log one line per second (optional). We intentionally pass empty
        //    file/line so normal logs stay clean and do not show "(file:line)".
        if (m_logEnabled) {
            Log::writef(LogLevel::Info, "PERF", __FILE__, __LINE__,
                "FPS(avg): %.1f, frame(avg): %.2f ms, FPS(smooth): %.1f",
                m_lastFps, m_lastAvgMs, m_smoothFps);
        }

        // 2) Update the window title if the user provided a callback.
        if (m_titleUpdater) {
            char title[128];
            std::snprintf(title, sizeof(title),
                "StructSquad - FPS: %.1f (avg) | %.1f (smooth)",
                m_lastFps, m_smoothFps);
            m_titleUpdater(title);
        }
    }

    void FpsCounter::smooth_update_(double dtSeconds) {

        // Ignore invalid or zero durations.
        if (dtSeconds <= 0.0) return;

        // Instantaneous FPS for this frame.
        const double instFps = 1.0 / dtSeconds;
        if (m_smoothFps <= 0.0) {
            m_smoothFps = instFps;
        }
        else {

            // EMA formula: s = a * x + (1 - a) * s
            // where:
            //   s = smoothed value, x = current sample, a = alpha in [0,1]
            m_smoothFps = m_alpha * instFps + (1.0 - m_alpha) * m_smoothFps;
        }
    }

} // namespace eng::debug
