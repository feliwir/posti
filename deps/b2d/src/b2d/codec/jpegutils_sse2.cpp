// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// The JPEG codec is based on stb_image <https://github.com/nothings/stb>
// released into PUBLIC DOMAIN. Blend2D's JPEG codec can be distributed
// under Blend2D's ZLIB license or under STB's PUBLIC DOMAIN as well.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/jpegcodec_p.h"
#include "../codec/jpegutils_p.h"
#include "../core/simd.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE
namespace JpegInternal {

// ============================================================================
// [b2d::JpegInternal - IDCT (SSE2)]
// ============================================================================

struct alignas(16) JpegSSE2Constants {
  // IDCT.
  int16_t idct_rot0a[8], idct_rot0b[8];
  int16_t idct_rot1a[8], idct_rot1b[8];
  int16_t idct_rot2a[8], idct_rot2b[8];
  int16_t idct_rot3a[8], idct_rot3b[8];

  int32_t idct_col_bias[4];
  int32_t idct_row_bias[4];

  // YCbCr.
  int32_t ycbcr_allones[4];
  int16_t ycbcr_tosigned[8];
  int32_t ycbcr_round[4];
  int16_t ycbcr_yycrMul[8];
  int16_t ycbcr_yycbMul[8];
  int16_t ycbcr_cbcrMul[8];
};

#define DATA_4X(...) { __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__ }
static const JpegSSE2Constants jpegSSE2Constants = {
  // IDCT.
  DATA_4X(B2D_JPEG_IDCT_P_0_541196100                               ,
          B2D_JPEG_IDCT_P_0_541196100 + B2D_JPEG_IDCT_M_1_847759065),
  DATA_4X(B2D_JPEG_IDCT_P_0_541196100 + B2D_JPEG_IDCT_P_0_765366865 ,
          B2D_JPEG_IDCT_P_0_541196100                              ),
  DATA_4X(B2D_JPEG_IDCT_P_1_175875602 + B2D_JPEG_IDCT_M_0_899976223 ,
          B2D_JPEG_IDCT_P_1_175875602                              ),
  DATA_4X(B2D_JPEG_IDCT_P_1_175875602                               ,
          B2D_JPEG_IDCT_P_1_175875602 + B2D_JPEG_IDCT_M_2_562915447),
  DATA_4X(B2D_JPEG_IDCT_M_1_961570560 + B2D_JPEG_IDCT_P_0_298631336 ,
          B2D_JPEG_IDCT_M_1_961570560                              ),
  DATA_4X(B2D_JPEG_IDCT_M_1_961570560                               ,
          B2D_JPEG_IDCT_M_1_961570560 + B2D_JPEG_IDCT_P_3_072711026),
  DATA_4X(B2D_JPEG_IDCT_M_0_390180644 + B2D_JPEG_IDCT_P_2_053119869 ,
          B2D_JPEG_IDCT_M_0_390180644                              ),
  DATA_4X(B2D_JPEG_IDCT_M_0_390180644                               ,
          B2D_JPEG_IDCT_M_0_390180644 + B2D_JPEG_IDCT_P_1_501321110),

  DATA_4X(B2D_JPEG_IDCT_COL_BIAS),
  DATA_4X(B2D_JPEG_IDCT_ROW_BIAS),

  // YCbCr.
  DATA_4X(-1),
  DATA_4X(-128, -128),
  DATA_4X(1 << (B2D_JPEG_YCBCR_PREC - 1)),
  DATA_4X( B2D_JPEG_YCBCR_FIXED(1.00000),  B2D_JPEG_YCBCR_FIXED(1.40200)),
  DATA_4X( B2D_JPEG_YCBCR_FIXED(1.00000),  B2D_JPEG_YCBCR_FIXED(1.77200)),
  DATA_4X(-B2D_JPEG_YCBCR_FIXED(0.34414), -B2D_JPEG_YCBCR_FIXED(0.71414))
};
#undef DATA_4X

#define B2D_JPEG_CONST_XMM(c) (*(const SIMD::I128*)(jpegSSE2Constants.c))

#define B2D_JPEG_IDCT_INTERLEAVE8_XMM(a, b) { SIMD::I128 t = a; a = _mm_unpacklo_epi8(a, b); b = _mm_unpackhi_epi8(t, b); }
#define B2D_JPEG_IDCT_INTERLEAVE16_XMM(a, b) { SIMD::I128 t = a; a = _mm_unpacklo_epi16(a, b); b = _mm_unpackhi_epi16(t, b); }

// out(0) = c0[even]*x + c0[odd]*y (in 16-bit, out 32-bit).
// out(1) = c1[even]*x + c1[odd]*y (in 16-bit, out 32-bit).
#define B2D_JPEG_IDCT_ROTATE_XMM(dst0, dst1, x, y, c0, c1) \
  SIMD::I128 c0##_l = _mm_unpacklo_epi16(x, y); \
  SIMD::I128 c0##_h = _mm_unpackhi_epi16(x, y); \
  \
  SIMD::I128 dst0##_l = _mm_madd_epi16(c0##_l, B2D_JPEG_CONST_XMM(c0)); \
  SIMD::I128 dst0##_h = _mm_madd_epi16(c0##_h, B2D_JPEG_CONST_XMM(c0)); \
  SIMD::I128 dst1##_l = _mm_madd_epi16(c0##_l, B2D_JPEG_CONST_XMM(c1)); \
  SIMD::I128 dst1##_h = _mm_madd_epi16(c0##_h, B2D_JPEG_CONST_XMM(c1));

// out = in << 12 (in 16-bit, out 32-bit)
#define B2D_JPEG_IDCT_WIDEN_XMM(dst, in) \
  SIMD::I128 dst##_l = SIMD::vsrai32<4>(_mm_unpacklo_epi16(SIMD::vzeroi128(), in)); \
  SIMD::I128 dst##_h = SIMD::vsrai32<4>(_mm_unpackhi_epi16(SIMD::vzeroi128(), in));

// Wide add (32-bit).
#define B2D_JPEG_IDCT_WADD_XMM(dst, a, b) \
  SIMD::I128 dst##_l = SIMD::vaddi32(a##_l, b##_l); \
  SIMD::I128 dst##_h = SIMD::vaddi32(a##_h, b##_h);

// Wide sub (32-bit).
#define B2D_JPEG_IDCT_WSUB_XMM(dst, a, b) \
  SIMD::I128 dst##_l = SIMD::vsubi32(a##_l, b##_l); \
  SIMD::I128 dst##_h = SIMD::vsubi32(a##_h, b##_h);

// Butterfly a/b, add bias, then shift by `norm` and pack to 16-bit.
#define B2D_JPEG_IDCT_BFLY_XMM(dst0, dst1, a, b, bias, norm) { \
  SIMD::I128 abiased_l = SIMD::vaddi32(a##_l, bias); \
  SIMD::I128 abiased_h = SIMD::vaddi32(a##_h, bias); \
  \
  B2D_JPEG_IDCT_WADD_XMM(sum, abiased, b) \
  B2D_JPEG_IDCT_WSUB_XMM(diff, abiased, b) \
  \
  dst0 = SIMD::vpacki32i16(SIMD::vsrai32<norm>(sum_l), SIMD::vsrai32<norm>(sum_h)); \
  dst1 = SIMD::vpacki32i16(SIMD::vsrai32<norm>(diff_l), SIMD::vsrai32<norm>(diff_h)); \
}

#define B2D_JPEG_IDCT_IDCT_PASS_XMM(bias, norm) { \
  /* Even part. */ \
  B2D_JPEG_IDCT_ROTATE_XMM(t2e, t3e, row2, row6, idct_rot0a, idct_rot0b) \
  \
  SIMD::I128 sum04 = _mm_add_epi16(row0, row4); \
  SIMD::I128 dif04 = _mm_sub_epi16(row0, row4); \
  \
  B2D_JPEG_IDCT_WIDEN_XMM(t0e, sum04) \
  B2D_JPEG_IDCT_WIDEN_XMM(t1e, dif04) \
  \
  B2D_JPEG_IDCT_WADD_XMM(x0, t0e, t3e) \
  B2D_JPEG_IDCT_WSUB_XMM(x3, t0e, t3e) \
  B2D_JPEG_IDCT_WADD_XMM(x1, t1e, t2e) \
  B2D_JPEG_IDCT_WSUB_XMM(x2, t1e, t2e) \
  \
  /* Odd part */ \
  B2D_JPEG_IDCT_ROTATE_XMM(y0o, y2o, row7, row3, idct_rot2a, idct_rot2b) \
  B2D_JPEG_IDCT_ROTATE_XMM(y1o, y3o, row5, row1, idct_rot3a, idct_rot3b) \
  SIMD::I128 sum17 = _mm_add_epi16(row1, row7); \
  SIMD::I128 sum35 = _mm_add_epi16(row3, row5); \
  B2D_JPEG_IDCT_ROTATE_XMM(y4o,y5o, sum17, sum35, idct_rot1a, idct_rot1b) \
  \
  B2D_JPEG_IDCT_WADD_XMM(x4, y0o, y4o) \
  B2D_JPEG_IDCT_WADD_XMM(x5, y1o, y5o) \
  B2D_JPEG_IDCT_WADD_XMM(x6, y2o, y5o) \
  B2D_JPEG_IDCT_WADD_XMM(x7, y3o, y4o) \
  \
  B2D_JPEG_IDCT_BFLY_XMM(row0, row7, x0, x7, bias, norm) \
  B2D_JPEG_IDCT_BFLY_XMM(row1, row6, x1, x6, bias, norm) \
  B2D_JPEG_IDCT_BFLY_XMM(row2, row5, x2, x5, bias, norm) \
  B2D_JPEG_IDCT_BFLY_XMM(row3, row4, x3, x4, bias, norm) \
}

void B2D_CDECL idct8_sse2(uint8_t* dst, intptr_t dstStride, const int16_t* src, const uint16_t* qTable) noexcept {
  // Load and dequantize.
  SIMD::I128 row0 = SIMD::vmuli16(*(const SIMD::I128*)(src +  0), *(const SIMD::I128*)(qTable +  0));
  SIMD::I128 row1 = SIMD::vmuli16(*(const SIMD::I128*)(src +  8), *(const SIMD::I128*)(qTable +  8));
  SIMD::I128 row2 = SIMD::vmuli16(*(const SIMD::I128*)(src + 16), *(const SIMD::I128*)(qTable + 16));
  SIMD::I128 row3 = SIMD::vmuli16(*(const SIMD::I128*)(src + 24), *(const SIMD::I128*)(qTable + 24));
  SIMD::I128 row4 = SIMD::vmuli16(*(const SIMD::I128*)(src + 32), *(const SIMD::I128*)(qTable + 32));
  SIMD::I128 row5 = SIMD::vmuli16(*(const SIMD::I128*)(src + 40), *(const SIMD::I128*)(qTable + 40));
  SIMD::I128 row6 = SIMD::vmuli16(*(const SIMD::I128*)(src + 48), *(const SIMD::I128*)(qTable + 48));
  SIMD::I128 row7 = SIMD::vmuli16(*(const SIMD::I128*)(src + 56), *(const SIMD::I128*)(qTable + 56));

  // IDCT columns.
  B2D_JPEG_IDCT_IDCT_PASS_XMM(B2D_JPEG_CONST_XMM(idct_col_bias), B2D_JPEG_IDCT_COL_NORM)

  // Transpose.
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row0, row4) // [a0a4|b0b4|c0c4|d0d4] | [e0e4|f0f4|g0g4|h0h4]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row2, row6) // [a2a6|b2b6|c2c6|d2d6] | [e2e6|f2f6|g2g6|h2h6]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row1, row5) // [a1a5|b1b5|c2c5|d1d5] | [e1e5|f1f5|g1g5|h1h5]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row3, row7) // [a3a7|b3b7|c3c7|d3d7] | [e3e7|f3f7|g3g7|h3h7]

  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row0, row2) // [a0a2|a4a6|b0b2|b4b6] | [c0c2|c4c6|d0d2|d4d6]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row1, row3) // [a1a3|a5a7|b1b3|b5b7] | [c1c3|c5c7|d1d3|d5d7]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row4, row6) // [e0e2|e4e6|f0f2|f4f6] | [g0g2|g4g6|h0h2|h4h6]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row5, row7) // [e1e3|e5e7|f1f3|f5f7] | [g1g3|g5g7|h1h3|h5h7]

  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row0, row1) // [a0a1|a2a3|a4a5|a6a7] | [b0b1|b2b3|b4b5|b6b7]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row2, row3) // [c0c1|c2c3|c4c5|c6c7] | [d0d1|d2d3|d4d5|d6d7]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row4, row5) // [e0e1|e2e3|e4e5|e6e7] | [f0f1|f2f3|f4f5|f6f7]
  B2D_JPEG_IDCT_INTERLEAVE16_XMM(row6, row7) // [g0g1|g2g3|g4g5|g6g7] | [h0h1|h2h3|h4h5|h6h7]

  // IDCT rows.
  B2D_JPEG_IDCT_IDCT_PASS_XMM(B2D_JPEG_CONST_XMM(idct_row_bias), B2D_JPEG_IDCT_ROW_NORM)

  // Pack to 8-bit integers, also saturates the result to 0..255 as a side-effect.
  row0 = SIMD::vpacki16u8(row0, row1);        // [a0a1a2a3|a4a5a6a7|b0b1b2b3|b4b5b6b7]
  row2 = SIMD::vpacki16u8(row2, row3);        // [c0c1c2c3|c4c5c6c7|d0d1d2d3|d4d5d6d7]
  row4 = SIMD::vpacki16u8(row4, row5);        // [e0e1e2e3|e4e5e6e7|f0f1f2f3|f4f5f6f7]
  row6 = SIMD::vpacki16u8(row6, row7);        // [g0g1g2g3|g4g5g6g7|h0h1h2h3|h4h5h6h7]

  // Transpose.
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row0, row4)  // [a0e0a1e1|a2e2a3e3|a4e4a5e5|a6e6a7e7] | [b0f0b1f1|b2f2b3f3|b4f4b5f5|b6f6b7f7]
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row2, row6)  // [c0g0c1g1|c2g2c3g3|c4g4c5g5|c6g6c7g7] | [d0h0d1h1|d2h2d3h3|d4h4d5h5|d6h6d7h7]
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row0, row2)  // [a0c0e0g0|a1c1e1g1|a2c2e2g2|a3c3e3g3] | [a4c4e4g4|a5c5e5g5|a6c6e6g6|a7c7e7g7]
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row4, row6)  // [b0d0f0h0|b1d1f1h1|b2d2f2h2|b3d3f3h3| | [b4d4f4h4|b5d5f5h5|b6d6f6h6|b7d7f7h7]
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row0, row4)  // [a0b0c0d0|e0f0g0h0|a1b1c1d1|e1f1g1h1] | [a2b2c2d2|e2f2g2h2|a3b3c3d3|e3f3g3h3]
  B2D_JPEG_IDCT_INTERLEAVE8_XMM(row2, row6)  // [a4b4c4d4|e4f4g4h4|a5b5c5d5|e5f5g5h5] | [a6b6c6d6|e6f6g6h6|a7b7c7d7|e7f7g7h7]

  // Store.
  uint8_t* dst0 = dst;
  uint8_t* dst1 = dst + dstStride;
  intptr_t dstStride2 = dstStride * 2;

  SIMD::vstoreli64(dst0, row0); dst0 += dstStride2;
  SIMD::vstorehi64(dst1, row0); dst1 += dstStride2;

  SIMD::vstoreli64(dst0, row4); dst0 += dstStride2;
  SIMD::vstorehi64(dst1, row4); dst1 += dstStride2;

  SIMD::vstoreli64(dst0, row2); dst0 += dstStride2;
  SIMD::vstorehi64(dst1, row2); dst1 += dstStride2;

  SIMD::vstoreli64(dst0, row6);
  SIMD::vstorehi64(dst1, row6);
}

