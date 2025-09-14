#include "Sinks.h"
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace eng::debug {

    static const char* lvl_to_cstr(LogLevel l) {
        switch (l) {
        case LogLevel::Error: return "ERROR";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Info: return "INFO";
        default:              return "DEBUG";
        }
    }

    void ConsoleSink::write(LogLevel lvl, const char* tag, const char* msg) {
        std::fprintf(stderr, "[%s][%s] %s\n", lvl_to_cstr(lvl), tag, msg);
        std::fflush(stderr);
#if defined(_WIN32)
        if (m_usePlatformOutput) {
            std::string line = "[" + std::string(lvl_to_cstr(lvl)) + "][" + tag + "] " + msg + "\n";
            OutputDebugStringA(line.c_str());
        }
#endif
    }

    FileSink::FileSink(const std::string& path) : m_out(path, std::ios::out | std::ios::app) {}
    FileSink::~FileSink() { if (m_out.is_open()) m_out.flush(); }

    void FileSink::write(LogLevel lvl, const char* tag, const char* msg) {
        if (!m_out.is_open()) return;
        m_out << "[" << lvl_to_cstr(lvl) << "][" << tag << "] " << msg << "\n";
        m_out.flush();
    }

} // namespace eng::debug
