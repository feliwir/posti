// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/cputicks.h"

#include <asmjit/asmjit.h>

B2D_BEGIN_NAMESPACE

uint32_t CpuTicks::now() noexcept {
  // Already implemented in asmjit, just redirect.
  return asmjit::OSUtils::getTickCount();
}

B2D_END_NAMESPACE
