// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_PNGCODEC_P_H
#define _B2D_CODEC_PNGCODEC_P_H

// [Dependencies]
#include "../core/imagecodec.h"
#include "../core/math.h"
#include "../core/pixelconverter.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_codecs
//! \{

// ============================================================================
// [b2d::PngConstants]
// ============================================================================

enum PngConstants : uint32_t {
  //! Signature (8 bytes), IHDR tag (8 bytes), IHDR data (13 bytes), and IHDR CRC (4 bytes).
  kPngMinSize = 8 + 8 + 13 + 4
};

// ============================================================================
// [b2d::PngTag]
// ============================================================================

enum PngTag : uint32_t {
  #define B2D_PNG_TAG(A, B, C, D) ((uint32_t(A) << 24) + (uint32_t(B) << 16) + (uint32_t(C) <<  8) + (uint32_t(D)))

  kPngTag_CgBI = B2D_PNG_TAG('C', 'g', 'B', 'I'),
  kPngTag_IDAT = B2D_PNG_TAG('I', 'D', 'A', 'T'),
  kPngTag_IEND = B2D_PNG_TAG('I', 'E', 'N', 'D'),
  kPngTag_IHDR = B2D_PNG_TAG('I', 'H', 'D', 'R'),
  kPngTag_PLTE = B2D_PNG_TAG('P', 'L', 'T', 'E'),
  kPngTag_tRNS = B2D_PNG_TAG('t', 'R', 'N', 'S')

  #undef B2D_PNG_TAG
};

// ============================================================================
// [b2d::PngTagMask]
// ============================================================================

enum PngTagMask : uint32_t {
  kPngTagMask_IHDR = 0x00000001u,
  kPngTagMask_IDAT = 0x00000002u,
  kPngTagMask_IEND = 0x00000004u,
  kPngTagMask_PLTE = 0x00000010u,
  kPngTagMask_tRNS = 0x00000020u,
  kPngTagMask_CgBI = 0x00000040u
};

// ============================================================================
// [b2d::PngColorType]
// ============================================================================

enum PngColorType : uint32_t {
  kPngColorType0_Lum = 0,                //!< Each pixel is a grayscale sample (1/2/4/8/16-bits per sample).
  kPngColorType2_RGB = 2,                //!< Each pixel is an RGB triple (8/16-bits per sample).
  kPngColorType3_Pal = 3,                //!< Each pixel is a palette index (1/2/4/8 bits per sample).
  kPngColorType4_LumA = 4,               //!< Each pixel is a grayscale+alpha sample (8/16-bits per sample).
  kPngColorType6_RGBA = 6                //!< Each pixel is an RGBA quad (8/16 bits per sample).
};

// ============================================================================
// [b2d::PngFilterType]
// ============================================================================

enum PngFilterType : uint32_t {
  kPngFilterNone = 0,
  kPngFilterSub = 1,
  kPngFilterUp = 2,
  kPngFilterAvg = 3,
  kPngFilterPaeth = 4,
  kPngFilterCount = 5,

  kPngFilterAvg0 = 5                     //!< Synthetic filter used only by Blend2D's reverse-filter implementation.
};

// ============================================================================
// [b2d::PngDecoderInfo]
// ============================================================================

struct PngDecoderInfo {
  uint32_t chunkTags;                    //!< Mask of all PNG chunk types traversed.
  uint8_t colorType;                     //!< Color type.
  uint8_t sampleDepth;                   //!< Depth (depth per one sample).
  uint8_t sampleCount;                   //!< Number of samples (1, 2, 3, 4).
  uint8_t brokenByApple;                 //!< Contains "CgBI" chunk before "IHDR" and other violations.
};

// ============================================================================
// [b2d::PngUtils]
// ============================================================================

//! \internal
//!
//! PNG utilities.
//!
//! Internal functions used by PNG C and SSE2 implementation.
namespace PngUtils {
  //! Get a replacement filter for the first PNG row, because no prior row
  //! exists at that point. This is the only function that can replace average
  //! filter with `kPngFilterAvg0`.
  static B2D_INLINE uint32_t firstRowFilterReplacement(uint32_t filter) noexcept {
    uint32_t kReplacement = (kPngFilterNone <<  0) | // None  -> None
                            (kPngFilterSub  <<  4) | // Sub   -> Sub
                            (kPngFilterNone <<  8) | // Up    -> None
                            (kPngFilterAvg0 << 12) | // Avg   -> Avg0 (Special-Case)
                            (kPngFilterSub  << 16) ; // Paeth -> Sub
    return (kReplacement >> (filter * 4)) & 0xF;
  }

