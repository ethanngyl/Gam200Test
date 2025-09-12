#pragma once
// ----------------------------------------------------------------------------
// Assert utilities
// Provide runtime checks in debug; compile out in release.
// ----------------------------------------------------------------------------
#pragma once
#include <cstdio>
#include <cstdlib>

namespace Engine {
    namespace Debug {
        // ----------------------------------------------------------------------------
        // [ReportAssert]
        // [Prints assertion details to stderr and (optionally) a log sink.]
        // ----------------------------------------------------------------------------
        void ReportAssert(const char* expr, const char* msg, const char* file, int line);

        // ----------------------------------------------------------------------------
        // [DebugBreak]
        // [Interrupts execution so the debugger can break at the failure site.]
        // ----------------------------------------------------------------------------
        void DebugBreak();
    }
}

// ----------------------------------------------------------------------------
// [ENGINE_ASSERT]
// [Use for invariants. In debug builds, reports and breaks; in release, no-op.]
// ----------------------------------------------------------------------------
#if defined(ENGINE_ENABLE_DEBUG)
#define ENGINE_ASSERT(expr, msg) \
    do { \
      if (!(expr)) { \
        ::Engine::Debug::ReportAssert(#expr, msg, __FILE__, __LINE__); \
        ::Engine::Debug::DebugBreak(); \
      } \
    } while(0)
#else
#define ENGINE_ASSERT(expr, msg) do { (void)sizeof(expr); } while(0)
#endif


