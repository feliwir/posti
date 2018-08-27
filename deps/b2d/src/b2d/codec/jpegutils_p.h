// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_JPEGUTILS_P_H
#define _B2D_CODEC_JPEGUTILS_P_H

// [Dependencies]
#include "../codec/jpegcodec_p.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_codecs
//! \{

// ============================================================================
// [b2d::Jpeg - Macros]
// ============================================================================

// Derived from jidctint's `jpeg_idct_islow`.
#define B2D_JPEG_IDCT_PREC 12
#define B2D_JPEG_IDCT_HALF(precision) (1 << ((precision) - 1))

#define B2D_JPEG_IDCT_SCALE(x) ((x) << B2D_JPEG_IDCT_PREC)
#define B2D_JPEG_IDCT_FIXED(x) int(double(x) * double(1 << B2D_JPEG_IDCT_PREC) + 0.5)

#define B2D_JPEG_IDCT_M_2_562915447 (-B2D_JPEG_IDCT_FIXED(2.562915447))
#define B2D_JPEG_IDCT_M_1_961570560 (-B2D_JPEG_IDCT_FIXED(1.961570560))
#define B2D_JPEG_IDCT_M_1_847759065 (-B2D_JPEG_IDCT_FIXED(1.847759065))
#define B2D_JPEG_IDCT_M_0_899976223 (-B2D_JPEG_IDCT_FIXED(0.899976223))
#define B2D_JPEG_IDCT_M_0_390180644 (-B2D_JPEG_IDCT_FIXED(0.390180644))
#define B2D_JPEG_IDCT_P_0_298631336 ( B2D_JPEG_IDCT_FIXED(0.298631336))
#define B2D_JPEG_IDCT_P_0_541196100 ( B2D_JPEG_IDCT_FIXED(0.541196100))
#define B2D_JPEG_IDCT_P_0_765366865 ( B2D_JPEG_IDCT_FIXED(0.765366865))
#define B2D_JPEG_IDCT_P_1_175875602 ( B2D_JPEG_IDCT_FIXED(1.175875602))
#define B2D_JPEG_IDCT_P_1_501321110 ( B2D_JPEG_IDCT_FIXED(1.501321110))
#define B2D_JPEG_IDCT_P_2_053119869 ( B2D_JPEG_IDCT_FIXED(2.053119869))
#define B2D_JPEG_IDCT_P_3_072711026 ( B2D_JPEG_IDCT_FIXED(3.072711026))

// Keep 2 bits of extra precision for the intermediate results.
#define B2D_JPEG_IDCT_COL_NORM (B2D_JPEG_IDCT_PREC - 2)
#define B2D_JPEG_IDCT_COL_BIAS B2D_JPEG_IDCT_HALF(B2D_JPEG_IDCT_COL_NORM)

// Consume 2 bits of an intermediate results precision and 3 bits that were
// produced by `2 * sqrt(8)`. Also normalize to from `-128..127` to `0..255`.
#define B2D_JPEG_IDCT_ROW_NORM (B2D_JPEG_IDCT_PREC + 2 + 3)
#define B2D_JPEG_IDCT_ROW_BIAS (B2D_JPEG_IDCT_HALF(B2D_JPEG_IDCT_ROW_NORM) + (128 << B2D_JPEG_IDCT_ROW_NORM))

#define B2D_JPEG_YCBCR_PREC 12
#define B2D_JPEG_YCBCR_SCALE(x) ((x) << B2D_JPEG_YCBCR_PREC)
#define B2D_JPEG_YCBCR_FIXED(x) int(double(x) * double(1 << B2D_JPEG_YCBCR_PREC) + 0.5)

// ============================================================================
// [b2d::JpegOps]
// ============================================================================

//! \internal
//!
//! Optimized JPEG functions.
struct JpegOps {
  //! Dequantize and perform IDCT and store clamped 8-bit results to `dst`.
  void (B2D_CDECL* idct8)(uint8_t* dst, intptr_t dstStride, const int16_t* src, const uint16_t* qTable);

  //! No upsampling (stub).
  uint8_t* (B2D_CDECL* upsample1x1)(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs);
  //! Upsample row in vertical direction.
  uint8_t* (B2D_CDECL* upsample1x2)(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs);
  //! Upsample row in horizontal direction.
  uint8_t* (B2D_CDECL* upsample2x1)(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs);
  //! Upsample row in vertical and horizontal direction.
  uint8_t* (B2D_CDECL* upsample2x2)(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs);
  //! Upsample row any.
  uint8_t* (B2D_CDECL* upsampleAny)(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs);

  //! Perform planar YCbCr to RGB conversion and pack to XRGB32.
  void (B2D_CDECL* convYCbCr8ToRGB32)(uint8_t* dst, const uint8_t* pY, const uint8_t* pCb, const uint8_t* pCr, uint32_t count);
};
extern JpegOps _bJpegOps;

// ============================================================================
// [b2d::JpegInternal]
// ============================================================================

namespace JpegInternal {

void B2D_CDECL idct8_ref(uint8_t* dst, intptr_t dstStride, const int16_t* src, const uint16_t* qTable) noexcept;
uint8_t* B2D_CDECL upsample1x1_ref(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs) noexcept;
uint8_t* B2D_CDECL upsample1x2_ref(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs) noexcept;
uint8_t* B2D_CDECL upsample2x1_ref(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs) noexcept;
uint8_t* B2D_CDECL upsample2x2_ref(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs) noexcept;
uint8_t* B2D_CDECL upsampleAny_ref(uint8_t* dst, uint8_t* src0, uint8_t* src1, uint32_t w, uint32_t hs) noexcept;
void B2D_CDECL convYCbCr8ToRGB32_ref(uint8_t* dst, const uint8_t* pY, const uint8_t* pCb, const uint8_t* pCr, uint32_t count) noexcept;

#if B2D_ARCH_SSE2 || B2D_MAYBE_SSE2
void B2D_CDECL idct8_sse2(uint8_t* dst, intptr_t dstStride, const int16_t* src, const uint16_t* qTable) noexcept;
void B2D_CDECL convYCbCr8ToRGB32_sse2(uint8_t* dst, const uint8_t* pY, const uint8_t* pCb, const uint8_t* pCr, uint32_t count) noexcept;
#endif

} // JpegInternal namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_JPEGUTILS_P_H
