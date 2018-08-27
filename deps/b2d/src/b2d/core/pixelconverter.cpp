// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/image.h"
#include "../core/pixelconverter.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../pipe2d/tables_p.h"

B2D_BEGIN_NAMESPACE

#ifdef B2D_MAYBE_SSE2
bool PixelConverter_initNativeFromXRGB_SSE2(PixelConverter* self, uint32_t pixelFormat) noexcept;
#endif

#ifdef B2D_MAYBE_SSSE3
bool PixelConverter_initNativeFromXRGB_SSSE3(PixelConverter* self, uint32_t pixelFormat) noexcept;
#endif

#ifdef B2D_MAYBE_AVX2
bool PixelConverter_initNativeFromXRGB_AVX2(PixelConverter* self, uint32_t pixelFormat) noexcept;
#endif

// ============================================================================
// [Helpers]
// ============================================================================

// Please note that the blue component has to be shifted right by 8 bits to be
// at the right place, because there is no way to calculate the constants of
// component that has to stay within the first 8 bits - this is because the
// `scale` value always powers, so it creates 10 bits out of 5, 9 bits out of
// 3, etc... This means that it can't cover all possibilities if it has to stay
// at `0x000000FF` only.
static constexpr uint32_t PixelConverter_native32FromXRGBMask[] = {
  0xFF000000u, // A.
  0x00FF0000u, // R.
  0x0000FF00u, // G.
  0x0000FF00u  // B (shift to right by 8 to get the desired result).
};

static B2D_INLINE bool PixelConverter_isMaskByteAligned(uint32_t mask) noexcept {
  return mask == 0 || (mask >> (Support::ctz(mask) & ~0x7u)) == 0xFFu;
}

// ============================================================================
// [b2d::PixelAccess...]
// ============================================================================

#define B2D_PIXEL_FETCH(Fn, P)    BO == Globals::kByteOrderLE ? Fn##LE(P)    : Fn##BE(P)
#define B2D_PIXEL_STORE(Fn, P, V) BO == Globals::kByteOrderLE ? Fn##LE(P, V) : Fn##BE(P, V)

template<unsigned int BO>
struct PixelAccess16 {
  enum : uint32_t { kSize = 2 };

  static B2D_INLINE uint32_t fetchA(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU16a, p); }
  static B2D_INLINE uint32_t fetchU(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU16u, p); }

  static B2D_INLINE void storeA(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU16a, p, uint16_t(v)); }
  static B2D_INLINE void storeU(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU16u, p, uint16_t(v)); }
};

template<unsigned int BO>
struct PixelAccess24 {
  enum : uint32_t { kSize = 3 };

  static B2D_INLINE uint32_t fetchA(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU24u, p); }
  static B2D_INLINE uint32_t fetchU(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU24u, p); }

  static B2D_INLINE void storeA(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU24u, p, v); }
  static B2D_INLINE void storeU(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU24u, p, v); }
};

template<unsigned int BO>
struct PixelAccess32 {
  enum : uint32_t { kSize = 4 };

  static B2D_INLINE uint32_t fetchA(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU32a, p); }
  static B2D_INLINE uint32_t fetchU(const void* p) noexcept { return B2D_PIXEL_FETCH(Support::readU32u, p); }

  static B2D_INLINE void storeA(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU32a, p, v); }
  static B2D_INLINE void storeU(void* p, uint32_t v) noexcept { B2D_PIXEL_STORE(Support::writeU32u, p, v); }
};

#undef B2D_PIXEL_STORE
#undef B2D_PIXEL_FETCH

// ============================================================================
// [b2d::PixelConverter - Native <- Pal]
// ============================================================================

