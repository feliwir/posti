// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_COMPOP_H
#define _B2D_CORE_COMPOP_H

// [Dependencies]
#include "../core/pixelformat.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::CompOp]
// ============================================================================

//! Composition operators.
//!
//! This is just a namespace that contains `CompOp::Id`.
namespace CompOp {
  // --------------------------------------------------------------------------
  // [Id]
  // --------------------------------------------------------------------------

  //! Composition and blending operators.
  //!
  //! The difference between `kPlus` and `kPlusAlt`
  //! ---------------------------------------------
  //!
  //! These two operators are essentially the same, however, they behave dirrerently
  //! when a mask (rasterizer, clipping, global alpha, etc...) is used by compositor.
  //!
  //! Some libraries don't follow the composition rules regarding `PLUS` operator
  //! and reduce the composition equation to `Clamp(dst + src * m)`, which doesn't
  //! follow the rule that masking should be applied after composition.
  //!
  //! Instead of picking a side Blend2D offers both equations. The mathematically
  //! correct one is offered as `kPlus` and the alternative is offered as `kPlusAlt.
  //!
  //! `kPlus` equation without and with mask:
  //!
  //!   ```
  //!   Dca' = Clamp(Dca + Sca)          Dca' = Clamp(Dca + Sca).m + Dca.(1 - m)
  //!   Da'  = Clamp(Da  + Sa )          Da'  = Clamp(Da  + Sa ).m + Da .(1 - m)
  //!   ```
  //!
  //! `kPlusAlt` equation with mask:
  //!
  //!   ```
  //!   Dca' = Clamp(Dca + Sca)          Dca' = Clamp(Dca + Sca.m)
  //!   Da'  = Clamp(Da  + Sa )          Da'  = Clamp(Da  + Sa .m)
  //!   ```
  enum Id : uint32_t {
    kSrc                  = 0,           //!< Source (copy).
    kSrcOver              = 1,           //!< Source-over.
    kSrcIn                = 2,           //!< Source-in.
    kSrcOut               = 3,           //!< Source-out.
    kSrcAtop              = 4,           //!< Source-atop.
    kDst                  = 5,           //!< Destination (nop).
    kDstOver              = 6,           //!< Destination-over.
    kDstIn                = 7,           //!< Destination-in.
    kDstOut               = 8,           //!< Destination-out.
    kDstAtop              = 9,           //!< Destination-atop.
    kXor                  = 10,          //!< Xor.
    kClear                = 11,          //!< Clear.
    kPlus                 = 12,          //!< Plus (mathematically correct version).
    kPlusAlt              = 13,          //!< Plus (alternative version that clamps after applying the mask).
    kMinus                = 14,          //!< Minus.
    kMultiply             = 15,          //!< Multiply.
    kScreen               = 16,          //!< Screen.
    kOverlay              = 17,          //!< Overlay.
    kDarken               = 18,          //!< Darken.
    kLighten              = 19,          //!< Lighten.
    kColorDodge           = 20,          //!< Color dodge.
    kColorBurn            = 21,          //!< Color burn.
    kLinearBurn           = 22,          //!< Linear burn.
    kLinearLight          = 23,          //!< Linear light.
    kPinLight             = 24,          //!< Pin light.
    kHardLight            = 25,          //!< Hard-light.
    kSoftLight            = 26,          //!< Soft-light.
    kDifference           = 27,          //!< Difference.
    kExclusion            = 28,          //!< Exclusion.
    kCount                = 29,          //!< Count of composition and blending operators.

    _kAlphaSet            = 29,          //!< Pipe2D Only: Set destination alpha to 1 (alpha formats only).
    _kAlphaInv            = 30,          //!< Pipe2D Only: Invert destination alpha   (alpha formats only).
    _kInternalCount       = 31           //!< Pipe2D Only: Count of all operators (regular and Pipe2D).
  };
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_COMPOP_H
