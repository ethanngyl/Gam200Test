#pragma once
#include <string>
namespace eng::debug {
	class CrashLogger {
	public:
		static void install_handlers();
		static void force_crash_for_test();
	private:
		static void write_report_(const char* title, const char* detail);
		static std::string timestamp_();
		static void terminate_handler_();
		static long __stdcall seh_filter_(struct _EXCEPTION_POINTERS* info);
		static std::string seh_code_to_string_(unsigned long code);
	};
} // namespace eng::debug
