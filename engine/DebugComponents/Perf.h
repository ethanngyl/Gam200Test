#pragma once
#include <chrono>
#include <functional>

/*
===============================================================================
 Perf.h
 ------------------------------------------------------------------------------
 Purpose
   FpsCounter measures frame timing during runtime. It can:
	 - Measure FPS in a "pull" style using begin_frame() / end_frame().
	 - Or, if you already know the delta time per frame, you can call
	   tick_with_dt(dtSeconds).
	 - Emit an optional log line once per second.
	 - Update the window title using a user-supplied callback.

 How it works (high level)
   - With begin_frame() we note the time at the start of the frame.
   - With end_frame() we:
	   1) Compute the frame duration dt (seconds).
	   2) Update a smoothed FPS using exponential moving average (EMA).
	   3) Once per second, compute:
			* average frame time for the last second, in milliseconds
			* average FPS for the last second (= 1000 / avg_ms)
		  Then we log one compact line and optionally update the window title.
   - If you already have dtSeconds from your game loop, use tick_with_dt() instead.
   - "Smoothed FPS" is for display only (less jumpy than instantaneous FPS).

 Notes
   - Logging of FPS lines can be turned on/off with set_enable_logging().
   - We keep all timestamps in steady_clock to avoid sudden jumps from system time.
   - This class only owns counters; it does not own any window or renderer.
===============================================================================
*/

namespace eng::debug {

	class FpsCounter {
	public:
		// A monotonic clock that is not affected by system time changes.
		using clock = std::chrono::steady_clock;

		// Callback used to update the window title with a preformatted string.
		// Can supply something like:
		// fps.set_title_updater([window](const char* t) { glfwSetWindowTitle(window, t); });
		using TitleUpdater = std::function<void(const char* title)>;

		// Provide a title updater callback (optional).
		void set_title_updater(TitleUpdater updater) { m_titleUpdater = std::move(updater); }

		// Measurement API when you do not have dt yourself:
		// Call at the top of each frame.
		void begin_frame();

		// Call at the end of each frame.
		void end_frame();

		// Alternative API if engine already knows dt (in seconds).
		void tick_with_dt(double dtSeconds);

		// Read-only metrics (last published once-per-second values).
		double last_fps() const { return m_lastFps; }			// average FPS over the last second
		double last_avg_ms() const { return m_lastAvgMs; }		// average frame time (ms) over last second
		double smoothed_fps() const { return m_smoothFps; }		// EMA-smoothed instantaneous FPS

		// Smoothing factor for EMA. Range clamped to [0,1].
		// Higher alpha -> reacts faster to changes (but more jumpy).
		void set_smoothing_alpha(double a) { m_alpha = (a < 0) ? 0 : (a > 1 ? 1 : a); }

		// Optional: Turn off/on FPS log output (does not affect title updates)
		void set_enable_logging(bool on) { m_logEnabled = on; }

	private:

		// Build and emit the once-per-second output:
		//   - one log line (if enabled)
		//   - optional window title update
		void emit_output_();

		// Update the smoothed FPS based on this frame's dt (seconds).
		void smooth_update_(double dtSeconds);

		// Time anchors
		clock::time_point m_secStart{};   // start of the current 1-second window
		clock::time_point m_frameStart{}; // start time of the current frame
		bool   m_inited = false;          // set true after the first call

		// Counting within current 1-second window
		int    m_frameCount = 0;

		// Metrics published once per second
		double m_lastFps = 0.0;			  // average FPS over last second
		double m_lastAvgMs = 0.0;         // average ms over last second

		// Smoothed FPS using exponential moving average of instantaneous FPS
		double m_alpha = 0.20;   // EMA coefficient
		double m_smoothFps = 0.0;

		// Output settings
		bool  m_logEnabled = true;	 // whether to print the FPS line once per second
		TitleUpdater m_titleUpdater; // Optional title updater
	};

} // namespace eng::debug
