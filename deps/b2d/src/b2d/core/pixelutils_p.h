// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_PIXELUTILS_P_H
#define _B2D_CORE_PIXELUTILS_P_H

// [Dependencies]
#include "../core/argb.h"
#include "../pipe2d/tables_p.h"
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::PixelUtils]
// ============================================================================

//! Fast conversion between common pixel formats.
namespace PixelUtils {
  // --------------------------------------------------------------------------
  // [Conversion]
  // --------------------------------------------------------------------------

  static B2D_INLINE uint32_t xrgb32_0888_from_xrgb16_0555(uint32_t src) {
    uint32_t t0 = src;       // [00000000] [00000000] [XRRRRRGG] [GGGBBBBB]
    uint32_t t1;
    uint32_t t2;

    t0 = (t0 * 0x00080008u); // [RRRGGGGG] [BBBBBXRR] [RRRGGGGG] [BBBBB000]
    t0 = (t0 & 0x1F03E0F8u); // [000GGGGG] [000000RR] [RRR00000] [BBBBB000]
    t0 = (t0 | (t0 >> 5));   // [000GGGGG] [GGGGG0RR] [RRRRRRRR] [BBBBBBBB]

    t1 = t0 >> 13;           // [00000000] [00000000] [GGGGGGGG] [GG0RRRRR]
    t2 = t0 << 6;            // [GGGGGGG0] [RRRRRRRR] [RRBBBBBB] [BB000000]

    t0 &= 0x000000FFu;       // [00000000] [00000000] [00000000] [BBBBBBBB]
    t1 &= 0x0000FF00u;       // [00000000] [00000000] [GGGGGGGG] [00000000]
    t2 &= 0x00FF0000u;       // [00000000] [RRRRRRRR] [00000000] [00000000]

    return 0xFF000000u | t0 | t1 | t2;
  }

  static B2D_INLINE uint32_t xrgb32_0888_from_xrgb16_0565(uint32_t src) {
    uint32_t t0 = src;       // [00000000] [00000000] [RRRRRGGG] [GGGBBBBB]
    uint32_t t1 = src;
    uint32_t t2;

    t0 = (t0 & 0x0000F81Fu); // [00000000] [00000000] [RRRRR000] [000BBBBB]
    t1 = (t1 & 0x000007E0u); // [00000000] [00000000] [00000GGG] [GGG00000]

    t0 = (t0 * 0x21u);       // [00000000] [000RRRRR] [RRRRR0BB] [BBBBBBBB]
    t1 = (t1 * 0x41u);       // [00000000] [0000000G] [GGGGGGGG] [GGG00000]

    t2 = (t0 << 3);          // [00000000] [RRRRRRRR] [RR0BBBBB] [BBBBB000]
    t0 = (t0 >> 2);          // [00000000] [00000RRR] [RRRRRRR0] [BBBBBBBB]
    t1 = (t1 >> 1);          // [00000000] [00000000] [GGGGGGGG] [GGGG0000]

    t0 = t0 & 0x000000FFu;   // [00000000] [00000000] [00000000] [BBBBBBBB]
    t1 = t1 & 0x0000FF00u;   // [00000000] [00000000] [GGGGGGGG] [00000000]
    t2 = t2 & 0x00FF0000u;   // [00000000] [RRRRRRRR] [00000000] [00000000]

    return 0xFF000000u | t0 | t1 | t2;
  }