  // Sum two bytes and byte cast.
  static B2D_INLINE uint8_t sum(uint32_t a, uint32_t b) noexcept { return uint8_t((a + b) & 0xFF); }

  // Performs PNG average filter.
  static B2D_INLINE uint32_t average(uint32_t a, uint32_t b) noexcept { return (a + b) >> 1; }

  // Unsigned division by 3 translated into a multiplication and shift. The range
  // of `x` is [0, 255], inclusive. This means that we need at most 16 bits to
  // have the result. In SIMD this is exploited by using PMULHUW instruction that
  // will multiply and shift by 16 bits right (the constant is adjusted for that).
  static B2D_INLINE uint32_t udiv3(uint32_t x) noexcept { return (x * 0xABu) >> 9; }

  // This is an optimized implementation of PNG's Paeth reference filter. This
  // optimization originally comes from my previous implementation where I tried
  // to simplify it to be more SIMD friendly. One interesting property of Paeth
  // filter is:
  //
  //   Paeth(a, b, c) == Paeth(b, a, c);
  //
  // Actually what the filter needs is a minimum and maximum of `a` and `b`, so
  // I based the implementation on getting those first. If you know `min(a, b)`
  // and `max(a, b)` you can divide the interval to be checked against `c`. This
  // requires division by 3, which is available above as `UDiv3()`.
  //
  // The previous implementation looked like:
  //   static inline uint32_t Paeth(uint32_t a, uint32_t b, uint32_t c) {
  //     uint32_t minAB = Min(a, b);
  //     uint32_t maxAB = Max(a, b);
  //     uint32_t divAB = UDiv3(maxAB - minAB);
  //
  //     if (c <= minAB + divAB) return maxAB;
  //     if (c >= maxAB - divAB) return minAB;
  //
  //     return c;
  //   }
  //
  // Although it's not bad I tried to exploit more the idea of SIMD and masking.
  // The following code basically removes the need of any comparison, it relies
  // on bit shifting and performs an arithmetic (not logical) shift of signs
  // produced by `divAB + minAB` and `divAB - maxAB`, which are then used to mask
  // out `minAB` and `maxAB`. The `minAB` and `maxAB` can be negative after `c`
  // is subtracted, which will basically remove the original `c` if one of the
  // two additions is unmasked. The code can unmask either zero or one addition,
  // but it never unmasks both.
  //
  // Don't hesitate to contact the author <kobalicek.petr@gmail.com> if you need
  // a further explanation of the code below, it's probably hard to understand
  // without looking into the original Paeth implementation and without having a
  // visualization of the Paeth function.
  static B2D_INLINE uint32_t paeth(uint32_t a, uint32_t b, uint32_t c) noexcept {
    uint32_t minAB = Math::min(a, b) - c;
    uint32_t maxAB = Math::max(a, b) - c;
    uint32_t divAB = udiv3(maxAB - minAB);

    return c + (maxAB & ~uint32_t(int32_t(divAB + minAB) >> 31)) +
               (minAB & ~uint32_t(int32_t(divAB - maxAB) >> 31)) ;
  }
};

// ============================================================================
// [b2d::PngDecoderImpl]
// ============================================================================

class PngDecoderImpl final : public ImageDecoderImpl {
public:
  typedef ImageDecoderImpl Base;

  PngDecoderImpl() noexcept;
  virtual ~PngDecoderImpl() noexcept;

  Error reset() noexcept override;
  Error setup(const uint8_t* p, size_t size) noexcept override;
  Error decode(Image& dst, const uint8_t* p, size_t size) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  PngDecoderInfo _png;
};

// ============================================================================
// [b2d::PngEncoderImpl]
// ============================================================================

class PngEncoderImpl final : public ImageEncoderImpl {
public:
  typedef ImageEncoderImpl Base;

  PngEncoderImpl() noexcept;
  virtual ~PngEncoderImpl() noexcept;

  Error reset() noexcept override;
  Error encode(Buffer& dst, const Image& src) noexcept override;
};

// ============================================================================
// [b2d::PngCodecImpl]
// ============================================================================

class PngCodecImpl final : public ImageCodecImpl {
public:
  PngCodecImpl() noexcept;
  virtual ~PngCodecImpl() noexcept;

  int inspect(const uint8_t* data, size_t size) const noexcept override;
  ImageDecoderImpl* createDecoderImpl() const noexcept override;
  ImageEncoderImpl* createEncoderImpl() const noexcept override;
};

// ============================================================================
// [b2d::Png - Runtime Handlers]
// ============================================================================

#if defined(B2D_EXPORTS)
void ImageCodecOnInitPng(Array<ImageCodec>* codecList) noexcept;
#endif

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_PNGCODEC_P_H
