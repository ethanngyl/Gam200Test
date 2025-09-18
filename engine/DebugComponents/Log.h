#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
/*
============================================================================================
  Simple logging API for the engine (header)

  What this gives you:
    • Log levels (Error / Warn / Info / Debug)
    • Multiple “sinks” (where the log lines go): console, file, VS Output
    • Optional source info at the end of a line: "(file:line)"
    • Easy macros so you can write: LOG_INFO("CORE", "Hello %d", 123);

  Quick start (typical flow):
    1) At startup:
         eng::debug::LogConfig cfg;
         cfg.level = eng::debug::LogLevel::Info;
         cfg.filePath = "engine.log";
         cfg.showSourceInfo = false;  // normal logs stay clean
         eng::debug::Log::init(cfg);
    2) Use anywhere:
         LOG_INFO("CORE", "Window created: %dx%d", w, h);
    3) At shutdown:
         eng::debug::Log::shutdown();

  Notes:
    • "tag" is a short category like "CORE", "PERF", "AI".
    • If you pass empty file/line to write()/writef(), no "(file:line)" is appended.
    • Whether "(file:line)" is printed for normal logs is controlled by
      LogConfig::showSourceInfo (we keep it off by default to keep output clean).
============================================================================================
*/
namespace eng::debug {

    // Logging severity. Lower number = more severe.
    enum class LogLevel : uint8_t { Error = 0, Warn, Info, Debug };

    // Global configuration passed to Log::init().
    struct LogConfig {
        LogLevel level = LogLevel::Info;      // Minimum level that will be printed
        std::string filePath = "engine.log";  // Path of the log file (if enabled)
        bool useConsole = true;               // Print to stderr (console)
        bool useFile = true;                  // Append to file at filePath
        bool usePlatformOutput = true;        // Windows: also mirror to OutputDebugString
        bool showSourceInfo = false;          // Append "(file:line)" to normal logs if true
    };

    // A sink is a destination for a formatted log line.
    // The engine provides built-in ConsoleSink and FileSink in Sinks.h/cpp.
    struct ILogSink {
        virtual ~ILogSink() = default;
        // 'msg' is already fully formatted and contains timestamp/level/tag.
        virtual void write(LogLevel lvl, const char* tag, const char* msg) = 0;
    };

    // Central logging facade used via static methods.
    class Log {
    public:
        // Create sinks according to 'cfg' and set initial level. Call once at startup.
        static void init(const LogConfig& cfg);

        // Destroy sinks and flush any pending data. Call once at shutdown.
        static void shutdown();

        // Change / query the current minimum level at runtime.
        static void set_level(LogLevel lvl);
        static LogLevel get_level();

        // Add a custom sink (e.g., network sink). Takes ownership of the pointer.
        static void add_sink(std::unique_ptr<ILogSink> sink);

        // Toggle whether '(file:line)' is appended to normal logs in write()/writef().
        static void set_show_source_info(bool enabled);

        // printf-style log. Example:
        //   Log::writef(LogLevel::Info, "CORE", __FILE__, __LINE__, "Hello %d", 42);
        // If you pass empty file/line, no (file:line) will be printed.
        static void writef(LogLevel lvl, const char* tag,
            const char* file, int line,
            const char* fmt, ...) noexcept;

        // String-style log (already formatted message).
        static void write(LogLevel lvl, const char* tag,
            const char* file, int line,
            const std::string& message) noexcept;

    private:
        // Deliver one fully formatted line to every registered sink.
        static void dispatch_(LogLevel lvl, const char* tag, const char* formatted) noexcept;
    };

// ================================ Convenience macros ================================
// These macros automatically capture __FILE__ and __LINE__ so your log can
// include them when showSourceInfo==true (or you can read them in sinks).
// Use the *0 macros if you already have a std::string and don’t need printf.

#define LOG_ERROR(TAG, FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_WARN(TAG,  FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_INFO(TAG,  FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_DEBUG(TAG, FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)

#define LOG_ERROR0(TAG, MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, MSG)
#define LOG_WARN0(TAG,  MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, MSG)
#define LOG_INFO0(TAG,  MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, MSG)
#define LOG_DEBUG0(TAG, MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, MSG)

} // namespace eng::debug
