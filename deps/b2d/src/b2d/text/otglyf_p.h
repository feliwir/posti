// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTGLYF_P_H
#define _B2D_TEXT_OTGLYF_P_H

// [Dependencies]
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../text/glyphoutlinedecoder.h"
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::LocaTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Index to Location Table 'loca'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/loca
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6loca.html
struct LocaTable {
  enum : uint32_t {
    kTag = FontTag::loca,

    //! Minimum size would be 2 records (4 bytes) if the font has only 1 glyph and uses 16-bit LOCA.
    kMinSize = 4
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const OTUInt16* offsetArray16() const noexcept { return fieldAt<OTUInt16>(this, 0); }
  B2D_INLINE const OTUInt32* offsetArray32() const noexcept { return fieldAt<OTUInt32>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  /*
  union {
    OTUInt16 offsetArray16[...];
    OTUInt32 offsetArray32[...];
  };
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::GlyfTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Glyph Data Table 'glyf' (TrueType).
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6glyf.html
struct GlyfTable {
  enum : uint32_t {
    kTag = FontTag::glyf,
    kMinSize = 10
  };

  struct Simple {
    enum Flags : uint8_t {
      kOnCurvePoint             = 0x01u,
      kXIsByte                  = 0x02u,
      kYIsByte                  = 0x04u,
      kRepeatFlag               = 0x08u,
      kXIsSameOrXByteIsPositive = 0x10u,
      kYIsSameOrYByteIsPositive = 0x20u,

      // We internally only keep flags within this mask.
      kImportantFlagsMask       = 0x3Fu
    };

    /*
    OTUInt16 endPtsOfContours[numberOfContours];
    OTUInt16 instructionLength;
    OTUInt8 instructions[instructionLength];
    OTUInt8 flags[...];
    OTUInt8/OTUInt16 xCoordinates[...];
    OTUInt8/OTUInt16 yCoordinates[...];
    */
  };

  struct Compound {
    enum Flags : uint16_t {
      kArgsAreWords             = 0x0001u,
      kArgsAreXYValues          = 0x0002u,
      kRoundXYToGrid            = 0x0004u,
      kWeHaveScale              = 0x0008u,
      kMoreComponents           = 0x0020u,
      kWeHaveScaleXY            = 0x0040u,
      kWeHave2x2                = 0x0080u,
      kWeHaveInstructions       = 0x0100u,
      kUseMyMetrics             = 0x0200u,
      kOverlappedCompound       = 0x0400u,
      kScaledComponentOffset    = 0x0800u,
      kUnscaledComponentOffset  = 0x1000u,

      kAnyCompoundScale         = kWeHaveScale
                                | kWeHaveScaleXY
                                | kWeHave2x2,

      kAnyCompoundOffset        = kScaledComponentOffset
                                | kUnscaledComponentOffset
    };

    /*
    OTUInt16 flags;
    OTUInt16 glyphId;
    Var arguments[...];
    Var transformations[...];
    */
  };

  struct GlyphData {
    OTInt16 numberOfContours;
    OTFWord xMin;
    OTFWord yMin;
    OTFWord xMax;
    OTFWord yMax;

    /*
    union {
      Simple simple;
      Compound compound;
    };
    */
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  /*
  GlyphData glyphData[...] // Indexed by LOCA.
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::GlyfImpl]
// ============================================================================

//! \internal
namespace GlyfImpl {
  Error initGlyf(OTFaceImpl* faceI, FontData& fontData) noexcept;

  Error B2D_CDECL getGlyphBoundingBoxes(
    const GlyphEngine* self,
    const GlyphId* glyphIds,
    size_t glyphIdAdvance,
    IntBox* boxes,
    size_t count) noexcept;

  Error B2D_CDECL decodeGlyph(
    GlyphOutlineDecoder* self,
    uint32_t glyphId,
    const Matrix2D& matrix,
    Path2D& out,
    MemBuffer& tmpBuffer,
    GlyphOutlineSinkFunc sink, size_t sinkGlyphIndex, void* closure) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTGLYF_P_H
