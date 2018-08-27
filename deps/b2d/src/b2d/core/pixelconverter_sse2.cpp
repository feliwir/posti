// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/pixelconverter.h"
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::PixelConverter - PRGB32 <- XRGB32 (SSE2)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromXRGB32_SSE2(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;
  I128 fillMask = vseti128i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 16) {
      I128 p0, p1, p2, p3;

      p0 = vloadi128u(srcData +  0);
      p1 = vloadi128u(srcData + 16);
      p2 = vloadi128u(srcData + 32);
      p3 = vloadi128u(srcData + 48);

      p0 = vor(p0, fillMask);
      p1 = vor(p1, fillMask);
      p2 = vor(p2, fillMask);
      p3 = vor(p3, fillMask);

      vstorei128u(dstData +  0, p0);
      vstorei128u(dstData + 16, p1);
      vstorei128u(dstData + 32, p2);
      vstorei128u(dstData + 48, p3);

      dstData += 64;
      srcData += 64;
      i -= 16;
    }

    while (i >= 4) {
      I128 p0;

      p0 = vloadi128u(srcData);
      p0 = vor(p0, fillMask);
      vstorei128u(dstData, p0);

      dstData += 16;
      srcData += 16;
      i -= 4;
    }

    while (i) {
      I128 p0;

      p0 = vloadi128_32(srcData);
      p0 = vor(p0, fillMask);
      vstorei32(dstData, p0);

      dstData += 4;
      srcData += 4;
      i--;
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
};

// ============================================================================
// [b2d::PixelConverter - PRGB32 <- ARGB32 (SSE2)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromARGB32_SSE2(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;
  I128 zero = vzeroi128();
  I128 a255 = vseti128i64(int64_t(0x00FF000000000000));
  I128 fillMask = vseti128i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 4) {
      I128 p0, p1;
      I128 a0, a1;

      p0 = vloadi128u(srcData);

      p1 = vunpackhi8(p0, zero);
      p0 = vunpackli8(p0, zero);

      a1 = vswizi16<3, 3, 3, 3>(p1);
      p1 = vor(p1, a255);

      a0 = vswizi16<3, 3, 3, 3>(p0);
      p0 = vor(p0, a255);

      p1 = vdiv255u16(vmuli16(p1, a1));
      p0 = vdiv255u16(vmuli16(p0, a0));
      p0 = vpacki16u8(p0, p1);
      p0 = vor(p0, fillMask);
      vstorei128u(dstData, p0);

      dstData += 16;
      srcData += 16;
      i -= 4;
    }

    while (i) {
      I128 p0;
      I128 a0;

      p0 = vloadi128_32(srcData);
      p0 = vunpackli8(p0, zero);
      a0 = vswizi16<3, 3, 3, 3>(p0);
      p0 = vor(p0, a255);
      p0 = vdiv255u16(vmuli16(p0, a0));
      p0 = vpacki16u8(p0, p0);
      p0 = vor(p0, fillMask);
      vstorei32(dstData, p0);

      dstData += 4;
      srcData += 4;
      i--;
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
};

// ============================================================================
// [b2d::PixelConverter - Init (SSE2)]
// ============================================================================

bool PixelConverter_initNativeFromXRGB_SSE2(PixelConverter* self, uint32_t pixelFormat) noexcept {
  const auto& d = self->data<PixelConverter::Data::NativeFromXRGB>();
  uint32_t layout = d.layout;
  uint32_t depth = layout & PixelConverter::kLayoutDepthMask;

  if (depth == 32) {
    // Only BYTE aligned components (8888 or X888 formats).
    if (!(layout & PixelConverter::_kLayoutIsByteAligned))
      return false;

    // Only PRGB32, ARGB32, or XRGB32 formats. See SSSE3 implementation, which
    // uses PSHUFB instruction and implements optimized conversion between all
    // possible byte-aligned formats.
    if (d.shift[1] != 16 || d.shift[2] != 8 || d.shift[3] != 0)
      return false;

    bool isARGB = d.shift[0] == 24;
    bool isPremultiplied = (layout & PixelConverter::kLayoutIsPremultiplied) != 0;

    switch (pixelFormat) {
      case PixelFormat::kXRGB32:
      case PixelFormat::kPRGB32:
        self->_convert = (isARGB && !isPremultiplied) ? PixelConverter_cvtPRGB32FromARGB32_SSE2
                                                      : PixelConverter_cvtPRGB32FromXRGB32_SSE2;
        return true;

      default:
        break;
    }
  }

  return false;
}

B2D_END_NAMESPACE
