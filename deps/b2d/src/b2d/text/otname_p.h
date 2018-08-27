// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTNAME_P_H
#define _B2D_TEXT_OTNAME_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::NameTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Naming Table 'name'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/name
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6name.html
struct NameTable {
  enum : uint32_t {
    kTag = FontTag::name,
    kMinSize = 6
  };

  //! Name ID stored in 'name' table.
  enum Id {
    kIdCopyrightNotice            = 0,
    kIdFamilyName                 = 1,
    kIdSubfamilyName              = 2,
    kIdUniqueIdentifier           = 3,
    kIdFullName                   = 4,
    kIdVersionString              = 5,
    kIdPostScriptName             = 6,
    kIdTrademark                  = 7,
    kIdManufacturerName           = 8,
    kIdDesignerName               = 9,
    kIdDescription                = 10,
    kIdVendorURL                  = 11,
    kIdDesignerURL                = 12,
    kIdLicenseDescription         = 13,
    kIdLicenseInfoURL             = 14,
    kIdReserved                   = 15,
    kIdTypographicFamilyName      = 16,
    kIdTypographicSubfamilyName   = 17,
    kIdCompatibleFullName         = 18,
    kIdSampleText                 = 19,
    kIdPostScriptCIDFindFontName  = 20,
    kIdWWSFamilyName              = 21,
    kIdWWSSubfamilyName           = 22,
    kIdLightBackgroundPalette     = 23,
    kIdDarkBackgroundPalette      = 24,
    kIdVariationsPostScriptPrefix = 25,

    kIdStandarizedNamesCount      = 26,
    kIdSpecificNamesStartIndex    = 255
  };

  struct NameRecord {
    OTUInt16 platformId;                 //!< Platform id.
    OTUInt16 specificId;                 //!< Specific id.
    OTUInt16 languageId;                 //!< Language id.
    OTUInt16 nameId;                     //!< Name id.
    OTUInt16 length;                     //!< String length in bytes.
    OTUInt16 offset;                     //!< String offset from start of storage area in bytes.
  };

  struct LangTagRecord {
    OTUInt16 length;                     //!< Language-tag string length (in bytes).
    OTUInt16 offset;                     //!< Language-tag string offset from start of storage area (in bytes).
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasLangTags() const noexcept { return format() >= 1; }

  //! The name records where count is the number of records.
  B2D_INLINE const NameRecord* nameRecords() const noexcept {
    return fieldAt<NameRecord>(this, 6);
  }

  B2D_INLINE uint16_t langTagCount(size_t recordCount) const noexcept {
    return fieldAt<OTUInt16>(this, sizeof(NameTable) + recordCount * sizeof(NameRecord))->value();
  }

  B2D_INLINE const LangTagRecord* langTagRecords(size_t recordCount) const noexcept {
    return fieldAt<LangTagRecord>(this, sizeof(NameTable) + recordCount * sizeof(NameRecord) + 2);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 format;                       //!< Format (supported formats are 0 and 1).
  OTUInt16 recordCount;                  //!< Number of name records.
  OTUInt16 stringOffset;                 //!< Offset to start of string storage.

  /*
  NameRecord nameRecords[count];
  OTUInt16 langTagCount;
  LangTagRecord langTagRecords[langTagCount];
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::NameImpl]
// ============================================================================

//! \internal
namespace NameImpl {
  Error initName(OTFaceImpl* faceI, FontData& fontData) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTNAME_P_H
