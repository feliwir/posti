// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::DebugUtils]
// ============================================================================

void DebugUtils::debugOutput(const char* str) noexcept {
#if B2D_OS_WINDOWS
  ::OutputDebugStringA(str);
#else
  std::fputs(str, stderr);
#endif
}

void DebugUtils::debugFormat(const char* fmt, ...) noexcept {
  char buf[1024];
  std::va_list ap;

  va_start(ap, fmt);
  std::vsnprintf(buf, B2D_ARRAY_SIZE(buf), fmt, ap);
  va_end(ap);

  debugOutput(buf);
}

void DebugUtils::assertionFailed(const char* file, int line, const char* msg) noexcept {
  char buf[1024];

  std::snprintf(buf, 1024,
    "[b2d] Assertion failed at %s (line %d):\n"
    "[b2d] %s\n", file, line, msg);

  debugOutput(buf);
  std::abort();
}

B2D_END_NAMESPACE
