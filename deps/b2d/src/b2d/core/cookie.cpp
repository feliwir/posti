// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/cookie.h"
#include "../core/uniqueidgenerator_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Cookie - generateUnique64BitId]
// ============================================================================

uint64_t Cookie::generateUnique64BitId() noexcept {
  static UniqueIdGenerator _cookieIdGenerator;
  return _cookieIdGenerator.next();
}

B2D_END_NAMESPACE
