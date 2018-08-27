// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHDATA_P_H
#define _B2D_PIPE2D_FETCHDATA_P_H

// [Dependencies]
#include "./fetchgradientdata_p.h"
#include "./fetchsoliddata_p.h"
#include "./fetchtexturedata_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchData]
// ============================================================================

struct FetchData {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    FetchSolidData solid;
    FetchGradientData gradient;
    FetchTextureData texture;
  };
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHDATA_P_H
