// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/unicodeio_p.h"

B2D_BEGIN_SUB_NAMESPACE(Unicode)

// ============================================================================
// [b2d::Unicode - IO - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_unicode_io) {
  INFO("Unicode::Utf8Reader");
  {
    const uint8_t data[] = {
      0xE2, 0x82, 0xAC,      // U+0020AC
      0xF0, 0x90, 0x8D, 0x88 // U+010348
    };

    Utf8Reader it(data, B2D_ARRAY_SIZE(data));
    uint32_t uc;

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x0020AC);

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x010348);

    EXPECT(!it.hasNext());

    // Verify that sizes were calculated correctly.
    EXPECT(it.byteIndex(data) == 7);
    EXPECT(it.utf8Index(data) == 7);
    EXPECT(it.utf16Index(data) == 3); // 3 code-points (1 BMP and 1 SMP).
    EXPECT(it.utf32Index(data) == 2); // 2 code-points.

    const uint8_t invalidData[] = { 0xE2, 0x82 };
    it.reset(invalidData, B2D_ARRAY_SIZE(invalidData));

    EXPECT(it.hasNext());
    EXPECT(it.next(uc) == kErrorTruncatedString);

    // After error the iterator should not move.
    EXPECT(it.hasNext());
    EXPECT(it.byteIndex(invalidData) == 0);
    EXPECT(it.utf8Index(invalidData) == 0);
    EXPECT(it.utf16Index(invalidData) == 0);
    EXPECT(it.utf32Index(invalidData) == 0);
  }

  INFO("Unicode::Utf16Reader");
  {
    const uint16_t data[] = {
      0x20AC,                // U+0020AC
      0xD800, 0xDF48         // U+010348
    };

    Utf16Reader it(data, B2D_ARRAY_SIZE(data) * sizeof(uint16_t));
    uint32_t uc;

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x0020AC);

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x010348);

    EXPECT(!it.hasNext());

    // Verify that sizes were calculated correctly.
    EXPECT(it.byteIndex(data) == 6);
    EXPECT(it.utf8Index(data) == 7);
    EXPECT(it.utf16Index(data) == 3); // 3 code-points (1 BMP and 1 SMP).
    EXPECT(it.utf32Index(data) == 2); // 2 code-points.

    const uint16_t invalidData[] = { 0xD800 };
    it.reset(invalidData, B2D_ARRAY_SIZE(invalidData) * sizeof(uint16_t));

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex | kReadStrict>(uc) == kErrorTruncatedString);

    // After an error the iterator should not move.
    EXPECT(it.hasNext());
    EXPECT(it.byteIndex(invalidData) == 0);
    EXPECT(it.utf8Index(invalidData) == 0);
    EXPECT(it.utf16Index(invalidData) == 0);
    EXPECT(it.utf32Index(invalidData) == 0);

    // However, this should pass in non-strict mode.
    EXPECT(it.next(uc) == kErrorOk);
    EXPECT(!it.hasNext());
  }

  INFO("Unicode::Utf32Reader");
  {
    const uint32_t data[] = {
      0x0020AC,
      0x010348
    };

    Utf32Reader it(data, B2D_ARRAY_SIZE(data) * sizeof(uint32_t));
    uint32_t uc;

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x0020AC);

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex>(uc) == kErrorOk);
    EXPECT(uc == 0x010348);

    EXPECT(!it.hasNext());

    // Verify that sizes were calculated correctly.
    EXPECT(it.byteIndex(data) == 8);
    EXPECT(it.utf8Index(data) == 7);
    EXPECT(it.utf16Index(data) == 3); // 3 code-points (1 BMP and 1 SMP).
    EXPECT(it.utf32Index(data) == 2); // 2 code-points.

    const uint32_t invalidData[] = { 0xD800 };
    it.reset(invalidData, B2D_ARRAY_SIZE(invalidData) * sizeof(uint32_t));

    EXPECT(it.hasNext());
    EXPECT(it.next<kReadCalcUtfIndex | kReadStrict>(uc) == kErrorInvalidString);

    // After an error the iterator should not move.
    EXPECT(it.hasNext());
    EXPECT(it.byteIndex(invalidData) == 0);
    EXPECT(it.utf8Index(invalidData) == 0);
    EXPECT(it.utf16Index(invalidData) == 0);
    EXPECT(it.utf32Index(invalidData) == 0);

    // However, this should pass in non-strict mode.
    EXPECT(it.next(uc) == kErrorOk);
    EXPECT(!it.hasNext());
  }

  INFO("Unicode::Utf8Writer");
  {
    char dst[7];
    Utf8Writer writer(dst, B2D_ARRAY_SIZE(dst));

    EXPECT(writer.write(0x20ACu) == kErrorOk);
    EXPECT(uint8_t(dst[0]) == 0xE2);
    EXPECT(uint8_t(dst[1]) == 0x82);
    EXPECT(uint8_t(dst[2]) == 0xAC);

    EXPECT(writer.write(0x010348u) == kErrorOk);
    EXPECT(uint8_t(dst[3]) == 0xF0);
    EXPECT(uint8_t(dst[4]) == 0x90);
    EXPECT(uint8_t(dst[5]) == 0x8D);
    EXPECT(uint8_t(dst[6]) == 0x88);
    EXPECT(writer.atEnd());

    writer.reset(dst, 1);
    EXPECT(writer.write(0x20ACu) == kErrorStringNoSpaceLeft);
    EXPECT(writer.write(0x0080u) == kErrorStringNoSpaceLeft);
    EXPECT(writer.write(0x00C1u) == kErrorStringNoSpaceLeft);

    // We have only one byte left so this must pass...
    EXPECT(writer.write('a') == kErrorOk);
    EXPECT(writer.atEnd());

    writer.reset(dst, 2);
    EXPECT(writer.write(0x20ACu) == kErrorStringNoSpaceLeft);
    EXPECT(writer.write(0x00C1u) == kErrorOk);
    EXPECT(dst[0] == char(0xC3));
    EXPECT(dst[1] == char(0x81));
    EXPECT(writer.atEnd());
    EXPECT(writer.write('a') == kErrorStringNoSpaceLeft);
  }

  INFO("Unicode::Utf16Writer");
  {
    uint16_t dst[3];
    Utf16Writer<> writer(dst, B2D_ARRAY_SIZE(dst));

    EXPECT(writer.write(0x010348u) == kErrorOk);
    EXPECT(dst[0] == 0xD800u);
    EXPECT(dst[1] == 0xDF48u);

    EXPECT(writer.write(0x010348u) == kErrorStringNoSpaceLeft);
    EXPECT(writer.write(0x20ACu) == kErrorOk);
    EXPECT(dst[2] == 0x20ACu);
    EXPECT(writer.atEnd());
  }
}
#endif

B2D_END_SUB_NAMESPACE
