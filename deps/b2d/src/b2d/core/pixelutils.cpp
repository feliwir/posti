// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/pixelutils_p.h"
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::PixelUtils - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_pixelutils_xrgb32_0888_from_xrgb16_0555) {
  uint32_t c = 0;

  for (;;) {
    uint32_t r = ((c >> 10) & 0x1F) << 3;
    uint32_t g = ((c >>  5) & 0x1F) << 3;
    uint32_t b = ((c      ) & 0x1F) << 3;

    uint32_t result   = PixelUtils::xrgb32_0888_from_xrgb16_0555(c);
    uint32_t expected = ArgbUtil::pack32(0xFF, r + (r >> 5), g + (g >> 5), b + (b >> 5));

    EXPECT(result == expected,
      "xrgb32_0888_from_xrgb16_0555() - %08X -> %08X (Expected %08X)", c, result, expected);

    if (c == 0xFFFF)
      break;
    c++;
  }
}

UNIT(b2d_core_pixelutils_xrgb32_0888_from_xrgb16_0565) {
  uint32_t c = 0;

  for (;;) {
    uint32_t r = ((c >> 11) & 0x1F) << 3;
    uint32_t g = ((c >>  5) & 0x3F) << 2;
    uint32_t b = ((c      ) & 0x1F) << 3;

    uint32_t result   = PixelUtils::xrgb32_0888_from_xrgb16_0565(c);
    uint32_t expected = ArgbUtil::pack32(0xFF, r + (r >> 5), g + (g >> 6), b + (b >> 5));

    EXPECT(result == expected,
      "xrgb32_0888_from_xrgb16_0555() - %08X -> %08X (Expected %08X)", c, result, expected);

    if (c == 0xFFFF)
      break;
    c++;
  }
}

UNIT(b2d_core_pixelutils_argb32_8888_from_argb16_4444) {
  uint32_t c = 0;

  for (;;) {
    uint32_t a = ((c >> 12) & 0xF) * 0x11;
    uint32_t r = ((c >>  8) & 0xF) * 0x11;
    uint32_t g = ((c >>  4) & 0xF) * 0x11;
    uint32_t b = ((c      ) & 0xF) * 0x11;

    uint32_t result   = PixelUtils::argb32_8888_from_argb16_4444(c);
    uint32_t expected = ArgbUtil::pack32(a, r, g, b);

    EXPECT(result == expected,
      "argb32_8888_from_argb16_4444() - %08X -> %08X (Expected %08X)", c, result, expected);

    if (c == 0xFFFF)
      break;
    c++;
  }
}

UNIT(b2d_core_pixelutils_prgb32_8888_from_argb32_8888) {
  uint32_t i;
  uint32_t c = 0;

  for (i = 0; i < 10000000; i++) {
    uint32_t a = (c >> 24) & 0xFF;
    uint32_t r = (c >> 16) & 0xFF;
    uint32_t g = (c >>  8) & 0xFF;
    uint32_t b = (c      ) & 0xFF;

    uint32_t result = PixelUtils::prgb32_8888_from_argb32_8888(c);
    uint32_t expected = ArgbUtil::pack32(a,
      Support::udiv255(r * a),
      Support::udiv255(g * a),
      Support::udiv255(b * a));

    EXPECT(result == expected,
      "prgb32_8888_from_argb32_8888() - %08X -> %08X (Expected %08X)", c, result, expected);

    c += 7919;
  }
}
#endif

B2D_END_NAMESPACE
