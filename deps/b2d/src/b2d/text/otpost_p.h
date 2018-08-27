// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTPOST_P_H
#define _B2D_TEXT_OTPOST_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::PostTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! PostScript Table.
//!
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/post
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6Post.html
struct PostTable {
  enum : uint32_t {
    kTag = FontTag::post,
    kMinSize = 32
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTFixed version;
  OTFixed italicAngle;
  OTInt16 underlinePosition;
  OTInt16 underlineThickness;
  OTUInt32 isFixedPitch;
  OTUInt32 minMemType42;
  OTUInt32 maxMemType42;
  OTUInt32 minMemType1;
  OTUInt32 maxMemType1;
};
#pragma pack(pop)

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTPOST_P_H
