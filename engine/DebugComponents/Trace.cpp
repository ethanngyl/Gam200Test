#include "Trace.h"
#include "PerfViewer.h"

namespace eng::debug {

	ScopeTimer::ScopeTimer(std::string_view name, Subsystem sys) noexcept
		: m_name(name), m_sys(sys), m_start(clock::now()) {
	}

	ScopeTimer::~ScopeTimer() noexcept {
		using namespace std::chrono;
		const auto sec = duration_cast<duration<double>>(clock::now() - m_start).count();
		PerfViewer::record(m_sys, sec);
	}

} // namespace eng::debug
