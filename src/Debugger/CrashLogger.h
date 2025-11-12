/*
===============================================================================
 File:          CrashLogger.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-09-30
 Contribution:  100%
 ------------------------------------------------------------------------------
 Purpose
   Provides a global crash logging facility for Windows (and partially portable).
   It installs handlers for:
	 - std::terminate (C++ unhandled exceptions)
	 - SEH (Structured Exception Handling, e.g. access violations on Windows)

   When a crash occurs, it writes a text report to:
	   crash_YYYYMMDD_HHMMSS.txt
   in the same directory as the executable, and also mirrors a concise line
   into the log system with tag "CRASH".

   Typical usage:
	 // At startup, after Log::init():
	 eng::debug::CrashLogger::install_handlers();

	 // Optional: during runtime, you can trigger a crash for testing:
	 eng::debug::CrashLogger::force_crash_for_test();

 Key features:
   - Reports crash reason and code (if SEH).
   - Captures a call stack (with file:line info if PDB symbols are available).
   - Guarantees a file is written before process termination.
   - Configurable through game_config.txt using ConfigReader.
===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace eng::debug {
	class CrashLogger {
	public:
		// Configuration structure for crash logger (For Defensive Programming)
		struct Config {
			bool enabled = true;
			std::string reportPrefix = "crash_";
			std::string reportExtension = ".txt";
			std::string reportDirectory = ".";
			int maxStackFrames = 64;
			int skipFrames = 0;
			int symbolBufferSize = 256;
			int maxSymbolNameLength = 255;
			std::string separatorLine = "--------------------------------------------------";
		};

		// Install the terminate handler and SEH filter (Windows only).
		// Automatically loads configuration from ConfigReader if available.
		static void install_handlers();

		// Load configuration from ConfigReader
		// Call this before install_handlers() to customize settings
		static void load_config();

		// Get current configuration
		static const Config& get_config() { return config_; }

		// Force a crash for testing (currently writes through a null pointer).
		static void force_crash_for_test();
	private:
		// Configuration instance
		static Config config_;

		// Internal helper: write a crash report file and mirror a log line.
		static void write_report_(const char* title, const char* detail);

		// Build a timestamp string: "YYYYMMDD_HHMMSS".
		static std::string timestamp_();

		// C++ terminate handler. Called when std::terminate() is invoked due to
		// unhandled exceptions or failed noexcept.
		static void terminate_handler_();

		// Windows SEH filter: catches access violations, divide-by-zero, etc.
		static long __stdcall seh_filter_(struct _EXCEPTION_POINTERS* info);

		// Translate common SEH exception codes to a short string.
		static std::string seh_code_to_string_(unsigned long code);
	};
} // namespace eng::debug