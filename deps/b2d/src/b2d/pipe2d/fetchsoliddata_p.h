// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHSOLIDDATA_P_H
#define _B2D_PIPE2D_FETCHSOLIDDATA_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchSolidData]
// ============================================================================

//! Solid fetch data - acts as a 32-bit or 64-bit integer.
struct FetchSolidData {
  inline void reset() noexcept { prgb64 = 0; }

  union {
    uint32_t prgb32;                   //!< 32-bit ARGB premultiplied.
    uint64_t prgb64;                   //!< 64-bit ARGB premultiplied.
  };
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHSOLIDDATA_P_H
