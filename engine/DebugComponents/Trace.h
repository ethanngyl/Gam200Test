#pragma once
#include <chrono>
#include <string_view>

namespace eng::debug {

	// can expand as needed: Physics/AI/Audio/Gameplay/IO/Other...
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

	class PerfViewer;

	class ScopeTimer {
	public:
		ScopeTimer(std::string_view name, Subsystem sys) noexcept;
		~ScopeTimer() noexcept;

	private:
		using clock = std::chrono::steady_clock;
		std::string_view m_name;
		Subsystem m_sys;
		clock::time_point m_start;
	};

	// Count the time consumed within a scope and automatically report it to PerfViewer
#define DBG_SCOPE_SYS(NAME, SUBSYS) ::eng::debug::ScopeTimer _dbg_scope_##__LINE__{NAME, SUBSYS}

} // namespace eng::debug