static void B2D_CDECL PixelConverter_cvtNative32FromPal1(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcLine, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  // Instead of doing a table lookup each time we create a XOR mask that is
  // used to get the second color value from the first one. This allows to
  // remove the lookup completely. The only requirement is that we need all
  // zeros or ones depending on the source value (see the implementation, it
  // uses signed right shift to fill these bits in).
  uint32_t c0 = cvt->data<PixelConverter::Data::NativeFromIndexed>().pal[0];
  uint32_t cm = cvt->data<PixelConverter::Data::NativeFromIndexed>().pal[1] ^ c0;

  dstStride -= w * 4 + gap;

  if (c0 == 0x00000000u && cm == 0xFFFFFFFFu) {
    // Special case for black/white palette, quite common.
    for (uint32_t y = h; y != 0; y--) {
      const uint8_t* srcData = srcLine;

      uint32_t i = w;
      while (i >= 8) {
        uint32_t b0 = uint32_t(*srcData++) << 24;
        uint32_t b1 = b0 << 1;

        Support::writeU32a(dstData +  0, Support::sar(b0, 31)); b0 <<= 2;
        Support::writeU32a(dstData +  4, Support::sar(b1, 31)); b1 <<= 2;
        Support::writeU32a(dstData +  8, Support::sar(b0, 31)); b0 <<= 2;
        Support::writeU32a(dstData + 12, Support::sar(b1, 31)); b1 <<= 2;
        Support::writeU32a(dstData + 16, Support::sar(b0, 31)); b0 <<= 2;
        Support::writeU32a(dstData + 20, Support::sar(b1, 31)); b1 <<= 2;
        Support::writeU32a(dstData + 24, Support::sar(b0, 31));
        Support::writeU32a(dstData + 28, Support::sar(b1, 31));

        dstData += 32;
        i -= 8;
      }

      if (i) {
        uint32_t b0 = uint32_t(*srcData++) << 24;
        do {
          Support::writeU32a(dstData, Support::sar(b0, 31));

          dstData += 4;
          b0 <<= 1;
        } while (--i);
      }

      dstData = PixelConverter::_fillGap(dstData, gap);
      dstData += dstStride;
      srcLine += srcStride;
    }
  }
  else {
    // Generic case for any other combination.
    for (uint32_t y = h; y != 0; y--) {
      const uint8_t* srcData = srcLine;

      uint32_t i = w;
      while (i >= 8) {
        uint32_t b0 = uint32_t(*srcData++) << 24;
        uint32_t b1 = b0 << 1;

        Support::writeU32a(dstData +  0, c0 ^ (cm & Support::sar(b0, 31))); b0 <<= 2;
        Support::writeU32a(dstData +  4, c0 ^ (cm & Support::sar(b1, 31))); b1 <<= 2;
        Support::writeU32a(dstData +  8, c0 ^ (cm & Support::sar(b0, 31))); b0 <<= 2;
        Support::writeU32a(dstData + 12, c0 ^ (cm & Support::sar(b1, 31))); b1 <<= 2;
        Support::writeU32a(dstData + 16, c0 ^ (cm & Support::sar(b0, 31))); b0 <<= 2;
        Support::writeU32a(dstData + 20, c0 ^ (cm & Support::sar(b1, 31))); b1 <<= 2;
        Support::writeU32a(dstData + 24, c0 ^ (cm & Support::sar(b0, 31)));
        Support::writeU32a(dstData + 28, c0 ^ (cm & Support::sar(b1, 31)));

        dstData += 32;
        i -= 8;
      }

      if (i) {
        uint32_t b0 = uint32_t(*srcData++) << 24;
        do {
          Support::writeU32a(dstData, c0 ^ (cm & Support::sar(b0, 31)));

          dstData += 4;
          b0 <<= 1;
        } while (--i);
      }

      dstData = PixelConverter::_fillGap(dstData, gap);
      dstData += dstStride;
      srcLine += srcStride;
    }
  }
}

