#pragma once
#include <chrono>
#include <string_view>

/*
===============================================================================
 Trace.h
 ------------------------------------------------------------------------------
 Purpose
   Provide a tiny RAII timer (ScopeTimer) to measure how long a block of code
   takes to run. When the timer object goes out of scope (end of the block),
   it automatically reports the duration to PerfViewer, grouped by a "Subsystem"
   (Graphics, Physics, etc.). This makes it easy to see how much time each
   system consumes per frame.

 Key ideas
   - RAII (Resource Acquisition Is Initialization): you construct an object at
	 the start of a scope; its destructor runs at the end of the scope, even
	 if the scope exits via return/exception. We use that to measure time.
   - Minimal usage cost: one macro DBG_SCOPE_SYS(...) creates a ScopeTimer on
	 the stack. No manual "start/stop" calls are needed.

 How to use
   void render_frame() {
	 DBG_SCOPE_SYS("Graphics", eng::debug::Subsystem::Graphics);
	 // your rendering code...
   }

   In each frame:
	 - Call PerfViewer::begin_frame() before doing any work you want to measure.
	 - Use DBG_SCOPE_SYS for each system block you want to profile.
	 - Call PerfViewer::end_frame() after the frame finishes.

 Notes
   - The "name" parameter is currently kept for clarity but not used by the
	 aggregator; the Subsystem enum is what groups the times.
   - Overhead is very low: one timestamp at construction and one at destruction,
	 plus an atomic-like accumulation inside PerfViewer.
===============================================================================
*/

namespace eng::debug {

	// Subsystem
	// -------------------------------------------------------------------------
	// This enum lists the major systems you might want to profile individually.
	// You can expand it over time (AI, Audio, etc.). COUNT is kept as the last
	// entry so arrays can use it for their size.
	enum class Subsystem : unsigned char {
		Graphics = 0,
		Physics,
		AI,
		Audio,
		Gameplay,
		IO,
		Other,
		COUNT
	};

	// Forward declaration to avoid a header dependency cycle.
	class PerfViewer;

	// ScopeTimer
	// -------------------------------------------------------------------------
	// RAII timer that records the time spent between construction and
	// destruction, and then reports it to PerfViewer::record(sys, seconds).
	//
	// Usage:
	//   {
	//     DBG_SCOPE_SYS("Physics step", eng::debug::Subsystem::Physics);
	//     // physics code...
	//   } // ~ScopeTimer() is called here, time is reported automatically.
	class ScopeTimer {
	public:
		// name: a short label describing the work (used for clarity).
		// sys : which subsystem to attribute this time to.
		ScopeTimer(std::string_view name, Subsystem sys) noexcept;

		// On scope exit, compute the elapsed time and report to PerfViewer.
		~ScopeTimer() noexcept;

	private:
		using clock = std::chrono::steady_clock; // monotonic clock

		std::string_view m_name;     // label (not currently used by aggregator)
		Subsystem        m_sys;      // which subsystem this scope belongs to
		clock::time_point m_start;   // timestamp captured at construction
	};

	// Helper macro
	// -------------------------------------------------------------------------
	// Creates a unique ScopeTimer object on the stack for the current block.
	// NAME  : human-readable label (const char* or std::string_view)
	// SUBSYS: one of the values from Subsystem (e.g., Subsystem::Graphics)
	//
	// The trick with __LINE__ makes the variable name unique per line, so you
	// can place multiple DBG_SCOPE_SYS(...) in the same function.
	//
	// Example:
	//   void update() {
	//     DBG_SCOPE_SYS("Gameplay", eng::debug::Subsystem::Gameplay);
	//     // gameplay code...
	//   }
#define DBG_SCOPE_SYS(NAME, SUBSYS) ::eng::debug::ScopeTimer _dbg_scope_##__LINE__{NAME, SUBSYS}

} // namespace eng::debug
