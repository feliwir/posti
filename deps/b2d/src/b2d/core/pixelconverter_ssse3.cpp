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
// [b2d::PixelConverter - PRGB32 <- RGB24 (SSSE3)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtXRGB32FromRGB24_SSSE3(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 3);

  using namespace SIMD;
  I128 fillMask = vseti128i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);
  I128 predicate = vloadi128u(cvt->data<PixelConverter::Data::NativeFromXRGB>().simdData);

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 16) {
      I128 p0, p1, p2, p3;

      p0 = vloadi128u(srcData +  0);   // [x5|z4 y4 x4|z3 y3 x3 z2|y2 x2 z1 y1|x1 z0 y0 x0]
      p1 = vloadi128u(srcData + 16);   // [yA|xA|z9 y9|x9 z8 y8 x8|z7 y7 x7 z6|y6 x6 z5 y5]
      p3 = vloadi128u(srcData + 32);   // [zF yF xF zE|yE xE zD yD|xD zC yC xC|zB yB xB zA]

      p2 = vpalignr<8>(p3, p1);        // [-- -- -- --|zB yB xB zA|yA|xA|z9 y9|x9 z8 y8 x8]
      p1 = vpalignr<12>(p1, p0);       // [-- -- -- --|z7 y7 x7 z6|y6 x6 z5 y5|x5|z4 y4 x4]
      p3 = vsrli128b<4>(p3);           // [-- -- -- --|zF yF xF zE|yE xE zD yD|xD zC yC xC]

      p0 = vpshufb(p0, predicate);
      p1 = vpshufb(p1, predicate);
      p2 = vpshufb(p2, predicate);
      p3 = vpshufb(p3, predicate);

      p0 = vor(p0, fillMask);
      p1 = vor(p1, fillMask);
      p2 = vor(p2, fillMask);
      p3 = vor(p3, fillMask);

      vstorei128u(dstData +  0, p0);
      vstorei128u(dstData + 16, p1);
      vstorei128u(dstData + 32, p2);
      vstorei128u(dstData + 48, p3);

      dstData += 64;
      srcData += 48;
      i -= 16;
    }

    if (i >= 8) {
      I128 p0, p1;

      p0 = vloadi128u  (srcData +  0); // [x5|z4 y4 x4|z3 y3 x3 z2|y2 x2 z1 y1|x1 z0 y0 x0]
      p1 = vloadi128_64(srcData + 16); // [-- -- -- --|-- -- -- --|z7 y7 x7 z6|y6 x6 z5 y5]

      p1 = vpalignr<12>(p1, p0);       // [-- -- -- --|z7 y7 x7 z6|y6 x6 z5 y5|x5|z4 y4 x4]

      p0 = vpshufb(p0, predicate);
      p1 = vpshufb(p1, predicate);

      p0 = vor(p0, fillMask);
      p1 = vor(p1, fillMask);

      vstorei128u(dstData +  0, p0);
      vstorei128u(dstData + 16, p1);

      dstData += 32;
      srcData += 24;
      i -= 8;
    }

    if (i >= 4) {
      I128 p0, p1;

      p0 = vloadi128_64(srcData +  0); // [-- -- -- --|-- -- -- --|y2 x2 z1 y1|x1 z0 y0 x0]
      p1 = vloadi128_32(srcData +  8); // [-- -- -- --|-- -- -- --|-- -- -- --|z3 y3 x3 z2]

      p0 = vunpackli64(p0, p1);        // [-- -- -- --|z3 y3 x3 z2|y2 x2 z1 y1|x1 z0 y0 x0]
      p0 = vpshufb(p0, predicate);
      p0 = vor(p0, fillMask);

      vstorei128u(dstData +  0, p0);

      dstData += 16;
      srcData += 12;
      i -= 4;
    }

    while (i) {
      uint32_t yx = Support::readU16u(srcData + 0);
      uint32_t z  = Support::readU8(srcData + 2);

      I128 p0 = vcvtu32i128((z << 16) | yx);
      p0 = vpshufb(p0, predicate);
      p0 = vor(p0, fillMask);
      vstorei32(dstData, p0);

      dstData += 4;
      srcData += 3;
      i--;
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
};

