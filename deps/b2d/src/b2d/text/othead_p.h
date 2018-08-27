// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTHEAD_P_H
#define _B2D_TEXT_OTHEAD_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::HeadTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Font Header Table 'head'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/head
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6head.html
struct HeadTable {
  enum : uint32_t {
    kTag = FontTag::head,
    kMinSize = 54
  };

  enum : uint32_t {
    kCheckSumAdjustment      = FontTag::make(0xB1, 0xB0, 0xAF, 0xBA),
    kMagicNumber             = FontTag::make(0x5F, 0x0F, 0x3C, 0xF5)
  };

  enum Flags : uint16_t {
    kFlagBaselineYEquals0    = 0x0001u,
    kFlagLSBPointXEquals0    = 0x0002u,
    kFlagInstDependOnPtSize  = 0x0004u,
    kFlagForcePPEMToInteger  = 0x0008u,
    kFlagInstMayAlterAW      = 0x0010u,
    kFlagLossLessData        = 0x0800u,
    kFlagConvertedFont       = 0x1000u,
    kFlagClearTypeOptimized  = 0x2000u,
    kFlagLastResortFont      = 0x4000u
  };

  enum MacStyle : uint16_t {
    kMacStyleBold            = 0x0001u,
    kMacStyleItalic          = 0x0002u,
    kMacStyleUnderline       = 0x0004u,
    kMacStyleOutline         = 0x0008u,
    kMacStyleShadow          = 0x0010u,
    kMacStyleCondensed       = 0x0020u,
    kMacStyleExtended        = 0x0040u,
    kMacStyleReservedBits    = 0xFF70u
  };

  enum IndexToLocFormat : uint16_t {
    kIndexToLocUInt16        = 0,
    kIndexToLocUInt32        = 1
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTFixed version;
  OTFixed revision;

  OTUInt32 checkSumAdjustment;
  OTUInt32 magicNumber;
  OTUInt16 flags;
  OTUInt16 unitsPerEm;

  OTDateTime created;
  OTDateTime modified;

  OTInt16 xMin;
  OTInt16 yMin;
  OTInt16 xMax;
  OTInt16 yMax;

  OTUInt16 macStyle;
  OTUInt16 lowestRecPPEM;

  OTInt16 fontDirectionHint;
  OTInt16 indexToLocFormat;
  OTInt16 glyphDataFormat;
};
#pragma pack(pop)

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTHEAD_P_H
