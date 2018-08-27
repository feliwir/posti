// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTLAYOUT_P_H
#define _B2D_TEXT_OTLAYOUT_P_H

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
// [b2d::OT::LanguageSystemTable]
// ============================================================================

#pragma pack(push, 1)
struct LanguageSystemRecord {
  OTTag tag;
  OTUInt16 offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct LanguageSystemTable {
  enum : uint32_t { kMinSize = 6 };

  B2D_INLINE const OTUInt16* featureIndexArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

  OTUInt16 lookupOrderOffset;
  OTUInt16 requiredFeatureIndex;
  OTUInt16 featureIndexCount;
  /*
  OTUInt16 featureIndexArray[featureIndexCount];
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::ScriptTable]
// ============================================================================

#pragma pack(push, 1)
struct ScriptRecord {
  OTTag tag;
  OTUInt16 scriptOffset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ScriptList {
  enum : uint32_t { kMinSize = 2 };

  B2D_INLINE const ScriptRecord* records() const noexcept { return fieldAt<ScriptRecord>(this, 2); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 count;
  /*
  ScriptRecord records[count];
  */
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ScriptTable {
  enum : uint32_t { kMinSize = 4 };

  B2D_INLINE const LanguageSystemRecord* records() const noexcept { return fieldAt<LanguageSystemRecord>(this, 4); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 defaultLangSys;
  OTUInt16 langSysCount;
  /*
  LanguageSystemRecord langSysRecords[count];
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::FeatureTable]
// ============================================================================

#pragma pack(push, 1)
struct FeatureTable {
  enum : uint32_t { kMinSize = 4 };

  struct Record {
    OTTag featureTag;
    OTUInt16 featureOffset;
  };

  struct List {
    B2D_INLINE const Record* records() const noexcept { return fieldAt<Record>(this, 2); }

    OTUInt16 count;
    /*
    Record records[count];
    */
  };

  B2D_INLINE const OTUInt16* lookupIndexArray() const noexcept { return fieldAt<OTUInt16>(this, 4); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 featureParams;
  OTUInt16 lookupIndexCount;
  /*
  OTUInt16 lookupIndexArray;
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::LookupTable]
// ============================================================================

#pragma pack(push, 1)
struct LookupList {
  enum : uint32_t { kMinSize = 2 };

  B2D_INLINE const OTUInt16* lookupArray() const noexcept { return fieldAt<OTUInt16>(this, 2); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 lookupCount;                  //!< Number of lookups in this table.
  /*
  OTUInt16 lookupArray;                  //!< Array of offsets to lookup tables.
  */
};
#pragma pack(pop)

#pragma pack(push, 1)
struct LookupTable {
  enum : uint32_t { kMinSize = 6 };

  enum Flags : uint8_t {
    kFlagRightToLeft         = 0x01u,    //!< Relates only to the correct processing of the cursive attachment lookup type (GPOS lookup type 3).
    kFlagIgnoreBaseGlyphs    = 0x02u,    //!< Skips over base glyphs.
    kFlagIgnoreLigatures     = 0x04u,    //!< Skips over ligatures.
    kFlagIgnoreMarks         = 0x08u,    //!< Skips over all combining marks.
    kFlagUseMarkFilteringSet = 0x10u,    //!< Indicates that the lookup table structure is followed by a `markFilteringSet` field.
    kFlagReserved            = 0xE0u     //!< Must be zero.
  };

  B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 lookupType;                   //!< See `GPosTable::LookupType` and `GSubTable::LookupType`.
  OTUInt8 markAttachmentType;            //!< If non-zero, skips over all marks of attachment type different from specified.
  OTUInt8 flags;                         //!< Lookup qualifiers.
  OTUInt16 lookupCount;                  //!< Number of subtables for this lookup.
  /*
  OTUInt16 offsetArray[subTableCount];   //!< Array of offsets to lookup subtables, from the beginning of LookupTable.
  OTUInt16 markFilteringSet;             //!< Index (base 0) into GDEF mark glyph sets structure.
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::CoverageTable]
// ============================================================================

#pragma pack(push, 1)
struct CoverageTable {
  enum : uint32_t { kMinSize = 4 };

  struct RangeRecord {
    OTUInt16 firstGlyph;                 //!< First glyph id in the range.
    OTUInt16 lastGlyph;                  //!< Last glyph id in the range.
    OTUInt16 startCoverageIndex;         //!< Coverage index of first glyph id in range.
  };

  struct Format1 {
    enum : uint32_t { kMinSize = 4 };

    B2D_INLINE const OTUInt16* glyphArray() const noexcept { return fieldAt<OTUInt16>(this, 4); }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    OTUInt16 format;                     //!< Coverage table format (must be 1).
    OTUInt16 glyphCount;                 //!< Count of entries in `glyphArray`.
    /*
    OTUInt16 glyphArray[glyphCount];     //!< Array of GlyphIds — in numerical order.
    */
  };

  struct Format2 {
    enum : uint32_t { kMinSize = 4 };

    B2D_INLINE const RangeRecord* rangeArray() const noexcept { return fieldAt<RangeRecord>(this, 4); }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    OTUInt16 format;                     //!< Coverage table format (must be 2).
    OTUInt16 rangeCount;                 //!< Count of `RangeRecord` entries.
    /*
    RangeRecord rangeArray[rangeCount];  //!< Array of `RangeRecord` entries.
    */
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Format1* format1() const noexcept { return fieldAt<Format1>(this, 0); }
  B2D_INLINE const Format2* format2() const noexcept { return fieldAt<Format2>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 format;                       //!< Coverage table format.
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::ClassDefinitionTable]
// ============================================================================

#pragma pack(push, 1)
struct ClassDefinitionTable {
  enum : uint32_t { kMinSize = 4 };

  struct RangeRecord {
    OTUInt16 firstGlyph;                 //!< First glyph ID in the range.
    OTUInt16 lastGlyph;                  //!< Last glyph ID in the range.
    OTUInt16 classValue;                 //!< Applied to all glyphs in the range.
  };

  struct Format1 {
    enum : uint32_t { kMinSize = 6 };

    B2D_INLINE const OTUInt16* classValueArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    OTUInt16 format;                     //!< Class definition table format (must be 1).
    OTUInt16 firstGlyph;                 //!< First glyph id of the `classValueArray`.
    OTUInt16 glyphCount;                 //!< Number of entries in `classValueArray`.
    /*
    OTUInt16 classValueArray[glyphCount];//!< Array of class values, one per glyph.
    */
  };

  struct Format2 {
    enum : uint32_t { kMinSize = 4 };

    B2D_INLINE const RangeRecord* rangeArray() const noexcept { return fieldAt<RangeRecord>(this, 4); }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    OTUInt16 format;                     //!< Class definition table format (must be 2).
    OTUInt16 rangeCount;                 //!< Number of RangeRecord entries.
    /*
    RangeRecord rangeArray[rangeCount];  //!< Array of RangeRecord entries ordered by `RangeRecord::firstGlyph`.
    */
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Format1* format1() const noexcept { return fieldAt<Format1>(this, 0); }
  B2D_INLINE const Format2* format2() const noexcept { return fieldAt<Format2>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 format;                       //!< Class definition table format.
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::ConditionTable]
// ============================================================================

#pragma pack(push, 1)
struct ConditionTable {
  enum : uint32_t { kMinSize = 2 };

  struct Format1 {
    enum : uint32_t { kMinSize = 8 };

    OTUInt16 format;                     //!< Condition table format (must be 1).
    OTUInt16 axisIndex;                  //!< Index (zero-based) for the variation axis within the 'fvar' table.
    OTF2Dot14 filterRangeMinValue;       //!< Minimum value of the font variation instances that satisfy this condition.
    OTF2Dot14 filterRangeMaxValue;       //!< Maximum value of the font variation instances that satisfy this condition.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Format1* format1() const noexcept { return fieldAt<Format1>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTUInt16 format;                       //!< Condition table format.
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::GDefTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Glyph Definition Table 'GDEF'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/gdef
struct GDefTable {
  enum : uint32_t {
    kTag = FontTag::GDEF,
    kMinSize = 12
  };

  struct HeaderV1_0 {
    enum : uint32_t { kMinSize = 12 };

    OTFixed version;
    OTUInt16 classDefinitionOffset;
    OTUInt16 attachListOffset;
    OTUInt16 ligCaretListOffset;
    OTUInt16 markAttachClassDefOffset;
  };

  struct HeaderV1_2 : public HeaderV1_0 {
    enum : uint32_t { kMinSize = 14 };

    OTUInt16 markGlyphSetsDefOffset;
  };

  struct HeaderV1_3 : public HeaderV1_2 {
    enum : uint32_t { kMinSize = 18 };

    OTUInt32 itemVarStoreOffset;
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const HeaderV1_0* v1_0() const noexcept { return &header; }
  B2D_INLINE const HeaderV1_2* v1_2() const noexcept { return fieldAt<HeaderV1_2>(this, 0); }
  B2D_INLINE const HeaderV1_3* v1_3() const noexcept { return fieldAt<HeaderV1_3>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  HeaderV1_0 header;
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::GSubTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Glyph Substitution Table 'GSUB'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/gsub
//!   - https://fontforge.github.io/gposgsub.html
struct GSubTable {
  enum : uint32_t {
    kTag = FontTag::GSUB,
    kMinSize = 10
  };

  enum LookupType : uint8_t {
    kLookupSingle                 = 1,   //!< Replace one glyph with one glyph.
    kLookupMultiple               = 2,   //!< Replace one glyph with more than one glyph.
    kLookupAlternate              = 3,   //!< Replace one glyph with one of many glyphs.
    kLookupLigature               = 4,   //!< Replace multiple glyphs with one glyph.
    kLookupContext                = 5,   //!< Replace one or more glyphs in context.
    kLookupChainingContext        = 6,   //!< Replace one or more glyphs in chained context.
    kLookupExtensionSubstitution  = 7,   //!< Extension mechanism for other substitutions.
    kLookupReverseChainingContext = 8    //!< Applied in reverse order, replace single glyph in chaining context.
  };

  // --------------------------------------------------------------------------
  // [Header]
  // --------------------------------------------------------------------------

  struct HeaderV1_0 {
    enum : uint32_t { kMinSize = 10 };

    OTFixed version;
    OTUInt16 scriptListOffset;
    OTUInt16 featureListOffset;
    OTUInt16 lookupListOffset;
  };

  struct HeaderV1_1 : public HeaderV1_0 {
    enum : uint32_t { kMinSize = 14 };

    OTUInt32 featureVariationsOffset;
  };

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  struct SubstHeader {
    OTUInt16 format;                     //!< Format identifier.
  };

  struct SubstLookupRecord {
    OTUInt16 glyphSequenceIndex;         //!< Index into current glyph sequence (first glyph == 0).
    OTUInt16 lookupListIndex;            //!< Lookup to apply to that position (zero-based index).
  };

  struct OffsetTable {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 2); }

    OTUInt16 offsetCount;                //!< Count of entries in `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets.
    */
  };

  // --------------------------------------------------------------------------
  // [SingleSubst {1}]
  // --------------------------------------------------------------------------

  struct SingleSubst1 : public SubstHeader {
    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTInt16 deltaGlyphId;                //!< Add to original glyph id to get substitute glyph id.
  };

  struct SingleSubst2 : public SubstHeader {
    B2D_INLINE const OTUInt16* glyphIdArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 glyphIdCount;               //!< Number of glyph IDs in the `glyphIds` array.
    /*
    OTUInt16 glyphIdArray[glyphIdCount]; //!< Array of substitute glyph ids — ordered by coverage index.
    */
  };

  // --------------------------------------------------------------------------
  // [MultipleSubst {2}]
  // --------------------------------------------------------------------------

  struct Sequence {
    B2D_INLINE const OTUInt16* glyphArray() const noexcept { return fieldAt<OTUInt16>(this, 2); }

    OTUInt16 glyphCount;                 //!< Count of entries in `glyphArray`, must be greater than 0.
    /*
    OTUInt16 glyphArray[glyphCount];     //!< String of glyph ids to substitute.
    */
  };

  struct MultipleSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Number of Sequence table offsets in the `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to sequence tables, ordered by coverage index.
    */
  };

  // --------------------------------------------------------------------------
  // [AlternateSubst {3}]
  // --------------------------------------------------------------------------

  struct AlternateSet {
    B2D_INLINE const OTUInt16* glyphArray() const noexcept { return fieldAt<OTUInt16>(this, 2); }

    OTUInt16 glyphCount;                 //!< Count of entries in `glyphArray`, must be greater than 0.
    /*
    OTUInt16 glyphArray[glyphCount];     //!< Array of alternate glyph ids, in arbitrary order.
    */
  };

  struct AlternateSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Number of AlternateSet tables.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to AlternateSet tables, ordered by coverage index.
    */
  };

  // --------------------------------------------------------------------------
  // [LigatureSubst {4}]
  // --------------------------------------------------------------------------

  struct Ligature {
    B2D_INLINE const OTUInt16* glyphArray() const noexcept { return fieldAt<OTUInt16>(this, 4); }

    OTUInt16 ligatureGlyphId;            //!< Glyph id of ligature to substitute.
    OTUInt16 glyphCount;                 //!< Count of entries in `glyphArray`.
    /*
    OTUInt16 glyphArray[glyphCount];     //!< Array of component glyph ids — start with the second component, ordered in writing direction
    */
  };

  typedef OffsetTable LigatureSet;

  struct LigatureSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Count of entries in `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to LigatureSet tables.
    */
  };

  // --------------------------------------------------------------------------
  // [ContextSubst {5}]
  // --------------------------------------------------------------------------

  struct SubRule {
    B2D_INLINE const OTUInt16* inputSequence() const noexcept { return fieldAt<OTUInt16>(this, 4); }
    B2D_INLINE const SubstLookupRecord* substArray(size_t glyphCount) const noexcept { return fieldAt<SubstLookupRecord>(this, 4u + glyphCount * 2u - 2u); }

    OTUInt16 glyphCount;                 //!< Total number of glyphs/classes in input glyph sequence (includes the first glyph).
    OTUInt16 substCount;                 //!< Count of entries in `substArray`.
    /*
    OTUInt16 inputSequence[glyphCount - 1];   //!< Array of input glyphs/classes, starts with second glyph.
    SubstLookupRecord substArray[substCount]; //!< SubstLookupRecord array, in design order.
    */
  };
  typedef SubRule SubClassRule;

  typedef OffsetTable SubRuleSet;
  typedef OffsetTable SubClassSet;

  struct ContextSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Count of entries in `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to SubRuleSet tables, ordered by coverage index.
    */
  };

  struct ContextSubst2 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 8); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 classDefOffset;             //!< Offset to glyph ClassDef table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Count of entries in `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to SubClassSet tables, ordered by class (may be NULL).
    */
  };

  struct ContextSubst3 : public SubstHeader {
    B2D_INLINE const OTUInt16* coverageOffsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }
    B2D_INLINE const SubstLookupRecord* substArray(size_t glyphCount) const noexcept { return fieldAt<SubstLookupRecord>(this, 6u + glyphCount * 2u); }

    OTUInt16 glyphCount;                 //!< Number of glyphs in the input glyph sequence.
    OTUInt16 substCount;                 //!< Count of SubstLookupRecord entries.
    /*
    OTUInt16 coverageOffsetArray[glyphCount]; //!< Array of offsets to coverage tables, in glyph sequence order.
    SubstLookupRecord substArray[substCount]; //!< Array of substitution lookups, in design order.
    */
  };

  // --------------------------------------------------------------------------
  // [ChainContextSubst {6}]
  // --------------------------------------------------------------------------

  struct ChainSubRule {
    B2D_INLINE const OTUInt16* backtrackSequence() const noexcept { return fieldAt<OTUInt16>(this, 2); }

    OTUInt16 backtrackGlyphCount;
    /*
    OTUInt16 backtrackSequence[backtrackGlyphCount];
    OTUInt16 inputGlyphCount;
    OTUInt16 inputSequence[inputGlyphCount - 1];
    OTUInt16 lookaheadGlyphCount;
    OTUInt16 lookAheadSequence[lookaheadGlyphCount];
    OTUInt16 substCount;
    SubstLookupRecord substArray[substCount];
    */
  };
  typedef ChainSubRule ChainSubClassRule;

  typedef OffsetTable ChainSubRuleSet;
  typedef OffsetTable ChainSubClassRuleSet;

  struct ChainContextSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 offsetCount;                //!< Count of entries in `offsetArray`.
    /*
    OTUInt16 offsetArray[offsetCount];   //!< Array of offsets to ChainSubRuleSet tables, ordered by coverage index.
    */
  };

  struct ChainContextSubst2 : public SubstHeader {
    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 backtrackClassDefOffset;    //!< Offset to glyph ClassDef table containing backtrack sequence data.
    OTUInt16 inputClassDefOffset;        //!< Offset to glyph ClassDef table containing input sequence data.
    OTUInt16 lookaheadClassDefOffset;    //!< Offset to glyph ClassDef table containing lookahead sequence data.
    OTUInt16 chainSubClassSetCount;      //!< Number of ChainSubClassSet tables.
    /*
    OTUInt16 chainSubClassSetOffsets[chainSubClassSetCount]; //!< Array of offsets to ChainSubClassSet tables.
    */
  };

  struct ChainContextSubst3 : public SubstHeader {
    B2D_INLINE const OTUInt16* backtrackCoverageOffsets() const noexcept { return fieldAt<OTUInt16>(this, 4); }

    OTUInt16 backtrackGlyphCount;
    /*
    OTUInt16 backtrackCoverageOffsets[backtrackGlyphCount];
    OTUInt16 inputGlyphCount;
    OTUInt16 inputCoverageOffsets[inputGlyphCount - 1];
    OTUInt16 lookaheadGlyphCount;
    OTUInt16 lookaheadCoverageOffsets[lookaheadGlyphCount];
    OTUInt16 substCount;
    SubstLookupRecord substArray[substCount];
    */
  };

  // --------------------------------------------------------------------------
  // [ExtensionSubst {7}]
  // --------------------------------------------------------------------------

  struct ExtensionSubst1 : public SubstHeader {
    B2D_INLINE const OTUInt16* offsetArray() const noexcept { return fieldAt<OTUInt16>(this, 6); }

    OTUInt16 lookupType;                 //!< Lookup type of subtable referenced by extensionOffset.
    OTUInt32 extensionOffset;            //!< Offset to the extension subtable.
  };

  // --------------------------------------------------------------------------
  // [ReverseChainSingleSubst {8}]
  // --------------------------------------------------------------------------

  struct ReverseChainSingleSubst1 : public SubstHeader {
    OTUInt16 coverageOffset;             //!< Offset to coverage table, from beginning of substitution subtable.
    OTUInt16 backtrackGlyphCount;        //!< Number of glyphs in the backtracking sequence.
    /*
    OTUInt16 backtrackCoverageOffsets[backtrackGlyphCount];
    OTUInt16 lookaheadGlyphCount;
    OTUInt16 lookaheadCoverageOffsets[lookaheadGlyphCount];
    OTUInt16 substGlyphCount;
    OTUInt16 substGlyphArray[substGlyphCount];
    */
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const HeaderV1_0* v1_0() const noexcept { return &header; }
  B2D_INLINE const HeaderV1_1* v1_1() const noexcept { return fieldAt<HeaderV1_1>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  HeaderV1_0 header;
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::GPosTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Glyph Positioning Table 'GPOS'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/gpos
//!   - https://fontforge.github.io/gposgsub.html
struct GPosTable {
  enum : uint32_t {
    kTag = FontTag::GPOS,
    kMinSize = 0
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::LayoutData]
// ============================================================================

//! \internal
//!
//! Data stored in OTFace that is related to OpenType layout features (GDEF, GSUB, GPSO, etc).
struct LayoutData {
  struct GDef {
    uint8_t classDefinitionFormat;

    uint16_t classDefinitionOffset;
    uint16_t attachListOffset;
    uint16_t ligCaretListOffset;
    uint16_t markAttachClassDefOffset;
    uint16_t markGlyphSetsDefOffset;
    uint32_t itemVarStoreOffset;
  };

  GDef _gdef;
};

// ============================================================================
// [b2d::OT::LayoutImpl]
// ============================================================================

//! \internal
namespace LayoutImpl {
  Error initLayout(OTFaceImpl* faceI, FontData& fontData) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTLAYOUT_P_H
