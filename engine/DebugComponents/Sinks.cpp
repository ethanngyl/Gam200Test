#include "Sinks.h"
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#endif

/*
===============================================================================
 Sinks.cpp
 ------------------------------------------------------------------------------
 Implements two concrete sinks for the logging system:

   - ConsoleSink: prints to stderr and (optionally) to VS Output window.
   - FileSink   : appends to a text file.

 Both sinks expect that the incoming 'msg' is already fully formatted by the
 Log facade (timestamp, level, tag, message body). Sinks just render it.

 Implementation notes:
   - ConsoleSink uses fprintf(stderr, ...) and flushes after each line so you
     can see output immediately in a console.
   - On Windows, ConsoleSink can also call OutputDebugStringA so that messages
     appear in Visual Studio's Output window when debugging.
   - FileSink uses std::ofstream opened in append mode and flushes per write.
===============================================================================
*/
namespace eng::debug {  

    // Small helper: convert enum LogLevel to a short text label.
    // We keep it here (local to this .cpp) because only sinks need it.
    static const char* lvl_to_cstr(LogLevel l) {
        switch (l) {
        case LogLevel::Error: return "ERROR";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Info: return "INFO";
        default:              return "DEBUG";
        }
    }

    // ConsoleSink::write
    // -------------------------------------------------------------------------
    // Print to stderr. Example line:
    //   [INFO][CORE] 12:34:56.789 Message text ...
    //
    // Then, on Windows and when enabled, mirror the same line to the VS Output
    // window via OutputDebugStringA. This is handy when running without a
    // console (e.g., double-clicking the .exe from Explorer).
    void ConsoleSink::write(LogLevel lvl, const char* tag, const char* msg) {
        std::fprintf(stderr, "[%s][%s] %s\n", lvl_to_cstr(lvl), tag, msg);
        std::fflush(stderr);
    #if defined(_WIN32)
        if (m_usePlatformOutput) {

            // Build one string and send it to the debugger output.
            std::string line = "[" + std::string(lvl_to_cstr(lvl)) + "][" + tag + "] " + msg + "\n";
            OutputDebugStringA(line.c_str());
        }
    #endif
    }

    // FileSink: open the file in append mode so previous logs are kept.
    FileSink::FileSink(const std::string& path) : m_out(path, std::ios::out | std::ios::app) {}

    // Flush on destruction to make sure no buffered data is lost.
    FileSink::~FileSink() { if (m_out.is_open()) m_out.flush(); }

    // FileSink::write
    // -------------------------------------------------------------------------
    // Append one line to the log file and flush immediately. Flushing ensures
    // the line makes it to disk even if the program exits unexpectedly.
    void FileSink::write(LogLevel lvl, const char* tag, const char* msg) {
        if (!m_out.is_open()) return;
        m_out << "[" << lvl_to_cstr(lvl) << "][" << tag << "] " << msg << "\n";
        m_out.flush();
    }

} // namespace eng::debug
