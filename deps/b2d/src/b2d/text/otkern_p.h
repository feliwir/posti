// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTKERN_P_H
#define _B2D_TEXT_OTKERN_P_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/array.h"
#include "../core/support.h"
#include "../text/glyphengine.h"
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::KernTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Kerning Table 'kern'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/kern
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kern.html
struct KernTable {
  enum : uint32_t {
    kTag = FontTag::kern,
    kMinSize = 4
  };

  struct WinTableHeader {
    OTUInt16 version;
    OTUInt16 tableCount;
  };

  struct MacTableHeader {
    OTFixed version;
    OTUInt32 tableCount;
  };

  struct WinGroupHeader {
    enum Coverage : uint8_t {
      kCoverageHorizontal   = 0x01u,
      kCoverageMinimum      = 0x02u,
      kCoverageCrossStream  = 0x04u,
      kCoverageOverride     = 0x08u,
      kCoverageReservedBits = 0xF0u
    };

    OTUInt16 version;
    OTUInt16 length;
    OTUInt8 format;
    OTUInt8 coverage;
  };

  struct MacGroupHeader {
    enum Coverage : uint8_t {
      kCoverageVertical     = 0x80u,
      kCoverageCrossStream  = 0x40u,
      kCoverageVariation    = 0x20u,
      kCoverageReservedBits = 0x1Fu
    };

    OTUInt32 length;
    OTUInt8 coverage;
    OTUInt8 format;
    OTUInt16 tupleIndex;
  };

  struct Pair {
    union {
      OTUInt32 combined;
      struct {
        OTUInt16 left;
        OTUInt16 right;
      };
    };
    OTInt16 value;
  };

  //! Format 0.
  struct Format0 {
    B2D_INLINE const Pair* pairArray() const noexcept { return fieldAt<Pair>(this, 8); }

    OTUInt16 pairCount;
    OTUInt16 searchRange;
    OTUInt16 entrySelector;
    OTUInt16 rangeShift;

    /*
    Pair pairArray[pairCount];
    */
  };

  //! Format 1.
  struct Format1 {
    struct StateHeader {
      OTUInt16 stateSize;
      OTUInt16 classTable;
      OTUInt16 stateArray;
      OTUInt16 entryTable;
    };

    enum ValueBits : uint16_t {
      kValueOffsetMask = 0x3FFFu,
      kValueNoAdvance  = 0x4000u,
      kValuePush       = 0x8000u
    };

    StateHeader stateHeader;
    OTUInt16 valueTable;
  };

  //! Format 2.
  struct Format2 {
    struct Table {
      B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 4); }

      OTUInt16 firstGlyph;
      OTUInt16 glyphCount;
      /*
      OTUInt16 offsetArray[glyphCount];
      */
    };

    OTUInt16 rowWidth;
    OTUInt16 leftClassTable;
    OTUInt16 rightClassTable;
    OTUInt16 kerningArray;
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  WinTableHeader header;
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::KernData]
// ============================================================================

//! \internal
//!
//! Kerning data stored in `OTFontFace` and used to perform kerning.
class KernData {
public:
  B2D_NONCOPYABLE(KernData)

  enum HeaderType : uint32_t {
    kHeaderNone    = 0,
    kHeaderMac     = 1,
    kHeaderWindows = 2
  };

  // Using the same bits as `KernTable::WinGroupHeader::Coverage`.
  enum Coverage : uint32_t {
    kCoverageHorizontal   = 0x01u,
    kCoverageMinimum      = 0x02u,
    kCoverageCrossStream  = 0x04u,
    kCoverageOverride     = 0x08u
  };

  struct Group {
    static B2D_INLINE Group makeLinked(uintptr_t dataOffset, uint32_t pairCount) noexcept { return Group { pairCount, false, { dataOffset } }; }
    static B2D_INLINE Group makeSynthesized(KernTable::Pair* pairs, uint32_t pairCount) noexcept { return Group { pairCount, true, { (uintptr_t)pairs } }; }

    B2D_INLINE uint32_t pairCount() const noexcept { return _pairCount; }
    B2D_INLINE uint32_t isSynthesized() const noexcept { return _synthesized; }

    B2D_INLINE const KernTable::Pair* pairs(const void* base) const noexcept {
      return isSynthesized() ? _dataPtr : static_cast<const KernTable::Pair*>(Support::advancePtr(base, _dataOffset));
    }

#if B2D_ARCH_BITS < 64
    uint32_t _pairCount : 31;
    uint32_t _synthesized : 1;
#else
    uint32_t _pairCount;
    uint32_t _synthesized;
#endif

    union {
      uintptr_t _dataOffset;
      KernTable::Pair* _dataPtr;
    };
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE KernData() noexcept
    : _format(0),
      _flags(0),
      _coverage(0),
      _reserved(0) {}
  B2D_INLINE ~KernData() noexcept { releaseGroups(); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    releaseGroups();
    _format = 0;
    _flags = 0;
    _coverage = 0;
    _groups.reset();
  }

  void releaseGroups() noexcept {
    size_t groupCount = _groups.size();
    for (size_t i = 0; i < groupCount; i++) {
      const Group& group = _groups[i];
      if (group.isSynthesized())
        Allocator::systemRelease(group._dataPtr);
    }
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _groups.empty(); }
  B2D_INLINE uint32_t format() const noexcept { return _format; }
  B2D_INLINE uint32_t flags() const noexcept { return _flags; }
  B2D_INLINE uint32_t coverage() const noexcept { return _coverage; }
  B2D_INLINE const Array<Group>& groups() const noexcept { return _groups; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _format;
  uint8_t _flags;
  uint8_t _coverage;
  uint8_t _reserved;
  Array<Group> _groups;
};

// ============================================================================
// [b2d::OT::KernImpl]
// ============================================================================

//! \internal
namespace KernImpl {
  Error initKern(OTFaceImpl* faceI, FontData& fontData) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTKERN_P_H
