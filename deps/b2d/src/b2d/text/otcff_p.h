// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTCFF_P_H
#define _B2D_TEXT_OTCFF_P_H

// [Dependencies]
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/glyphoutlinedecoder.h"
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

//! \internal
//! \{

// ============================================================================
// [b2d::OT::CFFTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Compact Font Format (CFF) Table.
//!
//! The structure of CFF File looks like this:
//!   - Header
//!   - Name INDEX
//!   - TopDict INDEX
//!   - String INDEX
//!   - GSubR INDEX
//!   - Encodings
//!   - Charsets
//!   - FDSelect
//!   - CharStrings INDEX   <- [get offset from 'TopDict.CharStrings']
//!   - FontDict INDEX
//!   - PrivateDict         <- [get offset+size from 'TopDict.Private']
//!   - LSubR INDEX
//!   - Copyright and trademark notices
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/cff
//!   - http://wwwimages.adobe.com/www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5176.CFF.pdf
//!   - http://wwwimages.adobe.com/www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5177.Type2.pdf
//!
//! NOTE 1: The term `VarOffset` that is used inside CFF code means that the
//! offset size is variable and must be previously specified by an `offsetSize`
//! field.
//!
//! NOTE 2: Many enums inside this structure are just for reference purposes.
//! They would be useful if we want to implement support for RAW PostScript
//! fonts (CFF) that are not part of OpenType.
struct CFFTable {
  enum : uint32_t {
    kTag = FontTag::CFF,
    kMinSize = 4
  };

  enum : uint32_t {
    kOffsetAdjustment = 1
  };

  enum CharsetId : uint32_t {
    kCharsetIdISOAdobe           = 0,
    kCharsetIdExpert             = 1,
    kCharsetIdExpertSubset       = 2
  };

  enum DictOp : uint32_t {
    // Escape.
    kEscapeDictOp                = 0x0C,

    // Top Dict Operator Entries.
    kDictOpTopVersion            = 0x0000,
    kDictOpTopNotice             = 0x0001,
    kDictOpTopFullName           = 0x0002,
    kDictOpTopFamilyName         = 0x0003,
    kDictOpTopWeight             = 0x0004,
    kDictOpTopFontBBox           = 0x0005,
    kDictOpTopUniqueId           = 0x000D,
    kDictOpTopXUID               = 0x000E,
    kDictOpTopCharset            = 0x000F, // Default 0.
    kDictOpTopEncoding           = 0x0010, // Default 0.
    kDictOpTopCharStrings        = 0x0011,
    kDictOpTopPrivate            = 0x0012,

    kDictOpTopCopyright          = 0x0C00,
    kDictOpTopIsFixedPitch       = 0x0C01, // Default 0 (false).
    kDictOpTopItalicAngle        = 0x0C02, // Default 0.
    kDictOpTopUnderlinePosition  = 0x0C03, // Default -100.
    kDictOpTopUnderlineThickness = 0x0C04, // Default 50.
    kDictOpTopPaintType          = 0x0C05, // Default 0.
    kDictOpTopCharstringType     = 0x0C06, // Default 2.
    kDictOpTopFontMatrix         = 0x0C07, // Default [0.001 0 0 0.001 0 0].
    kDictOpTopStrokeWidth        = 0x0C08, // Default 0.
    kDictOpTopSyntheticBase      = 0x0C14,
    kDictOpTopPostScript         = 0x0C15,
    kDictOpTopBaseFontName       = 0x0C16,
    kDictOpTopBaseFontBlend      = 0x0C17,

    // CIDFont Operator Extensions:
    kDictOpTopROS                = 0x0C1E,
    kDictOpTopCIDFontVersion     = 0x0C1F, // Default 0.
    kDictOpTopCIDFontRevision    = 0x0C20, // Default 0.
    kDictOpTopCIDFontType        = 0x0C21, // Default 0.
    kDictOpTopCIDCount           = 0x0C22, // Default 8720.
    kDictOpTopUIDBase            = 0x0C23,
    kDictOpTopFDArray            = 0x0C24,
    kDictOpTopFDSelect           = 0x0C25,
    kDictOpTopFontName           = 0x0C26,

