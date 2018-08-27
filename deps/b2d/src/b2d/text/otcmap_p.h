// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTCMAP_P_H
#define _B2D_TEXT_OTCMAP_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::CMapTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Character to glyph mapping table 'cmap'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/cmap
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6cmap.html
//!
//! NOTE: Some names inside this table do not match 1:1 specifications of Apple
//! and MS (they diverge as well). In general the naming was normalized to be
//! consistent in the following:
//!   - `first` - First character or glyph included in the set.
//!   - `last`  - Last character or glyph included in the set.
//!   - `end`   - First character or glyph excluded from the set.
//!   - `count` - Count of something, specifies a range of [first, first + count).
struct CMapTable {
  enum : uint32_t {
    kTag = FontTag::cmap,
    kMinSize = 4 + 8 // Header and one encoding record (just to read the header).
  };

  // --------------------------------------------------------------------------
  // [Encoding - Records That Follow CMAP Header]
  // --------------------------------------------------------------------------

  struct Encoding {
    OTUInt16 platformId;                 //!< Platform identifier.
    OTUInt16 encodingId;                 //!< Plaform-specific encoding identifier.
    OTUInt32 offset;                     //!< Offset of the mapping table.
  };

  // --------------------------------------------------------------------------
  // [Group of Chars - Used by Some Formats]
  // --------------------------------------------------------------------------

  struct Group {
    OTUInt32 first;                      //!< First character code.
    OTUInt32 last;                       //!< Last character code.
    OTUInt32 glyphId;                    //!< Initial glyphId for the group (or the only GlyphId if it's Format13).
  };

  // --------------------------------------------------------------------------
  // [Format 0 - Byte Encoding Table]
  // --------------------------------------------------------------------------

  struct Format0 {
    enum : uint32_t { kMinSize = 262 };

    OTUInt16 format;                     //!< Format number is set to 0.
    OTUInt16 length;                     //!< Table length in bytes.
    OTUInt16 language;                   //!< Language.
    OTUInt8 glyphIdArray[256];           //!< glyph index array.
  };

  // --------------------------------------------------------------------------
  // [Format 2 - High-Byte Mapping Through Table]
  // --------------------------------------------------------------------------

  struct Format2 {
    enum : uint32_t { kMinSize = 518 };

    struct SubHeader {
      OTUInt16 firstCode;
      OTUInt16 entryCount;
      OTInt16 idDelta;
      OTUInt16 idRangeOffset;
    };

    B2D_INLINE const SubHeader* subHeaderArray() const noexcept {
      return fieldAt<SubHeader>(this, sizeof(Format2));
    }

    B2D_INLINE const OTUInt16* glyphIdArray(size_t numSub) const noexcept {
      return fieldAt<OTUInt16>(this, sizeof(Format2) + numSub * sizeof(SubHeader));
    }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 2.
    OTUInt16 length;                     //!< Table length in bytes.
    OTUInt16 language;                   //!< Language.
    OTUInt16 glyphIndexArray[256];       //!< glyph index array.
    /*
    SubHeader subHeaderArray[numSub];
    OTUInt16 glyphIdArray[];
    */
  };

  // --------------------------------------------------------------------------
  // [Format 4 - Segment Mapping to Delta Values]
  // --------------------------------------------------------------------------

  struct Format4 {
    enum : uint32_t { kMinSize = 24 };

