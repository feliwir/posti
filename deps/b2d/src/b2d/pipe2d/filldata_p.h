// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FILLDATA_P_H
#define _B2D_PIPE2D_FILLDATA_P_H

// [Dependencies]
#include "./pipepart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::A8Consts]
// ============================================================================

namespace A8Consts {
  enum A8 : uint32_t {
    kA8Shift      = 8,              // 8.
    kA8Scale      = 1 << kA8Shift,  // 256.
    kA8Mask       = kA8Scale - 1    // 255.
  };
}

// ============================================================================
// [b2d::Pipe2D::FillData]
// ============================================================================

struct FillData {
  typedef Support::BitWord BitWord;
  typedef uint32_t Cell;

  // --------------------------------------------------------------------------
  // [Structs]
  // --------------------------------------------------------------------------

  struct Common {
    IntBox box;                            //!< Rectangle to fill.
    Value32 alpha;                         //!< Alpha value (range depends on PixelFormat).
  };

  //! Rectangle (axis-aligned).
  struct BoxAA {
    IntBox box;                            //!< Rectangle to fill.
    Value32 alpha;                         //!< Alpha value (range depends on PixelFormat).
  };

  //! Rectangle (axis-unaligned).
  struct BoxAU {
    IntBox box;                            //!< Rectangle to fill.
    Value32 alpha;                         //!< Alpha value (range depends on PixelFormat).

    uint32_t masks[3];                     //!< Masks of top, middle and bottom part of the rect.
    uint32_t startWidth;                   //!< Start width (from 1 to 3).
    uint32_t innerWidth;                   //!< Inner width (from 0 to width).
  };

  struct Analytic {
    IntBox box;                            //!< Fill boundary (x0 is ignored, x1 acts as maxWidth, y0/y1 are used normally).
    Value32 alpha;                         //!< Alpha value (range depends on PixelFormat).
    uint32_t fillRuleMask;                 //!< All ones if NonZero or 0x01FF if EvenOdd.

    BitWord* bitTopPtr;                    //!< Rasterizer bits (marks a group of cells which are non-zero).
    size_t bitStride;                      //!< Bit stride (in bytes).

    Cell* cellTopPtr;                      //!< Rasterizer cells.
    size_t cellStride;                     //!< Cell stride (in bytes).
  };

  // --------------------------------------------------------------------------
  // [Init]
  // --------------------------------------------------------------------------

  inline uint32_t initBoxAA8bpc(uint32_t alpha, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) noexcept {
    // The rendering engine should never allow alpha out-of-range.
    B2D_ASSERT(alpha <= 256);

    // The rendering engine should never pass invalid box to the pipeline.
    B2D_ASSERT(x0 < x1);
    B2D_ASSERT(y0 < y1);

    boxAA.alpha.u = alpha;
    boxAA.box.reset(int(x0), int(y0), int(x1), int(y1));
    return kFillTypeBoxAA;
  }

  template<typename T>
  inline uint32_t initBoxAU8bpcT(uint32_t alpha, T x0, T y0, T x1, T y1) noexcept {
    return initBoxAU8bpc24x8(alpha, uint32_t(Math::ifloor(x0 * T(256))),
                                    uint32_t(Math::ifloor(y0 * T(256))),
                                    uint32_t(Math::ifloor(x1 * T(256))),
                                    uint32_t(Math::ifloor(y1 * T(256))));
  }

  uint32_t initBoxAU8bpc24x8(uint32_t alpha, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) noexcept {
    // The rendering engine should never allow alpha out-of-range.
    B2D_ASSERT(alpha <= 256);

    uint32_t ax0 = x0 >> 8;
    uint32_t ay0 = y0 >> 8;
    uint32_t ax1 = x1 >> 8;
    uint32_t ay1 = y1 >> 8;

    boxAU.alpha.u = alpha;
    boxAU.box.reset(int(ax0), int(ay0), int(ax1), int(ay1));

    // Special case - Coordinates are very close for 24x8 representation.
    if (x0 >= x1 || y0 >= y1)
      return kFillTypeNone;

    // Special case - Aligned Box.
    if (((x0 | x1 | y0 | y1) & 0xFFu) == 0u)
      return kFillTypeBoxAA;

    uint32_t fx0 = x0 & 0xFFu;
    uint32_t fy0 = y0 & 0xFFu;
    uint32_t fx1 = x1 & 0xFFu;
    uint32_t fy1 = y1 & 0xFFu;

    boxAU.box._x1 += fx1 != 0;
    boxAU.box._y1 += fy1 != 0;

    if (fx1 == 0) fx1 = 256;
    if (fy1 == 0) fy1 = 256;

    fx0 = 256 - fx0;
    fy0 = 256 - fy0;

    if ((x0 & ~0xFF) == (x1 & ~0xFF)) { fx0 = fx1 - fx0; fx1 = 0; }
    if ((y0 & ~0xFF) == (y1 & ~0xFF)) { fy0 = fy1 - fy0; fy1 = 0; }

    uint32_t iw = uint32_t(boxAU.box.x1() - boxAU.box.x0());
    uint32_t m0 = ((fx1 * fy0) >> 8);
    uint32_t m1 = ( fx1            );
    uint32_t m2 = ((fx1 * fy1) >> 8);

    if (iw > 2) {
      m0 = (m0 << 9) + fy0;
      m1 = (m1 << 9) + 256;
      m2 = (m2 << 9) + fy1;
    }

    if (iw > 1) {
      m0 = (m0 << 9) + ((fx0 * fy0) >> 8);
      m1 = (m1 << 9) + fx0;
      m2 = (m2 << 9) + ((fx0 * fy1) >> 8);
    }

    if (alpha != 256) {
      m0 = mulPackedMaskByAlpha(m0, alpha);
      m1 = mulPackedMaskByAlpha(m1, alpha);
      m2 = mulPackedMaskByAlpha(m2, alpha);
    }

    boxAU.masks[0] = m0;
    boxAU.masks[1] = m1;
    boxAU.masks[2] = m2;

    if (iw > 3) {
      boxAU.startWidth = 1;
      boxAU.innerWidth = iw - 2;
    }
    else {
      boxAU.startWidth = iw;
      boxAU.innerWidth = 0;
    }

    return kFillTypeBoxAU;
  }

  inline uint32_t initAnalytic(uint32_t alpha, BitWord* bitTopPtr, size_t bitStride, Cell* cellTopPtr, size_t cellStride) noexcept {
    analytic.alpha.u = alpha;
    analytic.bitTopPtr = bitTopPtr;
    analytic.bitStride = bitStride;
    analytic.cellTopPtr = cellTopPtr;
    analytic.cellStride = cellStride;

    return kFillTypeAnalytic;
  }

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  static inline uint32_t mulPackedMaskByAlpha(uint32_t m, uint32_t alpha) noexcept {
    return (((((m >> 18)        ) * alpha) >> 8) << 18) |
           (((((m >>  9) & 0x1FF) * alpha) >> 8) <<  9) |
           (((((m      ) & 0x1FF) * alpha) >> 8)      ) ;
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    Common common;
    BoxAA boxAA;
    BoxAU boxAU;
    Analytic analytic;
  };
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FILLDATA_P_H
