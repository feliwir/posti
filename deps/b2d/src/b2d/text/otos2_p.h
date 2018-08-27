// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTOS2_P_H
#define _B2D_TEXT_OTOS2_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::OS2Table]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! OS/2 and Windows Metrics Table 'OS/2'.
//!
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/os2
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6OS2.html
struct OS2Table {
  enum : uint32_t {
    kTag = FontTag::OS_2,
    kMinSize = 68
  };

  //! OS/2 selection flags used by `OS2Table::selectionFlags` field.
  enum SelectionFlags : uint32_t {
    kSelectionItalic         = 0x0001,   //!< Font contains italic or oblique characters, otherwise they are upright.
    kSelectionUnderscore     = 0x0002,   //!< Characters are underscored.
    kSelectionNegative       = 0x0004,   //!< Characters have their foreground and background reversed.
    kSelectionOutlined       = 0x0008,   //!< Outline (hollow) characters, otherwise they are solid.
    kSelectionStrikeout      = 0x0010,   //!< Characters are overstruck.
    kSelectionBold           = 0x0020,   //!< Characters are emboldened.
    kSelectionRegular        = 0x0040,   //!< Characters are in the standard weight/style for the font.
    kSelectionUseTypoMetrics = 0x0080,   //!< If set, it is strongly recommended to use typographic metrics.
    kSelectionWWS            = 0x0100,   //!< "name" table strings are consistent with a weight/width/slope family without requiring use of "name" IDs 21 and 22.
    kSelectionOblique        = 0x0200    //!< Font contains oblique characters.
  };

  struct V0A {
    enum : uint32_t { kMinSize = 68 };

    OTUInt16 version;                    //!< OS/2 table version number.
    OTInt16 xAverateWidth;               //!< Average width of all the 26 lowercase Latin letters, including the space character.
    OTUInt16 weightClass;                //!< Weight class.
    OTUInt16 widthClass;                 //!< Width class.
    OTUInt16 embeddingFlags;             //!< Indicates font embedding licensing rights for the font.
    OTInt16 ySubscriptXSize;             //!< Subscript horizontal font size.
    OTInt16 ySubscriptYSize;             //!< Subscript vertical font size.
    OTInt16 ySubscriptXOffset;           //!< Recommended horizontal offset in font design untis for subscripts for this font.
    OTInt16 ySubscriptYOffset;           //!< Recommended vertical offset in font design untis for subscripts for this font.
    OTInt16 ySuperscriptXSize;           //!< Superscript horizontal font size.
    OTInt16 ySuperscriptYSize;           //!< Superscript vertical font size.
    OTInt16 ySuperscriptXOffset;         //!< Recommended horizontal offset in font design untis for superscripts for this font.
    OTInt16 ySuperscriptYOffset;         //!< Recommended vertical offset in font design untis for superscripts for this font.
    OTInt16 yStrikeoutSize;              //!< Width of the strikeout stroke in font design units.
    OTInt16 yStrikeoutPosition;          //!< Position of the top of the strikeout stroke relative to the baseline in font design units.
    OTInt16 familyClass;                 //!< Font family class and subclass.
    OTUInt8 panose[10];                  //!< Panose clasification number.
    OTUInt32 unicodeCoverage[4];         //!< 128-bits describing unicode coverage of the face.
    OTUInt8 vendorId[4];                 //!< The four character identifier for the vendor of the given type face.
    OTUInt16 selectionFlags;             //!< Font selection flags.
    OTUInt16 firstChar;                  //!< Minimum unicode index in this font.
    OTUInt16 lastChar;                   //!< Maximum unicode index in this font.
  };

  struct V0B : public V0A {
    enum : uint32_t { kMinSize = 78 };

    OTInt16 typoAscender;                //!< Typographic ascender for this font.
    OTInt16 typoDescender;               //!< Typographic descender for this font.
    OTInt16 typoLineGap;                 //!< Typographic line-gap for this font.
    OTUInt16 winAscent;                  //!< The ascender metric for Windows.
    OTUInt16 winDescent;                 //!< The descender metric for Windows.
  };

  struct V1 : public V0B {
    enum : uint32_t { kMinSize = 86 };

    OTUInt32 codePageRange[2];
  };

  struct V2 : public V1 {
    enum : uint32_t { kMinSize = 96 };

    OTInt16 xHeight;                     //!< Distance between the baseline and the approximate height of non-ascending lowercase letters.
    OTInt16 capHeight;                   //!< Distance between the baseline and the approximate height of uppercase letters.
    OTUInt16 defaultChar;                //!< Default character for characters not provided by the font.
    OTUInt16 breakChar;                  //!< Unicode encoding of the glyph that Windows uses as the break character.
    OTUInt16 maxContext;                 //!< The maximum length of a target glyph context for any feature in this font
  };

  struct V5 : public V2 {
    enum : uint32_t { kMinSize = 100 };

    OTUInt16 lowerOpticalPointSize;
    OTUInt16 upperOpticalPointSize;
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const V0A* v0a() const noexcept { return fieldAt<V0A>(this, 0); }
  B2D_INLINE const V0B* v0b() const noexcept { return fieldAt<V0B>(this, 0); }
  B2D_INLINE const V1* v1() const noexcept { return fieldAt<V1>(this, 0); }
  B2D_INLINE const V2* v2() const noexcept { return fieldAt<V2>(this, 0); }
  B2D_INLINE const V5* v5() const noexcept { return fieldAt<V5>(this, 0); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  V0A header;
};
#pragma pack(pop)

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTOS2_P_H