  static B2D_INLINE uint32_t argb32_8888_from_argb16_4444(uint32_t src) {
    uint32_t t0 = src;       // [00000000] [00000000] [AAAARRRR] [GGGGBBBB]
    uint32_t t1;
    uint32_t t2;

    t1 = t0 << 12;           // [0000AAAA] [RRRRGGGG] [BBBB0000] [00000000]
    t2 = t0 << 4;            // [00000000] [0000AAAA] [RRRRGGGG] [BBBB0000]

    t0 = t0 | t1;            // [0000AAAA] [RRRRGGGG] [XXXXRRRR] [GGGGBBBB]
    t1 = t2 << 4;            // [00000000] [AAAARRRR] [GGGGBBBB] [00000000]

    t0 &= 0x0F00000Fu;       // [0000AAAA] [00000000] [00000000] [0000BBBB]
    t1 &= 0x000F0000u;       // [00000000] [0000RRRR] [00000000] [00000000]
    t2 &= 0x00000F00u;       // [00000000] [00000000] [0000GGGG] [00000000]

    t0 += t1;                // [0000AAAA] [0000RRRR] [00000000] [0000BBBB]
    t0 += t2;                // [0000AAAA] [0000RRRR] [0000GGGG] [0000BBBB]

    return t0 * 0x11u;       // [AAAAAAAA] [RRRRRRRR] [GGGGGGGG] [BBBBBBBB]
  }

  // --------------------------------------------------------------------------
  // [Premultiply / Demultiply]
  // --------------------------------------------------------------------------

  static B2D_INLINE uint32_t prgb32_8888_from_argb32_8888(uint32_t val32) noexcept {
#if B2D_SIMD_I
    B2D_SIMD_DEF_I128_1xI64(argb64_alpha_mask, 0x00FF000000000000u);

    SIMD::I128 p0 = SIMD::vmovli64u8u16(SIMD::vcvtu32i128(val32));
    SIMD::I128 a0 = SIMD::vswizli16<3, 3, 3, 3>(p0);

    p0 = SIMD::vor(p0, argb64_alpha_mask.i128);
    p0 = SIMD::vdiv255u16(SIMD::vmuli16(p0, a0));
    p0 = SIMD::vpackzzwb(p0);
    return SIMD::vcvti128u32(p0);
#else
    return prgb32_8888_from_argb32_8888(val32, val32 >> 24);
#endif
  }

  static B2D_INLINE uint32_t prgb32_8888_from_argb32_8888(uint32_t val32, uint32_t _a) noexcept {
#if B2D_SIMD_I
    SIMD::I128 p0 = SIMD::vcvtu32i128(val32);
    SIMD::I128 a0 = SIMD::vcvtu32i128(_a | 0x00FF0000u);

    p0 = SIMD::vmovli64u8u16(p0);
    a0 = SIMD::vswizli16<1, 0, 0, 0>(a0);
    p0 = SIMD::vdiv255u16(SIMD::vmuli16(p0, a0));
    p0 = SIMD::vpackzzwb(p0);
    return SIMD::vcvti128u32(p0);
#else
    uint32_t rb = val32;
    uint32_t ag = val32;

    ag |= 0xFF000000u;
    ag >>= 8;

    rb = (rb & 0x00FF00FFu) * _a;
    ag = (ag & 0x00FF00FFu) * _a;

    rb += 0x00800080u;
    ag += 0x00800080u;

    rb = (rb + ((rb >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;
    ag = (ag + ((ag >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;

    rb >>= 8;
    return ag | rb;
#endif
  }

  static B2D_INLINE uint32_t argb32_8888_from_prgb32_8888(uint32_t val32) noexcept {
    uint32_t a, r, g, b;
    a = ((val32 >> 24));
    r = ((val32 >> 16) & 0xFFu);
    g = ((val32 >>  8) & 0xFFu);
    b = ((val32      ) & 0xFFu);

    uint32_t recip = Pipe2D::gConstants.div24bit[a];
    r = (r * recip) >> 16;
    g = (g * recip) >> 16;
    b = (b * recip) >> 16;

    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  static B2D_INLINE uint32_t abgr32_8888_from_prgb32_8888(uint32_t val32) noexcept {
    uint32_t a, r, g, b;
    a = ((val32 >> 24));
    r = ((val32 >> 16) & 0xFFu);
    g = ((val32 >>  8) & 0xFFu);
    b = ((val32      ) & 0xFFu);

    uint32_t recip = Pipe2D::gConstants.div24bit[a];
    r = (r * recip) >> 16;
    g = (g * recip) >> 16;
    b = (b * recip) >> 16;

    return (a << 24) | (b << 16) | (g << 8) | r;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_PIXELUTILS_P_H
