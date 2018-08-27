// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTMAXP_P_H
#define _B2D_TEXT_OTMAXP_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::MaxPTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Maximum Profile Table 'maxp'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6maxp.html
struct MaxPTable {
  enum : uint32_t {
    kTag = FontTag::maxp,
    kMinSize = 6
  };

  enum : uint32_t {
    kVersion0_5 = FontTag::make(0, 0, 5, 0),
    kVersion1_0 = FontTag::make(0, 1, 0, 0)
  };

  // --------------------------------------------------------------------------
  // [V0.5 - Must be used with CFF Glyphs (OpenType)]
  // --------------------------------------------------------------------------

  struct V0_5 {
    OTFixed version;                       //!< Version number in fixed point.
    OTUInt16 glyphCount;                   //!< The number of glyphs in the font.
  };

  // --------------------------------------------------------------------------
  // [V1.0 - Must be used with TT Glyphs (TrueType)]
  // --------------------------------------------------------------------------

  struct V1_0 : public V0_5 {
    OTUInt16 maxPoints;                    //!< Maximum points in a non-composite glyph.
    OTUInt16 maxContours;                  //!< Maximum contours in a non-composite glyph.
    OTUInt16 maxComponentPoints;           //!< Maximum points in a composite glyph.
    OTUInt16 maxComponentContours;         //!< Maximum contours in a composite glyph.
    OTUInt16 maxZones;                     //!< 1 if instructions do not use Z0, or 2 if instructions use Z0.
    OTUInt16 maxTwilightPoints;            //!< Maximum points used in Z0.
    OTUInt16 maxStorage;                   //!< Number of Storage Area locations.
    OTUInt16 maxFunctionDefs;              //!< Number of FDEFs.
    OTUInt16 maxInstructionDefs;           //!< Number of IDEFs.
    OTUInt16 maxStackElements;             //!< Maximum stack depth (CVT).
    OTUInt16 maxSizeOfInstructions;        //!< Maximum byte count for glyph instructions.
    OTUInt16 maxComponentElements;         //!< Maximum number of components referenced at "top level" for any composite glyph.
    OTUInt16 maxComponentDepth;            //!< Maximum levels of recursion; 1 for simple components.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const V0_5* v0_5() const noexcept { return fieldAt<V0_5>(this, 0); }
  B2D_INLINE const V1_0* v1_0() const noexcept { return fieldAt<V1_0>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  V0_5 header;
};
#pragma pack(pop)

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTMAXP_P_H
