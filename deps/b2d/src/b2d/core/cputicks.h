// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_CPUTICKS_H
#define _B2D_CORE_CPUTICKS_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::CpuTicks]
// ============================================================================

struct CpuTicks {
  static B2D_API uint32_t now() noexcept;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_CPUTICKS_H
