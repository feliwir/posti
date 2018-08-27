// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_PNGUTILS_P_H
#define _B2D_CODEC_PNGUTILS_P_H

// [Dependencies]
#include "../core/imagecodec.h"
#include "../core/math.h"
#include "../core/pixelconverter.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_codecs
//! \{

// ============================================================================
// [b2d::PngOps]
// ============================================================================

//! \internal
//!
//! Optimized PNG functions.
struct PngOps {
  Error (B2D_CDECL* inverseFilter)(uint8_t* p, uint32_t bpp, uint32_t bpl, uint32_t h);
};
extern PngOps _bPngOps;

// ============================================================================
// [b2d::PngInternal]
// ============================================================================

namespace PngInternal {

Error B2D_CDECL inverseFilterRef(uint8_t* p, uint32_t bpp, uint32_t bpl, uint32_t h) noexcept;

#if B2D_ARCH_SSE2 || B2D_MAYBE_SSE2
Error B2D_CDECL inverseFilterSSE2(uint8_t* p, uint32_t bpp, uint32_t bpl, uint32_t h) noexcept;
#endif

} // PngInternal namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_PNGUTILS_P_H