// ============================================================================
// [b2d::JpegInternal - ConvYCbCrToRGB32 (SSE2)]
// ============================================================================

void B2D_CDECL convYCbCr8ToRGB32_sse2(uint8_t* dst, const uint8_t* pY, const uint8_t* pCb, const uint8_t* pCr, uint32_t count) noexcept {
  uint32_t i = count;

  while (i >= 8) {
    SIMD::I128 yy = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pY));
    SIMD::I128 cb = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pCb));
    SIMD::I128 cr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pCr));

    yy = SIMD::vmovli64u8u16(yy);
    cb = SIMD::vaddi16(SIMD::vmovli64u8u16(cb), B2D_JPEG_CONST_XMM(ycbcr_tosigned));
    cr = SIMD::vaddi16(SIMD::vmovli64u8u16(cr), B2D_JPEG_CONST_XMM(ycbcr_tosigned));

    SIMD::I128 r_l = _mm_madd_epi16(_mm_unpacklo_epi16(yy, cr), B2D_JPEG_CONST_XMM(ycbcr_yycrMul));
    SIMD::I128 r_h = _mm_madd_epi16(_mm_unpackhi_epi16(yy, cr), B2D_JPEG_CONST_XMM(ycbcr_yycrMul));

    SIMD::I128 b_l = _mm_madd_epi16(_mm_unpacklo_epi16(yy, cb), B2D_JPEG_CONST_XMM(ycbcr_yycbMul));
    SIMD::I128 b_h = _mm_madd_epi16(_mm_unpackhi_epi16(yy, cb), B2D_JPEG_CONST_XMM(ycbcr_yycbMul));

    SIMD::I128 g_l = _mm_madd_epi16(_mm_unpacklo_epi16(cb, cr), B2D_JPEG_CONST_XMM(ycbcr_cbcrMul));
    SIMD::I128 g_h = _mm_madd_epi16(_mm_unpackhi_epi16(cb, cr), B2D_JPEG_CONST_XMM(ycbcr_cbcrMul));

    g_l = SIMD::vaddi32(g_l, SIMD::vslli32<B2D_JPEG_YCBCR_PREC>(SIMD::vmovli64u16u32(yy)));
    g_h = SIMD::vaddi32(g_h, SIMD::vslli32<B2D_JPEG_YCBCR_PREC>(SIMD::vmovhi64u16u32(yy)));

    r_l = SIMD::vaddi32(r_l, B2D_JPEG_CONST_XMM(ycbcr_round));
    r_h = SIMD::vaddi32(r_h, B2D_JPEG_CONST_XMM(ycbcr_round));

    b_l = SIMD::vaddi32(b_l, B2D_JPEG_CONST_XMM(ycbcr_round));
    b_h = SIMD::vaddi32(b_h, B2D_JPEG_CONST_XMM(ycbcr_round));

    g_l = SIMD::vaddi32(g_l, B2D_JPEG_CONST_XMM(ycbcr_round));
    g_h = SIMD::vaddi32(g_h, B2D_JPEG_CONST_XMM(ycbcr_round));

    r_l = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(r_l);
    r_h = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(r_h);

    b_l = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(b_l);
    b_h = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(b_h);

    g_l = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(g_l);
    g_h = SIMD::vsrai32<B2D_JPEG_YCBCR_PREC>(g_h);

    __m128i r = SIMD::vpackzzdb(r_l, r_h);
    __m128i g = SIMD::vpackzzdb(g_l, g_h);
    __m128i b = SIMD::vpackzzdb(b_l, b_h);

    __m128i ra = SIMD::vunpackli8(r, B2D_JPEG_CONST_XMM(ycbcr_allones));
    __m128i bg = SIMD::vunpackli8(b, g);

    __m128i bgra0 = SIMD::vunpackli16(bg, ra);
    __m128i bgra1 = SIMD::vunpackhi16(bg, ra);

    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst +  0), bgra0);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 16), bgra1);

    dst += 32;
    pY  += 8;
    pCb += 8;
    pCr += 8;
    i   -= 8;
  }

  while (i) {
    int yy = (int(pY[0]) << B2D_JPEG_YCBCR_PREC) + (1 << (B2D_JPEG_YCBCR_PREC - 1));
    int cr = int(pCr[0]) - 128;
    int cb = int(pCb[0]) - 128;

    int r = yy + cr * B2D_JPEG_YCBCR_FIXED(1.40200);
    int g = yy - cr * B2D_JPEG_YCBCR_FIXED(0.71414) - cb * B2D_JPEG_YCBCR_FIXED(0.34414);
    int b = yy + cb * B2D_JPEG_YCBCR_FIXED(1.77200);

    Support::writeU32a(dst, ArgbUtil::pack32(
      uint8_t(0xFF),
      Support::clampToByte(r >> B2D_JPEG_YCBCR_PREC),
      Support::clampToByte(g >> B2D_JPEG_YCBCR_PREC),
      Support::clampToByte(b >> B2D_JPEG_YCBCR_PREC)));

    dst += 4;
    pY  += 1;
    pCb += 1;
    pCr += 1;
    i   -= 1;
  }
}

} // JpegInternal namespace
B2D_END_NAMESPACE
