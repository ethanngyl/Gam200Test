#include "Trace.h"
#include "PerfViewer.h"

/*
===============================================================================
 Trace.cpp
 ------------------------------------------------------------------------------
 Implementation of ScopeTimer.

 What happens at runtime
   1) When a ScopeTimer object is constructed (usually by DBG_SCOPE_SYS),
	  we store the current time (steady_clock) and the chosen Subsystem.
   2) When the object leaves scope, the destructor computes the elapsed time
	  in seconds and forwards it to PerfViewer::record(sys, seconds).
   3) PerfViewer aggregates these times per frame and can print system
	  percentages or export CSV.

 Why steady_clock?
   - It is monotonic, meaning it never goes backward if the system time changes.
	 This is ideal for measuring durations.

 Error safety
   - The destructor is noexcept, so even if exceptions happen in the user code,
	 the timer will still record the time on scope exit.
===============================================================================
*/

namespace eng::debug {

	ScopeTimer::ScopeTimer(std::string_view name, Subsystem sys) noexcept
		: m_name(name), m_sys(sys), m_start(clock::now()) {
		// Nothing else to do here. We only capture the start timestamp.
	}

	ScopeTimer::~ScopeTimer() noexcept {
		using namespace std::chrono;

		// Compute seconds elapsed since construction.
		const auto sec = duration_cast<duration<double>>(clock::now() - m_start).count();

		// Report the measured duration to the aggregator.
		// PerfViewer will attribute this time to the given subsystem for the
		// current frame (i.e., between begin_frame() and end_frame()).
		PerfViewer::record(m_sys, sec);
	}

} // namespace eng::debug