    B2D_INLINE const OTUInt16* lastCharArray() const noexcept { return fieldAt<OTUInt16>(this, sizeof(*this)); }
    B2D_INLINE const OTUInt16* firstCharArray(size_t numSeg) const noexcept { return fieldAt<OTUInt16>(this, sizeof(*this) + 2u + numSeg * 2u); }
    B2D_INLINE const OTUInt16* idDeltaArray(size_t numSeg) const noexcept { return fieldAt<OTUInt16>(this, sizeof(*this) + 2u + numSeg * 4u); }
    B2D_INLINE const OTUInt16* idOffsetArray(size_t numSeg) const noexcept { return fieldAt<OTUInt16>(this, sizeof(*this) + 2u + numSeg * 6u); }
    B2D_INLINE const OTUInt16* glyphIdArray(size_t numSeg) const noexcept { return fieldAt<OTUInt16>(this, sizeof(*this) + 2u + numSeg * 8u); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;
    OTUInt16 length;
    OTUInt16 macLanguageCode;
    OTUInt16 numSegX2;
    OTUInt16 searchRange;
    OTUInt16 entrySelector;
    OTUInt16 rangeShift;
    /*
    OTUInt16 lastCharArray[numSegs];
    OTUInt16 pad;
    OTUInt16 firstCharArray[numSegs];
    OTInt16 idDeltaArray[numSegs];
    OTUInt16 idOffsetArray[numSegs];
    OTUInt16 glyphIdArray[];
    */
  };

  // --------------------------------------------------------------------------
  // [Format 6 - Trimmed Table Mapping]
  // --------------------------------------------------------------------------

  struct Format6 {
    enum : uint32_t { kMinSize = 12 };

    B2D_INLINE const OTUInt16* glyphIdArray() const noexcept { return fieldAt<OTUInt16>(this, sizeof(Format6)); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 6.
    OTUInt16 length;                     //!< Table length in bytes.
    OTUInt16 language;                   //!< Language.
    OTUInt16 first;                      //!< First segment code.
    OTUInt16 count;                      //!< Segment size in bytes.
    /*
    OTUInt16 glyphIdArray[count];        //!< Glyph index array.
    */
  };

  // --------------------------------------------------------------------------
  // [Format 8 - Mixed 16-Bit and 32-Bit Coverage]
  // --------------------------------------------------------------------------

  //! This format is dead and it's not supported by Blend2D.
  struct Format8 {
    enum : uint32_t { kMinSize = 16 + 8192 };

    B2D_INLINE const Group* groupArray() const noexcept { return fieldAt<Group>(this, sizeof(Format8)); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 8.
    OTUInt16 reserved;                   //!< Reserved.
    OTUInt32 length;                     //!< Table length in bytes.
    OTUInt32 language;                   //!< Language.
    OTUInt8 is32[8192];                  //!< 32-bitness bitmap.
    OTUInt32 count;                      //!< Number of groupings which follow..
    /*
    Group groupArray[count];
    */
  };

  // --------------------------------------------------------------------------
  // [Format 10 - Trimmed Array]
  // --------------------------------------------------------------------------

  struct Format10 {
    enum : uint32_t { kMinSize = 20 };

    B2D_INLINE const OTUInt16* glyphIdArray() const noexcept { return fieldAt<OTUInt16>(this, sizeof(Format10)); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 10.
    OTUInt16 reserved;                   //!< Reserved.
    OTUInt32 length;                     //!< Table length in bytes.
    OTUInt32 language;                   //!< Language.
    OTUInt32 first;                      //!< First character in range.
    OTUInt32 count;                      //!< Count of characters in range.

    /*
    OTUInt16 glyphIdArray[count];
    */
  };

  // --------------------------------------------------------------------------
  // [Format 12 / 13 - Segmented Coverage / Many-To-One Range Mappings]
  // --------------------------------------------------------------------------

  struct Format12_13 {
    enum : uint32_t { kMinSize = 16 };

    B2D_INLINE const Group* groupArray() const noexcept { return fieldAt<Group>(this, sizeof(Format12_13)); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 12.
    OTUInt16 reserved;                   //!< Reserved.
    OTUInt32 length;                     //!< Table length in bytes.
    OTUInt32 language;                   //!< Language.
    OTUInt32 count;                      //!< Number of groups.

    /*
    Group groupArray[count];
    */
  };

  // --------------------------------------------------------------------------
  // [Format 14 - Unicode Variation Sequences]
  // --------------------------------------------------------------------------

  struct Format14 {
    enum : uint32_t { kMinSize = 10 };

    struct VarSelector {
      OTUInt24 varSelector;
      OTUInt32 defaultUVSOffset;
      OTUInt32 nonDefaultUVSOffset;
    };

