#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace eng::debug {

    enum class LogLevel : uint8_t { Error = 0, Warn, Info, Debug };

    struct LogConfig {
        LogLevel level = LogLevel::Info;      // Default threshold
        std::string filePath = "engine.log";  // File log path
        bool useConsole = true;               // stderr
        bool useFile = true;                  // Append to file
        bool usePlatformOutput = true;        // Windows: OutputDebugString
        bool showSourceInfo = false;          // Whether to display (file:line) at the end of normal logs
    };

    // Abstract output
    struct ILogSink {
        virtual ~ILogSink() = default;
        virtual void write(LogLevel lvl, const char* tag, const char* msg) = 0;
    };

    class Log {
    public:
        static void init(const LogConfig& cfg);
        static void shutdown();

        static void set_level(LogLevel lvl);
        static LogLevel get_level();

        static void add_sink(std::unique_ptr<ILogSink> sink);

        static void set_show_source_info(bool enabled);

        static void writef(LogLevel lvl, const char* tag,
            const char* file, int line,
            const char* fmt, ...) noexcept;

        static void write(LogLevel lvl, const char* tag,
            const char* file, int line,
            const std::string& message) noexcept;

    private:
        static void dispatch_(LogLevel lvl, const char* tag, const char* formatted) noexcept;
    };

#define LOG_ERROR(TAG, FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_WARN(TAG,  FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_INFO(TAG,  FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
#define LOG_DEBUG(TAG, FMT, ...) ::eng::debug::Log::writef(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)

#define LOG_ERROR0(TAG, MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, MSG)
#define LOG_WARN0(TAG,  MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, MSG)
#define LOG_INFO0(TAG,  MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, MSG)
#define LOG_DEBUG0(TAG, MSG) ::eng::debug::Log::write(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, MSG)

} // namespace eng::debug
