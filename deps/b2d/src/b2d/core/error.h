// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ERROR_H
#define _B2D_CORE_ERROR_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ErrorUtils]
// ============================================================================

namespace ErrorUtils {
  #if B2D_OS_WINDOWS
  B2D_API Error errorFromWinError(uint32_t e) noexcept;
  #endif

  #if B2D_OS_POSIX
  B2D_API Error errorFromPosixError(int e) noexcept;
  #endif
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ERROR_H