    struct UnicodeRange {
      OTUInt24 startUnicodeValue;
      OTUInt8 additionalCount;
    };

    struct UVSMapping {
      OTUInt24 unicodeValue;
      OTUInt16 glyphId;
    };

    struct DefaultUVS {
      B2D_INLINE const UnicodeRange* rangeArray() const noexcept { return fieldAt<UnicodeRange>(this, sizeof(DefaultUVS)); }

      OTUInt32 numRanges;
      /*
      UnicodeRange rangeArray[numRanges];
      */
    };

    struct NonDefaultUVS {
      B2D_INLINE const UVSMapping* uvsMappingArray() const noexcept { return fieldAt<UVSMapping>(this, sizeof(NonDefaultUVS)); }

      OTUInt32 numMappings;
      /*
      UVSMapping uvsMappingArray[numMappings];
      */
    };

    B2D_INLINE const VarSelector* varSelectorArray() const noexcept {
      return fieldAt<VarSelector>(this, sizeof(Format14));
    }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    OTUInt16 format;                     //!< Format number is set to 14.
    OTUInt32 length;                     //!< Table length in bytes.
    OTUInt32 numVarSelectorRecords;      //!< Number of variation selector records.

    /*
    VariationSelector varSelectorArray[numVarSelectorRecords];
    */
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Encoding* encodingArray() const noexcept { return fieldAt<Encoding>(this, sizeof(CMapTable)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 version;                      //!< Version of 'cmap' header, must be 0.
  OTUInt16 encodingCount;                //!< Count of 'cmap' encoding records.
  /*
  Encoding encodingArray[encodingCount];
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::CMapData]
// ============================================================================

//! \internal
//!
//! Character to glyph mapping data.
//!
//! This structure contains some data extracted from 'cmap' table that is essential
//! for Blend2D for converting characters to glyph ids.
struct CMapData {
  enum Flags : uint32_t {
    kFlagInitialized    = 0x00000100u    //!< CMAP Info/Context has been initialized and can be used.
  };

  // ------------------------------------------------------------------------
  // [Accessors]
  // ------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0, 0, 0, FontDataRange { 0, 0 }); }

  B2D_INLINE void reset(uint32_t format, uint32_t flags, uint32_t entryCount, const FontDataRange& dataRange) noexcept {
    _format = format;
    _flags = flags;
    _entryCount = entryCount;
    _dataRange = dataRange;
  }

  B2D_INLINE bool empty() const noexcept { return _flags == 0; }
  B2D_INLINE uint32_t flags() const noexcept { return _flags; }
  B2D_INLINE uint32_t format() const noexcept { return _format; }
  B2D_INLINE uint32_t entryCount() const noexcept { return _entryCount; }
  B2D_INLINE const FontDataRange& dataRange() const noexcept { return _dataRange; }

  // ------------------------------------------------------------------------
  // [Members]
  // ------------------------------------------------------------------------

  uint16_t _flags;                       //!< Flags.
  uint16_t _format;                      //!< Format of primary cmap encoding used.
  uint32_t _entryCount;                  //!< Count of entries in the selected "cmap" encoding.
  FontDataRange _dataRange;              //!< SubTable range (offset and length).
};

// ============================================================================
// [b2d::OT::CMapImpl]
// ============================================================================

//! \internal
namespace CMapImpl {
  //! Find the best encoding in the provided `cMap` and store this information
  //! into the given `faceI` instance. On success both `CMapIndex` and mapping
  //! related functions will be initialized.
  Error initCMap(OTFaceImpl* faceI, FontData& fontData) noexcept;

  //! Validate a CMapTable::Encoding subtable of any format specified by `dataRegion`.
  //! On success a valid `CMapData` data is stored in `cMapInfo`, otherwise an
  //! is returned and `cMapInfo` is kept unmodified.
  Error validateEncoding(FontDataRegion dataRegion, uint32_t dataOffset, CMapData& cMapInfo) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTCMAP_P_H
