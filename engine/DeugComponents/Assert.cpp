// ----------------------------------------------------------------------------
// Assert implementation (C++ style + integrated with Logger)
// ----------------------------------------------------------------------------
#include "Assert.h"
#include "Log.h"      
#include <iostream>   // std::cerr

#if defined(_WIN32)
#include <intrin.h>
#endif

#include <cstdlib>    // std::abort

namespace Engine {
    namespace Debug {

        // ----------------------------------------------------------------------------
        // [ReportAssert]
        // [Formats and writes assertion failure information to std::cerr and log.txt.]
        // ----------------------------------------------------------------------------
        void ReportAssert(const char* expr, const char* msg, const char* file, int line) {
            std::cerr << "[ASSERT] " << (expr ? expr : "(null)")
                << "\nMessage: " << (msg ? msg : "")
                << "\nAt " << (file ? file : "(unknown)") << ":" << line
                << std::endl;

            LOGF("Assert", "%s | Message: %s | At %s:%d",
                (expr ? expr : "(null)"),
                (msg ? msg : ""),
                (file ? file : "(unknown)"),
                line);
        }

        // ----------------------------------------------------------------------------
        // [DebugBreak]
        // [Breaks into debugger; falls back to abort if unsupported.]
        // ----------------------------------------------------------------------------
        void DebugBreak() {
        #if defined(_WIN32)
            __debugbreak();
        #elif defined(__GNUC__)
            __builtin_trap();
        #else
            std::abort();
        #endif
        }

    } // namespace Debug
} // namespace Engine
