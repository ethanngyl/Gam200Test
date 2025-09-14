#pragma once
#include <chrono>
#include <functional>

namespace eng::debug {

	class FpsCounter {
	public:
		using clock = std::chrono::steady_clock;
		using TitleUpdater = std::function<void(const char* title)>;

		void set_title_updater(TitleUpdater updater) { m_titleUpdater = std::move(updater); }

		void begin_frame();
		void end_frame();
		void tick_with_dt(double dtSeconds);

		double last_fps() const { return m_lastFps; }
		double last_avg_ms() const { return m_lastAvgMs; }
		double smoothed_fps() const { return m_smoothFps; }

		void set_smoothing_alpha(double a) { m_alpha = (a < 0) ? 0 : (a > 1 ? 1 : a); }

		// Optional: Turn off/on FPS log output (does not affect title updates)
		void set_enable_logging(bool on) { m_logEnabled = on; }

	private:
		void emit_output_();
		void smooth_update_(double dtSeconds);

		clock::time_point m_secStart{};
		clock::time_point m_frameStart{};
		bool   m_inited = false;

		int    m_frameCount = 0;

		double m_lastFps = 0.0;
		double m_lastAvgMs = 0.0;

		double m_alpha = 0.20;   // EMA coefficient
		double m_smoothFps = 0.0;

		bool  m_logEnabled = true;
		TitleUpdater m_titleUpdater; // Optional title updater
	};

} // namespace eng::debug