    // Private Dict Operator Entries.
    kDictOpPrivBlueValues        = 0x0006,
    kDictOpPrivOtherBlues        = 0x0007,
    kDictOpPrivFamilyBlues       = 0x0008,
    kDictOpPrivFamilyOtherBlues  = 0x0009,
    kDictOpPrivStdHW             = 0x000A,
    kDictOpPrivStdVW             = 0x000B,
    kDictOpPrivSubrs             = 0x0013,
    kDictOpPrivDefaultWidthX     = 0x0014, // Default 0.
    kDictOpPrivNominalWidthX     = 0x0015, // Default 0.

    kDictOpPrivBlueScale         = 0x0C09, // Default 0.039625.
    kDictOpPrivBlueShift         = 0x0C0A, // Default 7.
    kDictOpPrivBlueFuzz          = 0x0C0B, // Default 1.
    kDictOpPrivStemSnapH         = 0x0C0C,
    kDictOpPrivStemSnapV         = 0x0C0D,
    kDictOpPrivForceBold         = 0x0C0E, // Default 0 (false).
    kDictOpPrivLanguageGroup     = 0x0C11, // Default 0.
    kDictOpPrivExpansionFactor   = 0x0C12, // Default 0.06.
    kDictOpPrivInitialRandomSeed = 0x0C13  // Default 0.
  };

  struct Header {
    OTUInt8 majorVersion;
    OTUInt8 minorVersion;
    OTUInt8 headerSize;
  };

  struct HeaderV1 : public Header {
    OTUInt8 offsetSize;
  };

  struct HeaderV2 : public Header {
    OTUInt16 topDictLength;
  };

  //! Index table (v1).
  struct IndexV1 {
    enum : uint32_t {
      //! NOTE: An empty Index is represented by a `count` field with a 0 value
      //! and no additional fields, thus, the total size of bytes required by a
      //! zero index is 2.
      kMinSize = 2
    };

    B2D_INLINE const uint8_t* offsetArray() const noexcept { return fieldAt<uint8_t>(this, 3); }

    OTUInt16 count;
    OTUInt8 offsetSize;

    /*
    Offset offsetArray[count + 1];
    OTUInt8 data[...];
    */
  };

  //! Index table (v2).
  struct IndexV2 {
    enum : uint32_t {
      //! NOTE: An empty Index is represented by a `count` field with a 0 value
      //! and no additional fields, thus, the total size of bytes required by a
      //! zero index is 4.
      kMinSize = 4
    };

    B2D_INLINE const uint8_t* offsetArray() const noexcept { return fieldAt<uint8_t>(this, 5); }

    OTUInt32 count;
    OTUInt8 offsetSize;

    /*
    Offset offsetArray[count + 1];
    OTUInt8 data[...];
    */
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const HeaderV1* headerV1() const noexcept { return fieldAt<HeaderV1>(this, 0); }
  B2D_INLINE const HeaderV2* headerV2() const noexcept { return fieldAt<HeaderV2>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Header header;
};

#pragma pack(pop)

// ============================================================================
// [b2d::OT::CFFIndex]
// ============================================================================

struct CFFIndex {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t offsetAt(size_t index) const noexcept {
    B2D_ASSERT(index <= count);
    return OTUtils::readOffset(offsets + index * offsetSize, offsetSize) - CFFTable::kOffsetAdjustment;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t count;
  uint8_t headerSize;
  uint8_t offsetSize;
  uint16_t reserved;
  uint32_t payloadSize;
  uint32_t totalSize;
  const uint8_t* offsets;
  const uint8_t* payload;
};

// ============================================================================
// [b2d::OT::CFFDictEntry]
// ============================================================================

struct CFFDictEntry {
  enum : uint32_t {
    kValueCapacity = 48
  };

