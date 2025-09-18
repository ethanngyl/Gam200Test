#include "DebugComponents/CrashLogger.h"
#include "DebugComponents/Log.h"

#include <cstdio>
#include <exception>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace eng::debug {

    // ---------- helpers: exe dir / timestamp ----------
    static std::string exe_dir_() {
#if defined(_WIN32)
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string s(path);
        const size_t pos = s.find_last_of("\\/");
        return (pos == std::string::npos) ? std::string(".") : s.substr(0, pos);
#else
        return ".";
#endif
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

#if defined(_WIN32)
    // ---------- helpers: symbols & stack trace ----------
    static void init_symbols_() {
        static bool inited = false;
        if (inited) return;
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        inited = true;
    }

    static std::string addr_to_string_(DWORD64 addr) {
        init_symbols_();
        HANDLE proc = GetCurrentProcess();

        char buffer[sizeof(SYMBOL_INFO) + 256];
        PSYMBOL_INFO sym = reinterpret_cast<PSYMBOL_INFO>(buffer);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;

        std::ostringstream oss;
        DWORD64 disp64 = 0;
        if (SymFromAddr(proc, addr, &disp64, sym)) {
            IMAGEHLP_LINE64 line{};
            line.SizeOfStruct = sizeof(line);
            DWORD lineDisp = 0;
            if (SymGetLineFromAddr64(proc, addr, &lineDisp, &line)) {
                oss << sym->Name << " +0x" << std::hex << disp64
                    << " (" << line.FileName << ":" << std::dec << line.LineNumber << ")";
            }
            else {
                oss << sym->Name << " +0x" << std::hex << disp64;
            }
        }
        else {
            oss << "0x" << std::hex << addr;
        }
        return oss.str();
    }

    static std::string capture_stack_(unsigned skipFrames = 0) {
        init_symbols_();
        void* frames[64];
        USHORT n = CaptureStackBackTrace(skipFrames + 1, 64 - skipFrames, frames, nullptr);
        std::ostringstream oss;
        for (USHORT i = 0; i < n; ++i) {
            DWORD64 a = reinterpret_cast<DWORD64>(frames[i]);
            oss << "  [" << i << "] " << addr_to_string_(a) << "\n";
        }
        return oss.str();
    }
#endif // _WIN32

    // ---------- CrashLogger API ----------

    void CrashLogger::install_handlers() {
#if defined(_WIN32)
        SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
        SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(seh_filter_));
#endif
        std::set_terminate(terminate_handler_);
        Log::write(LogLevel::Info, "CRASH", "", 0, "CrashLogger installed.");
    }

    void CrashLogger::force_crash_for_test() {
        int* p = nullptr;
        *p = 42;

    }

    void CrashLogger::write_report_(const char* title, const char* detail) {
        const std::string filename = std::string("crash_") + timestamp_() + ".txt";
        const std::string fullpath = exe_dir_() + "/" + filename;

        if (std::FILE* fp = std::fopen(fullpath.c_str(), "w")) {
            std::fprintf(fp, "%s\n", title ? title : "Crash");
            std::fprintf(fp, "--------------------------------------------------\n");
            std::fprintf(fp, "%s\n", detail ? detail : "(no details)");
            std::fclose(fp);
        }

        Log::write(LogLevel::Error, "CRASH", "", 0, std::string("Crash report written: ") + fullpath);
    }

    void CrashLogger::terminate_handler_() {
        std::string whatMsg;
        if (auto ex = std::current_exception()) {
            try { std::rethrow_exception(ex); }
            catch (const std::exception& e) { whatMsg = e.what(); }
            catch (...) { whatMsg = "Unknown exception type."; }
        }
        else {
            whatMsg = "std::terminate without active exception.";
        }

        std::ostringstream d;
        d << "Unhandled C++ termination.\n"
            << "Reason: " << whatMsg << "\n";

#if defined(_WIN32)
        d << "Stack:\n" << capture_stack_(0);
#endif

        write_report_("C++ terminate", d.str().c_str());
        std::abort();
    }

    long __stdcall CrashLogger::seh_filter_(_EXCEPTION_POINTERS* info) {
#if defined(_WIN32)
        unsigned long code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0UL;
        void* faultAddr = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr;

        std::ostringstream d;
        d << "Unhandled SEH exception.\n"
            << "Code: 0x" << std::hex << std::uppercase << code
            << " (" << seh_code_to_string_(code) << ")\n";

        if (faultAddr) {
            d << "Fault address: " << addr_to_string_(reinterpret_cast<DWORD64>(faultAddr)) << "\n";
        }

        d << "Stack:\n" << capture_stack_(0);

        write_report_("SEH Exception", d.str().c_str());
        return EXCEPTION_EXECUTE_HANDLER;
#else
        write_report_("SEH Exception", "SEH not supported on this platform.");
        return 1;
#endif
    }

    std::string CrashLogger::seh_code_to_string_(unsigned long code) {
#if defined(_WIN32)
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
        default: return "UNKNOWN_EXCEPTION";
        }
#else
        return "UNKNOWN_EXCEPTION";
#endif
    }

} // namespace eng::debug
