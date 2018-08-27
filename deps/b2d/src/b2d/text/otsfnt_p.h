// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTSFNT_P_H
#define _B2D_TEXT_OTSFNT_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::SFNTHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! SFNT Header.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/font-file
struct SFNTHeader {
  enum : uint32_t {
    kTag = FontTag::sfnt,
    kMinSize = 12
  };

  enum VersionTag : uint32_t {
    kVersionTagOpenType   = FontTag::make('O', 'T', 'T', 'O'),
    kVersionTagTrueTypeA  = FontTag::make( 0,   1 ,  0 ,  0 ),
    kVersionTagTrueTypeB  = FontTag::make('t', 'r', 'u', 'e'),
    kVersionTagType1      = FontTag::make('t', 'y', 'p', '1')
  };

  struct TableRecord {
    OTTag tag;
    OTCheckSum checkSum;
    OTUInt32 offset;
    OTUInt32 length;
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const TableRecord* tableRecords() const noexcept { return fieldAt<TableRecord>(this, sizeof(SFNTHeader)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTTag versionTag;
  OTUInt16 numTables;
  OTUInt16 searchRange;
  OTUInt16 entrySelector;
  OTUInt16 rangeShift;
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::TTCHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! TTC header.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/font-file
struct TTCHeader {
  enum : uint32_t {
    kTag = FontTag::make('T', 'T', 'C', 'F'),
    kMinSize = 12,
    kMaxFonts = 65536
  };

  B2D_INLINE size_t calcSize(uint32_t numFonts) const noexcept {
    uint32_t headerSize = sizeof(TTCHeader);

    if (numFonts > kMaxFonts)
      return 0;

    if (version() >= 0x00020000u)
      headerSize += 12;

    return headerSize + numFonts * 4;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const OTUInt32* offsetArray() const noexcept { return fieldAt<OTUInt32>(this, sizeof(TTCHeader)); }

  // --------------------------------------------------------------------------
  // [Version 1.0]
  // --------------------------------------------------------------------------

  OTUInt32 ttcTag;
  OTFixed version;
  OTUInt32 numFonts;
  /*
  OTUInt32 offsetArray[numFonts];
  */

  // --------------------------------------------------------------------------
  // [Version 2.0]
  // --------------------------------------------------------------------------

  /*
  OTUInt32 dsigTag;
  OTUInt32 dsigLength;
  OTUInt32 dsigOffset;
  */
};
#pragma pack(pop)

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTSFNT_P_H
