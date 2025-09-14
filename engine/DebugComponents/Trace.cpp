#include "Trace.h"
#include "PerfViewer.h"
#include <chrono>

namespace eng::debug {

	ScopeTimer::ScopeTimer(std::string_view name, Subsystem sys) noexcept
		: m_name(name), m_sys(sys), m_start(clock::now()) {
	}

	ScopeTimer::~ScopeTimer() noexcept {
		using namespace std::chrono;
		const double sec = duration_cast<duration<double>>(clock::now() - m_start).count();
		PerfViewer::Record(m_sys, sec);
	}

} // namespace eng::debug
