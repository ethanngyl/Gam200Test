#pragma once
// ----------------------------------------------------------------------------
// Logging system (minimal)
// Thread-safe multi-level logger with console + file sinks.
// ----------------------------------------------------------------------------
#pragma once
#include <string>

namespace Engine {

	namespace Debug{


		enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal };

		// ----------------------------------------------------------------------------
		// [InitLogger]
		// [Opens file sink and prepares internal state. Call once at program start.]
		// ----------------------------------------------------------------------------
		void InitLogger(const char* filePath = "log.txt");

		// ----------------------------------------------------------------------------
		// [ShutdownLogger]
		// [Flushes and closes sinks. Call once before exit.]
		// ----------------------------------------------------------------------------
		void ShutdownLogger();

		// ----------------------------------------------------------------------------
		// [SetMinLevel]
		// [Sets minimum level to output; messages below are ignored.]
		// ----------------------------------------------------------------------------
		void SetMinLevel(LogLevel lvl);

		// ----------------------------------------------------------------------------
		// [Log]
		// [Formats and writes one log line to all sinks (thread-safe).]
		// ----------------------------------------------------------------------------
		void Log(LogLevel lvl, const char* category, const char* fmt, ...);

		// Convenience macros
		#define LOGT(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Trace, cat, fmt, ##__VA_ARGS__)
		#define LOGD(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Debug, cat, fmt, ##__VA_ARGS__)
		#define LOGI(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Info,  cat, fmt, ##__VA_ARGS__)
		#define LOGW(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Warn,  cat, fmt, ##__VA_ARGS__)
		#define LOGE(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Error, cat, fmt, ##__VA_ARGS__)
		#define LOGF(cat, fmt, ...) ::Engine::Debug::Log(::Engine::Debug::LogLevel::Fatal, cat, fmt, ##__VA_ARGS__)

	} // namespace Debug
}// namespace Engine