  B2D_INLINE bool isFpValue(uint32_t index) const noexcept {
    return (fpMask & (uint64_t(1) << index)) != 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t op;
  uint32_t count;
  uint64_t fpMask;
  double values[kValueCapacity];
};

// ============================================================================
// [b2d::OT::CFFDictIterator]
// ============================================================================

class CFFDictIterator {
public:
  B2D_INLINE CFFDictIterator() noexcept
    : _dataPtr(nullptr),
      _dataEnd(nullptr) {}

  B2D_INLINE CFFDictIterator(const uint8_t* data, size_t size) noexcept
    : _dataPtr(data),
      _dataEnd(data + size) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const uint8_t* data, size_t size) noexcept {
    _dataPtr = data;
    _dataEnd = data + size;
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasNext() const noexcept {
    return _dataPtr != _dataEnd;
  }

  Error next(CFFDictEntry& entry) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const uint8_t* _dataPtr;
  const uint8_t* _dataEnd;
};

// ============================================================================
// [b2d::OT::CFFInfo]
// ============================================================================

struct CFFInfo {
  //! CFF index id.
  enum IndexId : uint32_t {
    kIndexGSubR      = 0,
    kIndexLSubR      = 1,
    kIndexCharString = 2,
    kIndexCount      = 3
  };

  //! CFF index.
  struct Index {
    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    B2D_INLINE void reset(uint32_t dataOffset, uint32_t dataSize, uint32_t headerSize, uint32_t offsetSize, uint32_t entryCount) noexcept {
      _dataRange.reset(dataOffset, dataSize);
      _entryCount = entryCount;
      _headerSize = uint8_t(headerSize);
      _offsetSize = uint8_t(offsetSize);
      _reserved = 0;
    }

    constexpr FontDataRange dataRange() const noexcept { return _dataRange; }
    constexpr uint32_t dataOffset() const noexcept { return _dataRange.offset(); }
    constexpr uint32_t dataSize() const noexcept { return _dataRange.length(); }

    //! Get a size of the header.
    constexpr uint32_t headerSize() const noexcept { return _headerSize; }
    //! Get a size of a single offset [in bytes], could be [1..4].
    constexpr uint32_t offsetSize() const noexcept { return _offsetSize; }
    //! Get the number of entries stored in this index.
    constexpr uint32_t entryCount() const noexcept { return _entryCount; }

    //! Get an offset to the offsets data (array of offsets).
    constexpr uint32_t offsetsOffset() const noexcept { return headerSize(); }
    //! get a size of offset data (array of offsets) in bytes.
    constexpr uint32_t offsetsSize() const noexcept { return (entryCount() + 1) * offsetSize(); }

    //! Get an offset to the payload data.
    constexpr uint32_t payloadOffset() const noexcept { return offsetsOffset() + offsetsSize(); }
    //! Get a payload size in bytes.
    constexpr uint32_t payloadSize() const noexcept { return dataSize() - payloadOffset(); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    FontDataRange _dataRange;
    uint32_t _entryCount;
    uint8_t _headerSize;
    uint8_t _offsetSize;
    uint16_t _reserved;
  };

  Index _index[kIndexCount];             //!< GSubR, LSubR, and CharString indexes.
  uint16_t _bias[2];                     //!< GSubR, LSubR biases.
};

// ============================================================================
// [b2d::OT::CFFImpl]
// ============================================================================

namespace CFFImpl {
  Error readFloat(const uint8_t* p, const uint8_t* pEnd, double& valueOut, size_t& valueSizeInBytes) noexcept;
  Error readIndex(const void* data, size_t size, uint32_t version, CFFIndex* indexOut) noexcept;
  Error initCFF(OTFaceImpl* faceI, FontDataRegion cffData, uint32_t cffVersion) noexcept;

  Error B2D_CDECL decodeGlyph(
    GlyphOutlineDecoder* self,
    uint32_t glyphId,
    const Matrix2D& matrix,
    Path2D& out,
    MemBuffer& tmpBuffer,
    GlyphOutlineSinkFunc sink, size_t sinkGlyphIndex, void* closure) noexcept;
}

//! \}
//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTCFF_P_H
