#include "Perf.h"
#include "Log.h"
#include <chrono>
#include <cstdio>
#include <string>

namespace eng::debug {

    using namespace std::chrono;

    void FpsCounter::BeginFrame() {
        auto now = clock::now();
        if (!m_inited) {
            m_inited = true;
            m_secStart = now;
        }
        m_frameStart = now;
    }

    void FpsCounter::EndFrame() {
        if (!m_inited) return;
        m_frameCount++;

        // This frame dt (seconds), used for smoothing
        const double dtSec = duration_cast<duration<double>>(clock::now() - m_frameStart).count();
        smoothUpdate(dtSec);

        const auto now = clock::now();
        const auto secElapsedMs = duration_cast<milliseconds>(now - m_secStart).count();

        if (secElapsedMs >= 1000) {
            // The average of the last second
            m_lastAvgMs = static_cast<double>(secElapsedMs) / m_frameCount;
            m_lastFps = (m_lastAvgMs > 0.0) ? 1000.0 / m_lastAvgMs : 0.0;

            emitOutput();

            // Next second
            m_secStart = now;
            m_frameCount = 0;
        }
    }

    void FpsCounter::TickWithDt(double dtSeconds) {
        if (!m_inited) {
            m_inited = true;
            m_secStart = clock::now();
        }
        m_frameCount++;
        smoothUpdate(dtSeconds);

        const auto now = clock::now();
        const auto secElapsedMs =
            duration_cast<milliseconds>(now - m_secStart).count();

        if (secElapsedMs >= 1000) {
            m_lastAvgMs = static_cast<double>(secElapsedMs) / m_frameCount;
            m_lastFps = (m_lastAvgMs > 0.0) ? 1000.0 / m_lastAvgMs : 0.0;
            emitOutput();
            m_secStart = now;
            m_frameCount = 0;
        }
    }

    void FpsCounter::emitOutput() {
        // log
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "FPS(avg): %.1f, frame(avg): %.2f ms, FPS(smooth): %.1f",
            m_lastFps, m_lastAvgMs, m_smoothFps);

        Log::Write(LogLevel::Info, "PERF", "", 0, buf);


        // Optional: Update the window title
        if (m_titleUpdater) {
            char title[128];
            std::snprintf(title, sizeof(title),
                "StructSquad - FPS: %.1f (avg) | %.1f (smooth)",
                m_lastFps, m_smoothFps);
            m_titleUpdater(title);
        }
    }

    void FpsCounter::smoothUpdate(double dtSeconds) {
        if (dtSeconds <= 0.0) return;
        const double instFps = 1.0 / dtSeconds;
        if (m_smoothFps <= 0.0) {
            m_smoothFps = instFps;
        }
        else {
            m_smoothFps = m_alpha * instFps + (1.0 - m_alpha) * m_smoothFps;
        }
    }

} // namespace eng::debug
