#include "Log.h"
#include "Sinks.h"

#include <chrono>
#include <cstdarg>
#include <mutex>
#include <sstream>
#include <iomanip>
//
// ============================================================================================
//  Simple logging API for the engine (implementation)
//
//  Design notes (plain English):
//    • We keep a single global LogState (level + list of sinks).
//    • All public methods are static and thread-safe where needed.
//    • We format the final line once (with timestamp/level/tag/message)
//      then broadcast it to each sink via dispatch_().
//    • The "(file:line)" appendix is controlled by state().showSource.
//
//  Thread-safety:
//    • A single mutex protects the sink list and dispatch.
//    • If you need extremely high throughput, you can later swap the sinks
//      for an async queue without changing the Log API.
// ============================================================================================
//
namespace eng::debug {

    namespace {

        // Global logging state. This lives for the process lifetime.
        struct LogState {
            LogLevel level = LogLevel::Info;                // minimum level that gets printed
            std::vector<std::unique_ptr<ILogSink>> sinks;   // all active destinations
            std::mutex mtx;                                 // protects 'sinks' and dispatch
            bool showSource = false;                        // append "(file:line)" to lines if true
        };

        // Singleton accessor (initialized on first use).
        static LogState& state() { static LogState S; return S; }

        // Return "HH:MM:SS.mmm" (local time). We avoid i/o heavy calls here.
        static std::string time_now_str() {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto t = system_clock::to_time_t(now);
            auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
            std::tm tm{};
        #if defined(_WIN32)
            localtime_s(&tm, &t);   // Windows thread-safe variant
        #else   
            localtime_r(&t, &tm);   // POSIX thread-safe variant
        #endif
            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(2) << tm.tm_hour << ":"
                << std::setw(2) << tm.tm_min << ":"
                << std::setw(2) << tm.tm_sec << "."
                << std::setw(3) << ms;
            return oss.str();
        }

        // Convert enum to short uppercase string.
        static std::string level_to_cstr(LogLevel l) {
            switch (l) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warn: return "WARN";
            case LogLevel::Info: return "INFO";
            default:              return "DEBUG";
            }
        }
    } // namespace

    // Initialize sinks and state from a config.
    void Log::init(const LogConfig& cfg) {
        auto& S = state();
        std::scoped_lock lk(S.mtx);

        S.level = cfg.level;
        S.showSource = cfg.showSourceInfo;

        // Drop existing sinks (if any) so re-initialization is safe.
        S.sinks.clear();

        // Add built-in sinks according to the config.
        if (cfg.useConsole)       S.sinks.emplace_back(std::make_unique<ConsoleSink>(cfg.usePlatformOutput));
        if (cfg.useFile)          S.sinks.emplace_back(std::make_unique<FileSink>(cfg.filePath));
    }

    // Remove sinks and free resources.
    void Log::shutdown() {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        S.sinks.clear();
    }

    // Runtime control of threshold (useful to toggle verbose output).
    void Log::set_level(LogLevel lvl) { state().level = lvl; }
    LogLevel Log::get_level() { return state().level; }

    // Add a custom sink at runtime (e.g., network sink). Ownership transfers here.
    void Log::add_sink(std::unique_ptr<ILogSink> sink) {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        S.sinks.emplace_back(std::move(sink));
    }

    // Toggle "(file:line)" appendix for normal logs.
    void Log::set_show_source_info(bool enabled) { state().showSource = enabled; }

    // printf-style logging. We build a single fully formatted line and broadcast it.
    void Log::writef(LogLevel lvl, const char* tag, const char* file, int line, const char* fmt, ...) noexcept {

        // Filter by current threshold as early as possible.
        if (lvl > get_level()) return;

        // 1) Build the message body using printf-style formatting.
		char buf[2048]; // fixed-size buffer to avoid dynamic allocs in logging path
        va_list ap;
        va_start(ap, fmt);
    #if defined(_WIN32)
        _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    #else
        vsnprintf(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    #endif
        va_end(ap);

        // 2) Prepend timestamp, level, and tag. Keep it compact.
        std::ostringstream oss;
        oss << "[" << time_now_str() << "]"
            << "[" << level_to_cstr(lvl) << "]"
            << "[" << (tag ? tag : "LOG") << "] "
            << buf;

        // 3) Optionally append "(file:line)" for normal logs.
        if (state().showSource) {
            oss << " (" << file << ":" << line << ")";
        }

        // 4) Send to all sinks.
        dispatch_(lvl, tag ? tag : "LOG", oss.str().c_str());
    }

    // std::string-style logging (when you already have a formatted message).
    void Log::write(LogLevel lvl, const char* tag, const char* file, int line, const std::string& message) noexcept {
        if (lvl > get_level()) return;
        std::ostringstream oss;
        oss << "[" << time_now_str() << "]"
            << "[" << level_to_cstr(lvl) << "]"
            << "[" << (tag ? tag : "LOG") << "] "
            << message;
        if (state().showSource) {
            oss << " (" << file << ":" << line << ")";
        }
        dispatch_(lvl, tag ? tag : "LOG", oss.str().c_str());
    }

    // Deliver one formatted line to every sink.
    void Log::dispatch_(LogLevel lvl, const char* tag, const char* formatted) noexcept {
        auto& S = state();
        std::scoped_lock lk(S.mtx);

        // Each sink decides how to render (console/file/IDE window, etc.)  
        for (auto& s : S.sinks) s->write(lvl, tag, formatted);
    }

} // namespace eng::debug
