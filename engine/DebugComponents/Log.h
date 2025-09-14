#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace eng::debug {

    enum class LogLevel : uint8_t { Error = 0, Warn, Info, Debug };

    struct LogConfig {
        LogLevel level = LogLevel::Info;      
        std::string filePath = "engine.log"; 
        bool useConsole = true;               
        bool useFile = true;                 
        bool usePlatformOutput = true;       
        bool showSourceInfo = false;
    };

    struct ILogSink {
        virtual ~ILogSink() = default;
        virtual void write(LogLevel lvl, const char* tag, const char* msg) = 0;
    };

    class Log {
    public:
        static void Init(const LogConfig& cfg);
        static void Shutdown();

        static void SetLevel(LogLevel lvl);
        static LogLevel GetLevel();

        static void AddSink(std::unique_ptr<ILogSink> sink);

        static void SetShowSourceInfo(bool enabled);

        static void Writef(LogLevel lvl, const char* tag,
            const char* file, int line,
            const char* fmt, ...) noexcept;

        static void Write(LogLevel lvl, const char* tag,
            const char* file, int line,
            const std::string& message) noexcept;

    private:
        static void dispatch(LogLevel lvl, const char* tag, const char* formatted) noexcept;
    };

    #define LOG_ERROR(TAG, FMT, ...) ::eng::debug::Log::Writef(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
    #define LOG_WARN(TAG,  FMT, ...) ::eng::debug::Log::Writef(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
    #define LOG_INFO(TAG,  FMT, ...) ::eng::debug::Log::Writef(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)
    #define LOG_DEBUG(TAG, FMT, ...) ::eng::debug::Log::Writef(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, FMT, __VA_ARGS__)

    #define LOG_ERROR0(TAG, MSG) ::eng::debug::Log::Write(::eng::debug::LogLevel::Error, TAG, __FILE__, __LINE__, MSG)
    #define LOG_WARN0(TAG,  MSG) ::eng::debug::Log::Write(::eng::debug::LogLevel::Warn,  TAG, __FILE__, __LINE__, MSG)
    #define LOG_INFO0(TAG,  MSG) ::eng::debug::Log::Write(::eng::debug::LogLevel::Info,  TAG, __FILE__, __LINE__, MSG)
    #define LOG_DEBUG0(TAG, MSG) ::eng::debug::Log::Write(::eng::debug::LogLevel::Debug, TAG, __FILE__, __LINE__, MSG)

} // namespace eng::debug