static void B2D_CDECL PixelConverter_cvtNative32FromPal2(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcLine, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const uint32_t* pal = cvt->data<PixelConverter::Data::NativeFromIndexed>().pal;

  dstStride -= w * 4 + gap;

  for (uint32_t y = h; y != 0; y--) {
    const uint8_t* srcData = srcLine;

    uint32_t i = w;
    while (i >= 4) {
      uint32_t b0 = uint32_t(*srcData++) << 24;

      Support::writeU32a(dstData +  0, pal[b0 >> 30]); b0 <<= 2;
      Support::writeU32a(dstData +  4, pal[b0 >> 30]); b0 <<= 2;
      Support::writeU32a(dstData +  8, pal[b0 >> 30]); b0 <<= 2;
      Support::writeU32a(dstData + 12, pal[b0 >> 30]);

      dstData += 16;
      i -= 4;
    }

    if (i) {
      uint32_t b0 = uint32_t(*srcData++) << 24;
      do {
        Support::writeU32a(dstData, pal[b0 >> 30]);

        dstData += 4;
        b0 <<= 2;
      } while (--i);
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcLine += srcStride;
  }
}

static void B2D_CDECL PixelConverter_cvtNative32FromPal4(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcLine, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const uint32_t* pal = cvt->data<PixelConverter::Data::NativeFromIndexed>().pal;

  dstStride -= w * 4 + gap;

  for (uint32_t y = h; y != 0; y--) {
    const uint8_t* srcData = srcLine;

    uint32_t i = w;
    while (i >= 2) {
      uint32_t b0 = *srcData++;

      Support::writeU32a(dstData + 0, pal[b0 >> 4]);
      Support::writeU32a(dstData + 4, pal[b0 & 15]);

      dstData += 8;
      i -= 2;
    }

    if (i) {
      uint32_t b0 = srcData[0];
      Support::writeU32a(dstData, pal[b0 >> 4]);
      dstData += 4;
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcLine += srcStride;
  }
}

static void B2D_CDECL PixelConverter_cvtNative32FromPal8(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcLine, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const uint32_t* pal = cvt->data<PixelConverter::Data::NativeFromIndexed>().pal;
  for (uint32_t y = h; y != 0; y--) {
    const uint8_t* srcData = srcLine;

    for (uint32_t i = w; i != 0; i--) {
      uint32_t b0 = *srcData++;
      Support::writeU32a(dstData, pal[b0]);
      dstData += 4;
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcLine += srcStride;
  }
}

// ============================================================================
// [b2d::PixelConverter - Native <- XRGB|ARGB|PRGB]
// ============================================================================

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtNative32FromXRGBAny(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::NativeFromXRGB>();

  dstStride -= w * 4 + gap;
  srcStride -= w * PixelAccess::kSize;

  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  uint32_t rScale = d.scale[1];
  uint32_t gScale = d.scale[2];
  uint32_t bScale = d.scale[3];

  uint32_t fillMask = d.fillMask;

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(srcData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchA(srcData);

        uint32_t r = (((pix >> rShift) & rMask) * rScale) & 0x00FF0000u;
        uint32_t g = (((pix >> gShift) & gMask) * gScale) & 0x0000FF00u;
        uint32_t b = (((pix >> bShift) & bMask) * bScale) >> 8;

        Support::writeU32a(dstData, r | g | b | fillMask);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchU(srcData);

        uint32_t r = (((pix >> rShift) & rMask) * rScale) & 0x00FF0000u;
        uint32_t g = (((pix >> gShift) & gMask) * gScale) & 0x0000FF00u;
        uint32_t b = (((pix >> bShift) & bMask) * bScale) >> 8;

        Support::writeU32a(dstData, r | g | b | fillMask);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtNative32FromARGBAny(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::NativeFromXRGB>();

  dstStride -= w * 4 + gap;
  srcStride -= w * PixelAccess::kSize;

  uint32_t aMask = d.masks[0];
  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t aShift = d.shift[0];
  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  uint32_t aScale = d.scale[0];
  uint32_t rScale = d.scale[1];
  uint32_t gScale = d.scale[2];
  uint32_t bScale = d.scale[3];

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(srcData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchA(srcData);

        uint32_t _a = ((((pix >> aShift) & aMask) * aScale) >> 24);
        uint32_t ag = ((((pix >> gShift) & gMask) * gScale) >>  8);
        uint32_t rb = ((((pix >> rShift) & rMask) * rScale) & 0x00FF0000u) +
                      ((((pix >> bShift) & bMask) * bScale) >>  8);

        ag |= 0x00FF0000u;
        rb *= _a;
        ag *= _a;

        rb += 0x00800080u;
        ag += 0x00800080u;

        rb = (rb + ((rb >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;
        ag = (ag + ((ag >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;

        rb >>= 8;
        Support::writeU32a(dstData, ag + rb);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchU(srcData);

        uint32_t _a = ((((pix >> aShift) & aMask) * aScale) >> 24);
        uint32_t ag = ((((pix >> gShift) & gMask) * gScale) >>  8);
        uint32_t rb = ((((pix >> rShift) & rMask) * rScale) & 0x00FF0000u) +
                      ((((pix >> bShift) & bMask) * bScale) >>  8);

        ag |= 0x00FF0000u;
        rb *= _a;
        ag *= _a;

        rb += 0x00800080u;
        ag += 0x00800080u;

        rb = (rb + ((rb >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;
        ag = (ag + ((ag >> 8) & 0x00FF00FFu)) & 0xFF00FF00u;

        rb >>= 8;
        Support::writeU32a(dstData, ag + rb);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtNative32FromPRGBAny(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::NativeFromXRGB>();

  dstStride -= w * 4 + gap;
  srcStride -= w * PixelAccess::kSize;

  uint32_t aMask = d.masks[0];
  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t aShift = d.shift[0];
  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  uint32_t aScale = d.scale[0];
  uint32_t rScale = d.scale[1];
  uint32_t gScale = d.scale[2];
  uint32_t bScale = d.scale[3];

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(srcData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchA(srcData);

        uint32_t a = ((pix >> aShift) & aMask) * aScale;
        uint32_t g = ((pix >> gShift) & gMask) * gScale;
        uint32_t r = ((pix >> rShift) & rMask) * rScale;
        uint32_t b = ((pix >> bShift) & bMask) * bScale;

        uint32_t ag = (a + g) & 0xFF00FF00u;
        uint32_t rb = (r & 0x00FF0000u) + (b >> 8);
        Support::writeU32a(dstData, ag + rb);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = PixelAccess::fetchU(srcData);

        uint32_t a = ((pix >> aShift) & aMask) * aScale;
        uint32_t g = ((pix >> gShift) & gMask) * gScale;
        uint32_t r = ((pix >> rShift) & rMask) * rScale;
        uint32_t b = ((pix >> bShift) & bMask) * bScale;

        uint32_t ag = (a + g) & 0xFF00FF00u;
        uint32_t rb = (r & 0x00FF0000u) + (b >> 8);
        Support::writeU32a(dstData, ag + rb);

        dstData += 4;
        srcData += PixelAccess::kSize;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

// ============================================================================
// [b2d::PixelConverter - XRGB|ARGB|PRGB <- Native]
// ============================================================================

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtXRGBAnyFromNative32(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::XRGBFromNative>();

  dstStride -= w * PixelAccess::kSize + gap;
  srcStride -= w * 4;

  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(dstData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t r = ((pix >> 16) & 0xFFu) * 0x01010101u;
        uint32_t g = ((pix >>  8) & 0xFFu) * 0x01010101u;
        uint32_t b = ((pix      ) & 0xFFu) * 0x01010101u;

        PixelAccess::storeA(dstData, ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t r = ((pix >> 16) & 0xFFu) * 0x01010101u;
        uint32_t g = ((pix >>  8) & 0xFFu) * 0x01010101u;
        uint32_t b = ((pix      ) & 0xFFu) * 0x01010101u;

        PixelAccess::storeU(dstData, ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtARGBAnyFromNative32(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::XRGBFromNative>();

  dstStride -= w * PixelAccess::kSize + gap;
  srcStride -= w * 4;

  uint32_t aMask = d.masks[0];
  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t aShift = d.shift[0];
  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  const uint32_t* div24bitRecip = Pipe2D::gConstants.div24bit;

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(dstData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t a = pix >> 24;
        uint32_t recip = div24bitRecip[a];

        uint32_t r = ((((pix >> 16) & 0xFFu) * recip) >> 16) * 0x01010101u;
        uint32_t g = ((((pix >>  8) & 0xFFu) * recip) >> 16) * 0x01010101u;
        uint32_t b = ((((pix      ) & 0xFFu) * recip) >> 16) * 0x01010101u;

        a *= 0x01010101u;
        PixelAccess::storeA(dstData, ((a >> aShift) & aMask) +
                                     ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t a = pix >> 24;
        uint32_t recip = div24bitRecip[a];

        uint32_t r = ((((pix >> 16) & 0xFFu) * recip) >> 16) * 0x01010101u;
        uint32_t g = ((((pix >>  8) & 0xFFu) * recip) >> 16) * 0x01010101u;
        uint32_t b = ((((pix      ) & 0xFFu) * recip) >> 16) * 0x01010101u;

        a *= 0x01010101u;
        PixelAccess::storeU(dstData, ((a >> aShift) & aMask) +
                                     ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

template<typename PixelAccess, bool AlwaysUnaligned>
static void B2D_CDECL PixelConverter_cvtPRGBAnyFromNative32(
  const PixelConverter* cvt,
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcData, intptr_t srcStride, uint32_t w, uint32_t h, uint32_t gap) noexcept {

  const auto& d = cvt->data<PixelConverter::Data::XRGBFromNative>();

  dstStride -= w * PixelAccess::kSize + gap;
  srcStride -= w * 4;

  uint32_t aMask = d.masks[0];
  uint32_t rMask = d.masks[1];
  uint32_t gMask = d.masks[2];
  uint32_t bMask = d.masks[3];

  uint32_t aShift = d.shift[0];
  uint32_t rShift = d.shift[1];
  uint32_t gShift = d.shift[2];
  uint32_t bShift = d.shift[3];

  for (uint32_t y = h; y != 0; y--) {
    if (!AlwaysUnaligned && Support::isAligned(dstData, PixelAccess::kSize)) {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t r = ((pix >> 16) & 0xFFu) * 0x01010101u;
        uint32_t g = ((pix >>  8) & 0xFFu) * 0x01010101u;
        uint32_t b = ((pix      ) & 0xFFu) * 0x01010101u;
        uint32_t a = ((pix >> 24)        ) * 0x01010101u;

        PixelAccess::storeA(dstData, ((a >> aShift) & aMask) +
                                     ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }
    else {
      for (uint32_t i = w; i != 0; i--) {
        uint32_t pix = Support::readU32a(srcData);

        uint32_t r = ((pix >> 16) & 0xFFu) * 0x01010101u;
        uint32_t g = ((pix >>  8) & 0xFFu) * 0x01010101u;
        uint32_t b = ((pix      ) & 0xFFu) * 0x01010101u;
        uint32_t a = ((pix >> 24)        ) * 0x01010101u;

        PixelAccess::storeU(dstData, ((a >> aShift) & aMask) +
                                     ((r >> rShift) & rMask) +
                                     ((g >> gShift) & gMask) +
                                     ((b >> bShift) & bMask));
        dstData += PixelAccess::kSize;
        srcData += 4;
      }
    }

    dstData = PixelConverter::_fillGap(dstData, gap);
    dstData += dstStride;
    srcData += srcStride;
  }
}

// ============================================================================
// [b2d::PixelConverter - Init / Reset]
// ============================================================================

// TODO: Doesn't use `pixelFormat`, always converts to PRGB???
Error PixelConverter::_init(uint32_t mode, uint32_t pixelFormat, uint32_t layout, const void* layoutData) noexcept {
  _convert = nullptr;

  // Catch invalid pixel format.
  if (B2D_UNLIKELY(!PixelFormat::isValid(pixelFormat)))
    return DebugUtils::errored(kErrorInvalidArgument);

  // It's an error if both BE and LE byte-order flags were passed.
  if (B2D_UNLIKELY((layout & kLayoutBOMask) == (kLayoutLE | kLayoutBE)))
    return DebugUtils::errored(kErrorInvalidArgument);

  // Use the native byte-order if not specified.
  if ((layout & kLayoutBOMask) == 0)
    layout |= kLayoutNativeBO;

  // Extract basic information.
  uint32_t depth = (layout & kLayoutDepthMask);
  bool isIndexed = (layout & kLayoutIsIndexed) != 0;
  bool isByteAligned = false;

  // --------------------------------------------------------------------------
  // [Normalize - ByteOrder + Masks]
  // --------------------------------------------------------------------------

  const uint32_t* masks = static_cast<const uint32_t*>(layoutData);
  uint32_t normalizedMasks[4];

  if (!isIndexed) {
    B2D_ASSERT(masks != nullptr);
    isByteAligned = PixelConverter_isMaskByteAligned(masks[0]) &
                    PixelConverter_isMaskByteAligned(masks[1]) &
                    PixelConverter_isMaskByteAligned(masks[2]) &
                    PixelConverter_isMaskByteAligned(masks[3]) ;

    // If all masks can be BYTE indexed we can normalize them to a native byte-order.
    if (isByteAligned) {
      layout |= _kLayoutIsByteAligned;
      if ((layout & kLayoutBOMask) != kLayoutNativeBO && depth >= 16) {
        switch (depth) {
          case 16:
            normalizedMasks[0] = Support::byteswap16(masks[0]);
            normalizedMasks[1] = Support::byteswap16(masks[1]);
            normalizedMasks[2] = Support::byteswap16(masks[2]);
            normalizedMasks[3] = Support::byteswap16(masks[3]);
            break;

          case 24:
            normalizedMasks[0] = Support::byteswap24(masks[0]);
            normalizedMasks[1] = Support::byteswap24(masks[1]);
            normalizedMasks[2] = Support::byteswap24(masks[2]);
            normalizedMasks[3] = Support::byteswap24(masks[3]);
            break;

          case 32:
            normalizedMasks[0] = Support::byteswap32(masks[0]);
            normalizedMasks[1] = Support::byteswap32(masks[1]);
            normalizedMasks[2] = Support::byteswap32(masks[2]);
            normalizedMasks[3] = Support::byteswap32(masks[3]);
            break;
        }

        masks = normalizedMasks;
        layout ^= kLayoutBOMask;
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Import - Native <- External Layout]
  // --------------------------------------------------------------------------

  bool isLE = (layout & kLayoutLE) != 0;

  if (mode == kModeImport) {
    if (isIndexed) {
      switch (depth) {
        case 1: _convert = (ConvertFunc)PixelConverter_cvtNative32FromPal1; break;
        case 2: _convert = (ConvertFunc)PixelConverter_cvtNative32FromPal2; break;
        case 4: _convert = (ConvertFunc)PixelConverter_cvtNative32FromPal4; break;
        case 8: _convert = (ConvertFunc)PixelConverter_cvtNative32FromPal8; break;

        default:
          return DebugUtils::errored(kErrorInvalidArgument);
      }

      data<Data::NativeFromIndexed>().layout = layout;
      data<Data::NativeFromIndexed>().pal = static_cast<const uint32_t*>(layoutData);
      return kErrorOk;
    }
    else {
      auto& d = data<Data::NativeFromXRGB>();

      d.layout = layout;
      d.fillMask = (pixelFormat == PixelFormat::kPRGB32 && masks[0] != 0) ? 0x00000000u : 0xFF000000u;

      bool isARGB = masks[0] != 0;
      bool isPRGB = isARGB && (layout & kLayoutIsPremultiplied) != 0;
      bool isGray = masks[1] == masks[2] && masks[2] == masks[3];

      for (uint32_t i = 0; i < 4; i++) {
        uint32_t mask = masks[i];
        uint32_t shift = 0;

        d.masks[i] = 0;
        d.shift[i] = 0;
        d.scale[i] = 0;

        if (mask == 0)
          continue;

        // Calculate bit-shift needed to get value of 8 bits or less.
        shift = Support::ctz(mask);
        mask >>= shift;

        // Discard all bits that are below 8 most significant ones.
        while (mask & 0xFFFFFF00u) {
          mask >>= 1;
          shift++;
        }

        d.masks[i] = mask;
        d.shift[i] = uint8_t(shift);

        // Calculate a scale constant that will be used to expand bits in
        // case that the source contains less than 8 bits.
        uint32_t scale = 0x1;
        while ((mask & 0xFF) != 0xFF) {
          scale |= mask + 1;
          mask *= scale;
        }

        // Shift scale in a way that it contains MSB of the mask.
        uint32_t msb = PixelConverter_native32FromXRGBMask[i];
        while ((msb & mask) != msb) {
          mask <<= 1;
          scale <<= 1;
        }

        d.scale[i] = scale;
      }

      // Prefer SIMD optimized converters if possible.
      #if B2D_MAYBE_AVX2
      if (Runtime::hwInfo().hasAVX2() && PixelConverter_initNativeFromXRGB_AVX2(this, pixelFormat))
        return kErrorOk;
      #endif

      #if B2D_MAYBE_SSSE3
      if (Runtime::hwInfo().hasSSSE3() && PixelConverter_initNativeFromXRGB_SSSE3(this, pixelFormat))
        return kErrorOk;
      #endif

      #if B2D_MAYBE_SSE2
      if (Runtime::hwInfo().hasSSE2() && PixelConverter_initNativeFromXRGB_SSE2(this, pixelFormat))
        return kErrorOk;
      #endif

      // Generic converters.
      if (isGray) {
        // TODO:
      }
      else {
        switch (depth) {
          case 16:
            if (isPRGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                              : (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
            else if (isARGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                              : (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
            else
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                              : (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
            break;

          case 24:
            if (isPRGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess24<Globals::kByteOrderLE>, true>
                              : (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess24<Globals::kByteOrderBE>, true>;
            else if (isARGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess24<Globals::kByteOrderLE>, true>
                              : (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess24<Globals::kByteOrderBE>, true>;
            else
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess24<Globals::kByteOrderLE>, true>
                              : (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess24<Globals::kByteOrderBE>, true>;
            break;

          case 32:
            if (isPRGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                              : (ConvertFunc)PixelConverter_cvtNative32FromPRGBAny<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
            else if (isARGB)
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                              : (ConvertFunc)PixelConverter_cvtNative32FromARGBAny<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
            else
              _convert = isLE ? (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                              : (ConvertFunc)PixelConverter_cvtNative32FromXRGBAny<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
            break;

          default:
            return DebugUtils::errored(kErrorInvalidArgument);
        }
      }

      return kErrorOk;
    }
  }

  // --------------------------------------------------------------------------
  // [Export - External Layout <- Native]
  // --------------------------------------------------------------------------

  if (mode == kModeExport) {
    if (isIndexed) {
      // TODO:
      return DebugUtils::errored(kErrorNotImplemented);
    }
    else {
      auto& d = data<Data::XRGBFromNative>();

      d.layout = layout;
      d.fillMask = 0;

      bool isARGB = masks[0] != 0;
      bool isPRGB = isARGB && (layout & kLayoutIsPremultiplied) != 0;
      bool isGray = masks[1] == masks[2] && masks[2] == masks[3];

      for (uint32_t i = 0; i < 4; i++) {
        uint32_t mask = masks[i];
        uint32_t shift = 0;

        d.masks[i] = mask;
        d.shift[i] = 0;

        if (mask == 0)
          continue;

        // Calculate bit-shift needed to pack the value.
        if ((mask & 0xFFFF0000u) == 0) { shift += 16; mask <<= 16; }
        if ((mask & 0xFF000000u) == 0) { shift += 8 ; mask <<= 8 ; }
        if ((mask & 0xF0000000u) == 0) { shift += 4 ; mask <<= 4 ; }
        if ((mask & 0x30000000u) == 0) { shift += 2 ; mask <<= 2 ; }
        if ((mask & 0x10000000u) == 0) { shift += 1 ; mask <<= 1 ; }

        // Shift is performed from right to left.
        d.shift[i] = uint8_t(shift);
      }

      switch (depth) {
        case 16:
          if (isPRGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                            : (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
          else if (isARGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                            : (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
          else
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess16<Globals::kByteOrderLE>, Support::kUnalignedAccess16>
                            : (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess16<Globals::kByteOrderBE>, Support::kUnalignedAccess16>;
          break;

        case 24:
          if (isPRGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess24<Globals::kByteOrderLE>, true>
                            : (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess24<Globals::kByteOrderBE>, true>;
          else if (isARGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess24<Globals::kByteOrderLE>, true>
                            : (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess24<Globals::kByteOrderBE>, true>;
          else
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess24<Globals::kByteOrderLE>, true>
                            : (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess24<Globals::kByteOrderBE>, true>;
          break;

        case 32:
          if (isPRGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                            : (ConvertFunc)PixelConverter_cvtPRGBAnyFromNative32<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
          else if (isARGB)
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                            : (ConvertFunc)PixelConverter_cvtARGBAnyFromNative32<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
          else
            _convert = isLE ? (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess32<Globals::kByteOrderLE>, Support::kUnalignedAccess32>
                            : (ConvertFunc)PixelConverter_cvtXRGBAnyFromNative32<PixelAccess32<Globals::kByteOrderBE>, Support::kUnalignedAccess32>;
          break;

        default:
          return DebugUtils::errored(kErrorInvalidArgument);
      }

      return kErrorOk;
    }
  }

  // --------------------------------------------------------------------------
  // [Invalid]
  // --------------------------------------------------------------------------

  return DebugUtils::errored(kErrorInvalidArgument);
}

Error PixelConverter::reset() noexcept {
  _convert = nullptr;
  return kErrorOk;
}

// ============================================================================
// [b2d::PixelConverter - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
template<typename T>
struct PixelConverterUnit {
  static void fillMasks(uint32_t* dst) noexcept {
    dst[0] = T::kA;
    dst[1] = T::kR;
    dst[2] = T::kG;
    dst[3] = T::kB;
  }

  static void testPrgb32() noexcept {
    INFO("Testing '%s' format", T::formatString());

    PixelConverter fromPrgb32;
    PixelConverter toPrgb32;

    uint32_t masks[4];
    fillMasks(masks);

    EXPECT(fromPrgb32.initExport(PixelFormat::kPRGB32, T::kDepth | PixelConverter::kLayoutIsPremultiplied, masks) == kErrorOk,
      "%s: initExport(depth=%d, a=%08X, r=%08X, g=%08X, b=08X)",
      T::formatString(), T::kDepth, T::kA, T::kR, T::kG, T::kB);

    EXPECT(toPrgb32.initImport(PixelFormat::kPRGB32, T::kDepth | PixelConverter::kLayoutIsPremultiplied, masks) == kErrorOk,
      "%s: initImport(depth=%d, a=%08X, r=%08X, g=%08X, b=08X)",
      T::formatString(), T::kDepth, T::kA, T::kR, T::kG, T::kB);

    enum : uint32_t { kCount = 8 };

    static const uint32_t src[kCount] = {
      0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
      0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF
    };

    uint32_t dst[kCount];
    uint8_t buf[kCount * 8];

    uint32_t i;

    // The test is rather basic now, we basically convert from PRGB to external
    // pixel format, then back, and then compare if the output is matching input.
    // In the future we should also check the intermediate result.
    fromPrgb32.convertSpan(buf, src, kCount);
    toPrgb32.convertSpan(dst, buf, kCount);

    for (i = 0; i < kCount; i++) {
      EXPECT(dst[i] == src[i],
        "%s: %08X != %08X [Depth=%d A=%08X R=%08X G=%08X B=%08X]",
        T::formatString(), dst[i], src[i], T::kDepth, T::kA, T::kR, T::kG, T::kB);
    }
  }

  static void test() noexcept {
    testPrgb32();
  }
};

#define B2D_PIXEL_TEST(FORMAT, DEPTH, A_MASK, R_MASK, G_MASK, B_MASK)     \
  struct Test_##FORMAT {                                                  \
    static inline const char* formatString() noexcept { return #FORMAT; } \
                                                                          \
    enum : uint32_t {                                                     \
      kDepth = DEPTH,                                                     \
      kA = A_MASK,                                                        \
      kR = R_MASK,                                                        \
      kG = G_MASK,                                                        \
      kB = B_MASK                                                         \
    };                                                                    \
  }

B2D_PIXEL_TEST(XRGB_0555, 16, 0x00000000u, 0x00007C00u, 0x000003E0u, 0x0000001Fu);
B2D_PIXEL_TEST(XBGR_0555, 16, 0x00000000u, 0x0000001Fu, 0x000003E0u, 0x00007C00u);
B2D_PIXEL_TEST(XRGB_0565, 16, 0x00000000u, 0x0000F800u, 0x000007E0u, 0x0000001Fu);
B2D_PIXEL_TEST(XBGR_0565, 16, 0x00000000u, 0x0000001Fu, 0x000007E0u, 0x0000F800u);
B2D_PIXEL_TEST(ARGB_4444, 16, 0x0000F000u, 0x00000F00u, 0x000000F0u, 0x0000000Fu);
B2D_PIXEL_TEST(ABGR_4444, 16, 0x0000F000u, 0x0000000Fu, 0x000000F0u, 0x00000F00u);
B2D_PIXEL_TEST(RGBA_4444, 16, 0x0000000Fu, 0x0000F000u, 0x00000F00u, 0x000000F0u);
B2D_PIXEL_TEST(BGRA_4444, 16, 0x0000000Fu, 0x000000F0u, 0x00000F00u, 0x0000F000u);
B2D_PIXEL_TEST(XRGB_0888, 24, 0x00000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
B2D_PIXEL_TEST(XBGR_0888, 24, 0x00000000u, 0x000000FFu, 0x0000FF00u, 0x00FF0000u);
B2D_PIXEL_TEST(XRGB_8888, 32, 0x00000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
B2D_PIXEL_TEST(XBGR_8888, 32, 0x00000000u, 0x000000FFu, 0x0000FF00u, 0x00FF0000u);
B2D_PIXEL_TEST(RGBX_8888, 32, 0x00000000u, 0xFF000000u, 0x00FF0000u, 0x0000FF00u);
B2D_PIXEL_TEST(BGRX_8888, 32, 0x00000000u, 0x0000FF00u, 0x00FF0000u, 0xFF000000u);
B2D_PIXEL_TEST(ARGB_8888, 32, 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
B2D_PIXEL_TEST(ABGR_8888, 32, 0xFF000000u, 0x000000FFu, 0x0000FF00u, 0x00FF0000u);
B2D_PIXEL_TEST(RGBA_8888, 32, 0x000000FFu, 0xFF000000u, 0x00FF0000u, 0x0000FF00u);
B2D_PIXEL_TEST(BGRA_8888, 32, 0x000000FFu, 0x0000FF00u, 0x00FF0000u, 0xFF000000u);
B2D_PIXEL_TEST(BRGA_8888, 32, 0x000000FFu, 0x00FF0000u, 0x0000FF00u, 0xFF000000u);

#undef B2D_PIXEL_TEST

UNIT(b2d_core_pixel_converter) {
  PixelConverterUnit<Test_XRGB_0555>::test();
  PixelConverterUnit<Test_XBGR_0555>::test();
  PixelConverterUnit<Test_XRGB_0565>::test();
  PixelConverterUnit<Test_XBGR_0565>::test();
  PixelConverterUnit<Test_ARGB_4444>::test();
  PixelConverterUnit<Test_ABGR_4444>::test();
  PixelConverterUnit<Test_RGBA_4444>::test();
  PixelConverterUnit<Test_BGRA_4444>::test();
  PixelConverterUnit<Test_XRGB_0888>::test();
  PixelConverterUnit<Test_XBGR_0888>::test();
  PixelConverterUnit<Test_XRGB_8888>::test();
  PixelConverterUnit<Test_XBGR_8888>::test();
  PixelConverterUnit<Test_RGBX_8888>::test();
  PixelConverterUnit<Test_BGRX_8888>::test();
  PixelConverterUnit<Test_ARGB_8888>::test();
  PixelConverterUnit<Test_ABGR_8888>::test();
  PixelConverterUnit<Test_RGBA_8888>::test();
  PixelConverterUnit<Test_BGRA_8888>::test();
  PixelConverterUnit<Test_BRGA_8888>::test();
}
#endif

B2D_END_NAMESPACE
