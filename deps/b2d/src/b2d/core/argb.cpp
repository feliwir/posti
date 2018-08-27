// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/argb.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::ArgbUtil - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_argb_pack) {
  uint32_t a, r, g, b;

  EXPECT(ArgbUtil::pack32(0x01, 0x02, 0x03, 0x04) == 0x01020304);
  EXPECT(ArgbUtil::pack32(0xF1, 0xF2, 0xF3, 0xF4) == 0xF1F2F3F4);

  ArgbUtil::unpack32(0x01020304, a, r, g, b);
  EXPECT(a == 0x01 && r == 0x02 && g == 0x03 && b == 0x04);

  ArgbUtil::unpack32(0xF1E2D3C4, a, r, g, b);
  EXPECT(a == 0xF1 && r == 0xE2 && g == 0xD3 && b == 0xC4);

  EXPECT(ArgbUtil::pack64(0x0001, 0x0002, 0x0003, 0x0004) == 0x0001000200030004u);
  EXPECT(ArgbUtil::pack64(0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4) == 0xFFF1FFF2FFF3FFF4u);

  ArgbUtil::unpack64(0x0001000200030004u, a, r, g, b);
  EXPECT(a == 0x0001 && r == 0x0002 && g == 0x0003 && b == 0x0004);

  ArgbUtil::unpack64(0xFFF1EEE2DDD3CCC4u, a, r, g, b);
  EXPECT(a == 0xFFF1 && r == 0xEEE2 && g == 0xDDD3 && b == 0xCCC4);
}

UNIT(b2d_core_argb_opaque) {
  EXPECT( ArgbUtil::isOpaque32(0xFF000000u));
  EXPECT( ArgbUtil::isOpaque32(0xFF0000FFu));
  EXPECT( ArgbUtil::isOpaque32(0xFFFFFFFFu));

  EXPECT(!ArgbUtil::isOpaque32(0xFEFEFEFEu));
  EXPECT(!ArgbUtil::isOpaque32(0xFE000000u));
  EXPECT(!ArgbUtil::isOpaque32(0x80808080u));
  EXPECT(!ArgbUtil::isOpaque32(0x00000000u));

  EXPECT( ArgbUtil::isTransparent32(0x00000000u));
  EXPECT( ArgbUtil::isTransparent32(0x00000001u));
  EXPECT( ArgbUtil::isTransparent32(0x00FFFFFFu));

  EXPECT( ArgbUtil::isOpaque64(0xFFFF000000000000u));
  EXPECT( ArgbUtil::isOpaque64(0xFFFF00000000FFFFu));
  EXPECT( ArgbUtil::isOpaque64(0xFFFFFFFFFFFFFFFFu));

  EXPECT(!ArgbUtil::isOpaque64(0xFFFEFFFEFFFEFFFEu));
  EXPECT(!ArgbUtil::isOpaque64(0xFFFE000000000000u));
  EXPECT(!ArgbUtil::isOpaque64(0x8000800080008000u));
  EXPECT(!ArgbUtil::isOpaque64(0x0000000000000000u));

  EXPECT( ArgbUtil::isTransparent64(0x0000000000000000u));
  EXPECT( ArgbUtil::isTransparent64(0x0000000000000001u));
  EXPECT( ArgbUtil::isTransparent64(0x0000FFFFFFFFFFFFu));
}

UNIT(b2d_core_argb_convert) {
  EXPECT(ArgbUtil::convert64From32(0x01020304u) == 0x0101020203030404u);
  EXPECT(ArgbUtil::convert64From32(0x01FF03FFu) == 0x0101FFFF0303FFFFu);
  EXPECT(ArgbUtil::convert64From32(0xFF02FF04u) == 0xFFFF0202FFFF0404u);
  EXPECT(ArgbUtil::convert64From32(0xFFFFFFFFu) == 0xFFFFFFFFFFFFFFFFu);

  EXPECT(ArgbUtil::convert32From64(0xFF000100EE000200u) == 0xFF01EE02u);
  EXPECT(ArgbUtil::convert32From64(0x8000800080008000u) == 0x80808080u);
  EXPECT(ArgbUtil::convert32From64(0xF000E000D000C000u) == 0xF0E0D0C0u);
  EXPECT(ArgbUtil::convert32From64(0xF100E100D100C100u) == 0xF1E1D1C1u);
  EXPECT(ArgbUtil::convert32From64(0xFF00FF00FF00FF00u) == 0xFFFFFFFFu);
  EXPECT(ArgbUtil::convert32From64(0xFFFFFFFFFFFFFFFFu) == 0xFFFFFFFFu);
}

#endif

B2D_END_NAMESPACE
