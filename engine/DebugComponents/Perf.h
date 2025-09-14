#pragma once
#include <chrono>
#include <functional>

namespace eng::debug {

	class FpsCounter {
	public:
		using clock = std::chrono::steady_clock;
		using TitleUpdater = std::function<void(const char* title)>;

		// Optional: Register a function to update the window title; if not registered, the title will not be updated.
		void SetTitleUpdater(TitleUpdater updater) { m_titleUpdater = std::move(updater); }

		// Called at the beginning/end of each frame
		void BeginFrame();   // Record frame start time
		void EndFrame();     // Statistics and log output every 1 second/update title

		// Optional: When you already have dt (in seconds) you can feed it directly to the counter (instead of Begin/End)
		void TickWithDt(double dtSeconds);

		// The latest statistical results
		double LastFps() const { return m_lastFps; }       // Average FPS in the last second
		double LastAvgMs() const { return m_lastAvgMs; }   // Average frame time in the last second (ms)
		double SmoothedFps() const { return m_smoothFps; } // Smoothed FPS (Exponential Moving Average)

		// Adjust the smoothing intensity (0~1, the larger the smoothing, the smoother the hand, the default is 0.2)
		void SetSmoothingAlpha(double a) { m_alpha = (a < 0) ? 0 : (a > 1 ? 1 : a); }

	private:
		// Outputs an FPS log and (if registered) updates the window title
		void emitOutput();

		// Feed the smooth FPS with a frame duration (in seconds)
		void smoothUpdate(double dtSeconds);

		// Timing
		clock::time_point m_secStart{};
		clock::time_point m_frameStart{};
		bool   m_inited = false;

		// count
		int    m_frameCount = 0;

		// Statistics for the last second
		double m_lastFps = 0.0;
		double m_lastAvgMs = 0.0;

		// smooth
		double m_alpha = 0.20;   // EMA coefficient
		double m_smoothFps = 0.0;

		TitleUpdater m_titleUpdater; // Optional Title Updater
	};

} // namespace eng::debug
