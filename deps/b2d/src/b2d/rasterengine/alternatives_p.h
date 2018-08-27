// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_ANTERNATIVES_P_H
#define _B2D_RASTERENGINE_ANTERNATIVES_P_H

// [Dependencies]
#include "../core/compop.h"
#include "../core/pixelformat.h"
#include "../core/support.h"
#include "../rasterengine/rasterglobals_p.h"
#include "../rasterengine/rastercontext2d_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterEngine::Alternatives]
// ============================================================================

// This is similar to `Pipe2D::CompOpInfo`, however, it's not designed to
// simplify composition operators mathematically, but to minimize the number of
// pipelines the engine could use by replacing some composition operators with
// ones that perform the same operation but need a different source style.
namespace Alternatives {
  struct AltPixelFormat {
    enum AltId : uint32_t {
      kAltNone       = 0,                //!< No alternative, keep everything as is.
      kAltXRGBToPRGB = 1                 //!< Convert all XRGB formats to PRGB.
    };

    uint8_t map[Support::alignUp(PixelFormat::kCount, 8)];
  };

  template<uint32_t ALT_ID, uint32_t SRC_ID>
  struct AltPixelFormatT {
    enum : uint32_t {
      // Helpers...
      kAsIs       = SRC_ID,
      kXRGBToPRGB = SRC_ID == PixelFormat::kXRGB32 ? PixelFormat::kPRGB32 : kAsIs,

      // Destination pixel format.
      kDstId        = ALT_ID == AltPixelFormat::kAltNone       ? kAsIs       :
                      ALT_ID == AltPixelFormat::kAltXRGBToPRGB ? kXRGBToPRGB : kAsIs
    };
  };

  #define ROW(ALT_ID) {{ \
    AltPixelFormatT<ALT_ID, PixelFormat::kNone  >::kDstId, \
    AltPixelFormatT<ALT_ID, PixelFormat::kPRGB32>::kDstId, \
    AltPixelFormatT<ALT_ID, PixelFormat::kXRGB32>::kDstId, \
    AltPixelFormatT<ALT_ID, PixelFormat::kA8    >::kDstId, \
  }}

  static const AltPixelFormat altPixelFormatTable[] = {
    ROW(AltPixelFormat::kAltNone),
    ROW(AltPixelFormat::kAltXRGBToPRGB)
  };

  #undef ROW

  struct AltCompOpInfo {
    B2D_INLINE uint32_t additionalContextFlags() const noexcept { return _additionalContextFlags; }
    B2D_INLINE uint32_t suggestedCompOp() const noexcept { return _suggestedCompOp; }

    B2D_INLINE uint32_t suggestedPixelFormat(uint32_t pixelFormat) const noexcept {
      return altPixelFormatTable[_suggestedPixelFormatMapId].map[pixelFormat];
    }

    B2D_INLINE uint32_t suggestedPixelFormat(uint32_t pixelFormat, uint32_t maxAltId) const noexcept {
      return altPixelFormatTable[Math::min<uint32_t>(_suggestedPixelFormatMapId, maxAltId)].map[pixelFormat];
    }

    uint32_t _additionalContextFlags;

    uint8_t _suggestedCompOp;
    uint8_t _suggestedPixelFormatMapId;
    uint8_t _suggestedSolidSourceId;
    uint8_t _reserved;
  };

  template<uint32_t COMP_OP>
  struct AltCompOpInfoT {
    //   `CompOp::kClear` - Replace by `CompOp::kSrc`.
    //                    - Force solid color [0 0 0 0].
    //   `CompOp::kSrc`   - Force all components as it doesn't matter (it's a LERP operation).
    enum : uint32_t {
      kAdditionalContextFlags = COMP_OP == CompOp::kClear   ? kSWContext2DFlagCompOpSolid     :
                                COMP_OP == CompOp::kDst     ? kSWContext2DNoPaintGlobalCompOp : uint32_t(0u),

      kSuggestedCompOp        = COMP_OP == CompOp::kClear   ? CompOp::kSrc
                                                            : COMP_OP,

      kSuggestedPixelFormat   = COMP_OP == CompOp::kSrc     ? AltPixelFormat::kAltXRGBToPRGB
                                                            : AltPixelFormat::kAltNone
    };
  };

  #define ROW(COMP_OP) {                              \
    AltCompOpInfoT<COMP_OP>::kAdditionalContextFlags, \
    AltCompOpInfoT<COMP_OP>::kSuggestedCompOp,        \
    AltCompOpInfoT<COMP_OP>::kSuggestedPixelFormat,   \
    0,                                                \
    0                                                 \
  }

  static const AltCompOpInfo altCompOpInfo[] = {
    ROW(CompOp::kSrc        ),
    ROW(CompOp::kSrcOver    ),
    ROW(CompOp::kSrcIn      ),
    ROW(CompOp::kSrcOut     ),
    ROW(CompOp::kSrcAtop    ),
    ROW(CompOp::kDst        ),
    ROW(CompOp::kDstOver    ),
    ROW(CompOp::kDstIn      ),
    ROW(CompOp::kDstOut     ),
    ROW(CompOp::kDstAtop    ),
    ROW(CompOp::kXor        ),
    ROW(CompOp::kClear      ),
    ROW(CompOp::kPlus       ),
    ROW(CompOp::kPlusAlt    ),
    ROW(CompOp::kMinus      ),
    ROW(CompOp::kMultiply   ),
    ROW(CompOp::kScreen     ),
    ROW(CompOp::kOverlay    ),
    ROW(CompOp::kDarken     ),
    ROW(CompOp::kLighten    ),
    ROW(CompOp::kColorDodge ),
    ROW(CompOp::kColorBurn  ),
    ROW(CompOp::kLinearBurn ),
    ROW(CompOp::kLinearLight),
    ROW(CompOp::kPinLight   ),
    ROW(CompOp::kHardLight  ),
    ROW(CompOp::kSoftLight  ),
    ROW(CompOp::kDifference ),
    ROW(CompOp::kExclusion  ),
    ROW(CompOp::_kAlphaSet  ),
    ROW(CompOp::_kAlphaInv  )
  };

  #undef ROW
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_ANTERNATIVES_P_H
