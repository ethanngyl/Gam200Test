#include "CrashLogger.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <exception>
#include <new>
#include <sstream>
#include <iomanip>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace eng::debug {

    void CrashLogger::install_handlers() {
#if defined(_WIN32)
        // // Install SEH filter (Win32 native exception)
        SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(seh_filter_));
#endif
        // Installing C++ Termination Handlers
        std::set_terminate(terminate_handler_);

        Log::write(LogLevel::Info, "CRASH", "", 0, "CrashLogger installed.");
    }

    void CrashLogger::force_crash_for_test() {
        throw std::runtime_error("Simulated crash for testing");
    }


    void CrashLogger::write_report_(const char* title, const char* detail) {
        const std::string filename = std::string("crash_") + timestamp_() + ".txt";
        std::FILE* fp = std::fopen(filename.c_str(), "w");
        if (fp) {
            std::fprintf(fp, "%s\n", title ? title : "Crash");
            std::fprintf(fp, "--------------------------------------------------\n");
            std::fprintf(fp, "%s\n", detail ? detail : "(no details)");
            std::fclose(fp);
        }

        // Synchronize a line to the log system (rubric: concise mirror line)
        std::ostringstream oss;
        oss << "Crash report written: " << filename;
        Log::write(LogLevel::Error, "CRASH", "", 0, oss.str());
    }

    std::string CrashLogger::timestamp_() {
        using namespace std::chrono;
        const auto now = system_clock::now();
        const auto t = system_clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << (tm.tm_year + 1900)
            << std::setw(2) << (tm.tm_mon + 1)
            << std::setw(2) << tm.tm_mday << "_"
            << std::setw(2) << tm.tm_hour
            << std::setw(2) << tm.tm_min
            << std::setw(2) << tm.tm_sec;
        return oss.str();
    }

    void CrashLogger::terminate_handler_() {
        // Try to extract the current exception type information
        std::string whatMsg;
        if (auto exPtr = std::current_exception()) {
            try {
                std::rethrow_exception(exPtr);
            }
            catch (const std::exception& e) {
                whatMsg = e.what();
            }
            catch (...) {
                whatMsg = "Unknown exception type.";
            }
        }
        else {
            whatMsg = "std::terminate without active exception.";
        }

        std::ostringstream detail;
        detail << "Unhandled C++ termination.\n"
            << "Reason: " << (whatMsg.empty() ? "(no what())" : whatMsg) << "\n"
            << "Note: file/line will be attached by assert/crash sites if applicable.";

        write_report_("C++ terminate", detail.str().c_str());

        // Terminate a process
        std::abort();
    }

    long __stdcall CrashLogger::seh_filter_(_EXCEPTION_POINTERS* info) {
        unsigned long code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0UL;

        std::ostringstream detail;
        detail << "Unhandled SEH exception.\n"
            << "Code: 0x" << std::hex << std::uppercase << code
            << " (" << seh_code_to_string_(code) << ")\n"
            << "Note: stack trace omitted in minimal version.";

        write_report_("SEH Exception", detail.str().c_str());

        // Return EXCEPTION_EXECUTE_HANDLER to have the system terminate the program (and generate our report)
        return EXCEPTION_EXECUTE_HANDLER;
    }

    std::string CrashLogger::seh_code_to_string_(unsigned long code) {
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:     return "FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:       return "FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:    return "FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:             return "FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:          return "FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:            return "FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:             return "INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:      return "INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:              return "SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
        default: return "UNKNOWN_EXCEPTION";
        }
    }

} // namespace eng::debug