// ============================================================================
// [b2d::PixelConverter - PRGB32 <- XRGB32 (SSSE3)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromXRGB32_SSSE3(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;
  I128 fillMask = vseti128i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);
  I128 predicate = vloadi128u(cvt->data<PixelConverter::Data::NativeFromXRGB>().simdData);

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 16) {
      I128 p0, p1, p2, p3;

      p0 = vloadi128u(srcData +  0);
      p1 = vloadi128u(srcData + 16);
      p2 = vloadi128u(srcData + 32);
      p3 = vloadi128u(srcData + 48);

      p0 = vpshufb(p0, predicate);
      p1 = vpshufb(p1, predicate);
      p2 = vpshufb(p2, predicate);
      p3 = vpshufb(p3, predicate);

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
      p0 = vpshufb(p0, predicate);
      p0 = vor(p0, fillMask);
      vstorei128u(dstData, p0);

      dstData += 16;
      srcData += 16;
      i -= 4;
    }

    while (i) {
      I128 p0;

      p0 = vloadi128_32(srcData);
      p0 = vpshufb(p0, predicate);
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
// [b2d::PixelConverter - PRGB32 <- ARGB32 (SSSE3)]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtPRGB32FromARGB32_SSSE3(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  dstStride -= (w * 4) + gap;
  srcStride -= (w * 4);

  using namespace SIMD;
  I128 zero = vzeroi128();
  I128 a255 = vseti128i64(int64_t(0x00FF000000000000));
  I128 fillMask = vseti128i32(cvt->data<PixelConverter::Data::NativeFromXRGB>().fillMask);
  I128 predicate = vloadi128u(cvt->data<PixelConverter::Data::NativeFromXRGB>().simdData);

  for (uint32_t y = h; y != 0; y--) {
    uint32_t i = w;

    while (i >= 4) {
      I128 p0, p1;
      I128 a0, a1;

      p0 = vloadi128u(srcData);
      p0 = vpshufb(p0, predicate);

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
      p0 = vpshufb(p0, predicate);
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
// [b2d::PixelConverter - Init (SSSE3)]
// ============================================================================

static B2D_INLINE uint32_t PixelConverter_makePshufbPredicate24(const PixelConverter::Data::NativeFromXRGB& d) noexcept {
  uint32_t aIndex = 0x80u;
  uint32_t rIndex = uint32_t(d.shift[1]) >> 3;
  uint32_t gIndex = uint32_t(d.shift[2]) >> 3;
  uint32_t bIndex = uint32_t(d.shift[3]) >> 3;

  return (aIndex << 24) | (rIndex << 16) | (gIndex << 8) | (bIndex << 0);
}

static B2D_INLINE uint32_t PixelConverter_makePshufbPredicate32(const PixelConverter::Data::NativeFromXRGB& d) noexcept {
  uint32_t aIndex = uint32_t(d.shift[0]) >> 3;
  uint32_t rIndex = uint32_t(d.shift[1]) >> 3;
  uint32_t gIndex = uint32_t(d.shift[2]) >> 3;
  uint32_t bIndex = uint32_t(d.shift[3]) >> 3;

  return (aIndex << 24) | (rIndex << 16) | (gIndex << 8) | (bIndex << 0);
}

bool PixelConverter_initNativeFromXRGB_SSSE3(PixelConverter* self, uint32_t pixelFormat) noexcept {
  auto& d = self->data<PixelConverter::Data::NativeFromXRGB>();

  uint32_t layout = d.layout;
  uint32_t depth = layout & PixelConverter::kLayoutDepthMask;

  if (depth == 24) {
    // Only BYTE aligned components (888 format).
    if (!(layout & PixelConverter::_kLayoutIsByteAligned))
      return false;

    // We expect RGB components in any order, but not alpha.
    if (d.masks[0] != 0)
      return false;

    switch (pixelFormat) {
      case PixelFormat::kXRGB32:
      case PixelFormat::kPRGB32:
        d.simdData[0] = PixelConverter_makePshufbPredicate24(d);
        d.simdData[1] = d.simdData[0] + 0x00030303u;
        d.simdData[2] = d.simdData[0] + 0x00060606u;
        d.simdData[3] = d.simdData[0] + 0x00090909u;

        self->_convert = PixelConverter_cvtXRGB32FromRGB24_SSSE3;
        return true;

      default:
        return false;
    }
  }
  else if (depth == 32) {
    // Only BYTE aligned components (8888 or X888 formats).
    if (!(layout & PixelConverter::_kLayoutIsByteAligned))
      return false;

    // This combination is provided by SSE2 converter and doesn't use PSHUFB.
    // It's better on machines where PSHUFB is slow like ATOMs.
    if (d.shift[1] == 16 && d.shift[2] == 8 && d.shift[3] == 0)
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

        self->_convert = (isARGB && !isPremultiplied) ? PixelConverter_cvtPRGB32FromARGB32_SSSE3
                                                      : PixelConverter_cvtPRGB32FromXRGB32_SSSE3;
        return true;

      default:
        return false;
    }
  }

  return false;
}


B2D_END_NAMESPACE
