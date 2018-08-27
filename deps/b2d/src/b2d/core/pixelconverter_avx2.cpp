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
// [b2d::PixelConverter - PRGB32 <- XRGB32 (AVX2)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromXRGB32_AVX2(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;
  I256 fillMask = vseti256i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);
  I256 predicate = vdupli128(vloadi128u(cvt->data<PixelConverter::Data::NativeFromXRGB>().simdData));

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 32) {
      I256 p0, p1, p2, p3;

      p0 = vloadi256u(srcData +  0);
      p1 = vloadi256u(srcData + 32);
      p2 = vloadi256u(srcData + 64);
      p3 = vloadi256u(srcData + 96);

      p0 = vpshufb(p0, predicate);
      p1 = vpshufb(p1, predicate);
      p2 = vpshufb(p2, predicate);
      p3 = vpshufb(p3, predicate);

      p0 = vor(p0, fillMask);
      p1 = vor(p1, fillMask);
      p2 = vor(p2, fillMask);
      p3 = vor(p3, fillMask);

      vstorei256u(dstData +  0, p0);
      vstorei256u(dstData + 32, p1);
      vstorei256u(dstData + 64, p2);
      vstorei256u(dstData + 96, p3);

      dstData += 128;
      srcData += 128;
      i -= 32;
    }

    while (i >= 8) {
      I256 p0;

      p0 = vloadi256u(srcData);
      p0 = vpshufb(p0, predicate);
      p0 = vor(p0, fillMask);
      vstorei256u(dstData, p0);

      dstData += 32;
      srcData += 32;
      i -= 8;
    }

    while (i) {
      I128 p0;

      p0 = vloadi128_32(srcData);
      p0 = vpshufb(p0, vcast<I128>(predicate));
      p0 = vor(p0, vcast<I128>(fillMask));
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
// [b2d::PixelConverter - PRGB32 <- ARGB32 (AVX2)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromARGB32_AVX2(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;

  I256 a255 = vseti256i64(int64_t(0x00FF000000000000));
  I256 fillMask = vseti256i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);
  I256 predicate = vdupli128(vloadi128u(cvt->data<PixelConverter::Data::NativeFromXRGB>().simdData));

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 16) {
      I256 p0, p1, p2, p3;

      p0 = vloadi256u(srcData +  0);
      p2 = vloadi256u(srcData + 32);

      I256 zero = vzeroi256();
      p0 = vpshufb(p0, predicate);
      p2 = vpshufb(p2, predicate);

      p1 = vunpackhi8(p0, zero);
      p0 = vunpackli8(p0, zero);
      p3 = vunpackhi8(p2, zero);
      p2 = vunpackli8(p2, zero);

      p0 = vmuli16(vor(p0, a255), vswizi16<3, 3, 3, 3>(p0));
      p1 = vmuli16(vor(p1, a255), vswizi16<3, 3, 3, 3>(p1));
      p2 = vmuli16(vor(p2, a255), vswizi16<3, 3, 3, 3>(p2));
      p3 = vmuli16(vor(p3, a255), vswizi16<3, 3, 3, 3>(p3));

      p0 = vdiv255u16(p0);
      p1 = vdiv255u16(p1);
      p2 = vdiv255u16(p2);
      p3 = vdiv255u16(p3);

      p0 = vpacki16u8(p0, p1);
      p2 = vpacki16u8(p2, p3);

      p0 = vor(p0, fillMask);
      p2 = vor(p2, fillMask);

      vstorei256u(dstData +  0, p0);
      vstorei256u(dstData + 32, p2);

      dstData += 64;
      srcData += 64;
      i -= 16;
    }

    while (i >= 4) {
      I128 p0, p1;
      I128 a0, a1;

      p0 = vloadi128u(srcData);
      I128 zero = vzeroi128();
      p0 = vpshufb(p0, vcast<I128>(predicate));

      p1 = vunpackhi8(p0, zero);
      p0 = vunpackli8(p0, zero);

      a1 = vswizi16<3, 3, 3, 3>(p1);
      p1 = vor(p1, vcast<I128>(a255));

      a0 = vswizi16<3, 3, 3, 3>(p0);
      p0 = vor(p0, vcast<I128>(a255));

      p1 = vdiv255u16(vmuli16(p1, a1));
      p0 = vdiv255u16(vmuli16(p0, a0));
      p0 = vpacki16u8(p0, p1);
      p0 = vor(p0, vcast<I128>(fillMask));
      vstorei128u(dstData, p0);

      dstData += 16;
      srcData += 16;
      i -= 4;
    }

    while (i) {
      I128 p0;
      I128 a0;

      p0 = vloadi128_32(srcData);
      I128 zero = vzeroi128();

      p0 = vpshufb(p0, vcast<I128>(predicate));
      p0 = vunpackli8(p0, zero);
      a0 = vswizi16<3, 3, 3, 3>(p0);
      p0 = vor(p0, vcast<I128>(a255));
      p0 = vdiv255u16(vmuli16(p0, a0));
      p0 = vpacki16u8(p0, p0);
      p0 = vor(p0, vcast<I128>(fillMask));
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
// [b2d::PixelConverter - Init (AVX2)]
// ============================================================================

static B2D_INLINE uint32_t PixelConverter_makePshufbPredicate32(const PixelConverter::Data::NativeFromXRGB& d) noexcept {
  uint32_t aIndex = uint32_t(d.shift[0]) >> 3;
  uint32_t rIndex = uint32_t(d.shift[1]) >> 3;
  uint32_t gIndex = uint32_t(d.shift[2]) >> 3;
  uint32_t bIndex = uint32_t(d.shift[3]) >> 3;

  return (aIndex << 24) | (rIndex << 16) | (gIndex << 8) | (bIndex << 0);
}

bool PixelConverter_initNativeFromXRGB_AVX2(PixelConverter* self, uint32_t pixelFormat) noexcept {
  auto& d = self->data<PixelConverter::Data::NativeFromXRGB>();

  uint32_t layout = d.layout;
  uint32_t depth = layout & PixelConverter::kLayoutDepthMask;

  if (depth == 32) {
    // Only BYTE aligned components (8888 or X888 formats).
    if (!(layout & PixelConverter::_kLayoutIsByteAligned))
      return false;

    bool isARGB = d.shift[0] == 24;
    bool isPremultiplied = (layout & PixelConverter::kLayoutIsPremultiplied) != 0;

    switch (pixelFormat) {
      case PixelFormat::kXRGB32:
      case PixelFormat::kPRGB32:
        d.simdData[0] = PixelConverter_makePshufbPredicate32(d);
        d.simdData[1] = d.simdData[0] + 0x04040404u;
        d.simdData[2] = d.simdData[0] + 0x08080808u;
        d.simdData[3] = d.simdData[0] + 0x0C0C0C0Cu;

        self->_convert = (isARGB && !isPremultiplied) ? PixelConverter_cvtPRGB32FromARGB32_AVX2
                                                      : PixelConverter_cvtPRGB32FromXRGB32_AVX2;
        return true;

      default:
        return false;
    }
  }

  return false;
}

B2D_END_NAMESPACE
