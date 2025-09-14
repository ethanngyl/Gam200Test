#include "Log.h"
#include "Sinks.h"

#include <chrono>
#include <cstdarg>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace eng::debug {

    namespace {
        struct LogState {
            LogLevel level = LogLevel::Info;
            std::vector<std::unique_ptr<ILogSink>> sinks;
            std::mutex mtx;
            bool showSource = false;
        };
        static LogState& state() { static LogState S; return S; }

        static std::string time_now_str() {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto t = system_clock::to_time_t(now);
            auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
            std::tm tm{};
        #if defined(_WIN32)
            localtime_s(&tm, &t);
        #else
            localtime_r(&t, &tm);
        #endif
            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(2) << tm.tm_hour << ":"
                << std::setw(2) << tm.tm_min << ":"
                << std::setw(2) << tm.tm_sec << "."
                << std::setw(3) << ms;
            return oss.str();
        }

        static std::string level_to_cstr(LogLevel l) {
            switch (l) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warn: return "WARN";
            case LogLevel::Info: return "INFO";
            default:              return "DEBUG";
            }
        }
    } // namespace

    void Log::init(const LogConfig& cfg) {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        S.level = cfg.level;
        S.showSource = cfg.showSourceInfo;
        S.sinks.clear();
        if (cfg.useConsole)       S.sinks.emplace_back(std::make_unique<ConsoleSink>(cfg.usePlatformOutput));
        if (cfg.useFile)          S.sinks.emplace_back(std::make_unique<FileSink>(cfg.filePath));
    }

    void Log::shutdown() {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        S.sinks.clear();
    }

    void Log::set_level(LogLevel lvl) { state().level = lvl; }
    LogLevel Log::get_level() { return state().level; }

    void Log::add_sink(std::unique_ptr<ILogSink> sink) {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        S.sinks.emplace_back(std::move(sink));
    }

    void Log::set_show_source_info(bool enabled) { state().showSource = enabled; }

    void Log::writef(LogLevel lvl, const char* tag, const char* file, int line, const char* fmt, ...) noexcept {
        if (lvl > get_level()) return;

        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
    #if defined(_WIN32)
        _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    #else
        vsnprintf(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    #endif
        va_end(ap);

        std::ostringstream oss;
        oss << "[" << time_now_str() << "]"
            << "[" << level_to_cstr(lvl) << "]"
            << "[" << (tag ? tag : "LOG") << "] "
            << buf;

        if (state().showSource) {
            oss << " (" << file << ":" << line << ")";
        }

        dispatch_(lvl, tag ? tag : "LOG", oss.str().c_str());
    }

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

    void Log::dispatch_(LogLevel lvl, const char* tag, const char* formatted) noexcept {
        auto& S = state();
        std::scoped_lock lk(S.mtx);
        for (auto& s : S.sinks) s->write(lvl, tag, formatted);
    }

} // namespace eng::debug
