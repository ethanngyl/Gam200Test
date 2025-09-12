// ----------------------------------------------------------------------------
// Minimal Logger: console + file, level filter, thread-safe via std::mutex
// Now uses std::ofstream instead of fopen/fprintf
// ----------------------------------------------------------------------------
#include "Log.h"
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <thread>
#include <fstream>

namespace Engine {
    namespace Debug {

        static std::ofstream g_file;   // file sink
        static std::mutex    g_mutex;
        static LogLevel      g_minLevel = LogLevel::Trace;

        // ----------------------------------------------------------------------------
        // [now_string]
        // [Returns a simple local time string "HH:MM:SS.mmm".]
        // ----------------------------------------------------------------------------
        static std::string now_string() {
            using namespace std::chrono;
            const auto tp = system_clock::now();
            const auto t = system_clock::to_time_t(tp);
            const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

            std::tm buf{};
        #if defined(_WIN32)
            localtime_s(&buf, &t);
        #else
            localtime_r(&t, &buf);
        #endif
            char timebuf[32];
            std::snprintf(timebuf, sizeof(timebuf), "%02d:%02d:%02d.%03d",
                buf.tm_hour, buf.tm_min, buf.tm_sec, (int)ms.count());
            return timebuf;
        }

        static const char* level_name(LogLevel l) {
            switch (l) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "?";
            }
        }

        void InitLogger(const char* filePath) {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (!g_file.is_open()) {
                g_file.open(filePath, std::ios::out | std::ios::trunc);
            }
        }

        void ShutdownLogger() {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (g_file.is_open()) {
                g_file.flush();
                g_file.close();
            }
        }

        void SetMinLevel(LogLevel lvl) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_minLevel = lvl;
        }

        // ----------------------------------------------------------------------------
        // [Log]
        // [Thread-safe printf-style logging to stderr and file sink.]
        // ----------------------------------------------------------------------------
        void Log(LogLevel lvl, const char* category, const char* fmt, ...) {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (lvl < g_minLevel) return;

            char msgbuf[2048];
            va_list args;
            va_start(args, fmt);
            std::vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
            va_end(args);

            const auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
            const auto ts = now_string();

            // console (stderr)
            std::fprintf(stderr, "[%s][%s][%s][T%zu] %s\n",
                ts.c_str(), level_name(lvl), category ? category : "-",
                (size_t)tid, msgbuf);

            // file sink
            if (g_file.is_open()) {
                g_file << "[" << ts << "][" << level_name(lvl) << "]["
                    << (category ? category : "-") << "][T" << tid << "] "
                    << msgbuf << "\n";
                g_file.flush();
            }
        }

    } // namespace Engine::Debug
} // namespace Engine
