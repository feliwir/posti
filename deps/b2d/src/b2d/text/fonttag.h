// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTTAG_H
#define _B2D_TEXT_FONTTAG_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontTag]
// ============================================================================

namespace FontTag {
  //! Tag is a 32-bit number in BE format that can be directly compared to RAW
  //! 32-bit value read from font data (without converting it to machine endian).
  template<typename T0, typename T1, typename T2, typename T3>
  constexpr uint32_t make(T0&& c0, T1&& c1, T2&& c2, T3&& c3) noexcept {
    return Support::bytepack32_4x8(uint8_t(c0), uint8_t(c1), uint8_t(c2), uint8_t(c3));
  }

  enum : uint32_t {
    none  = 0u,

    // Font Header
    // -----------

    sfnt  = make('s', 'f', 'n', 't'),    //!< TrueType/OpenType font header.

    // Tables - Core
    // -------------

    OS_2  = make('O', 'S', '/', '2'),    //!< OS/2 and Windows specific metrics.
    cmap  = make('c', 'm', 'a', 'p'),    //!< Character to glyph mapping.
    head  = make('h', 'e', 'a', 'd'),    //!< Font header.
    hhea  = make('h', 'h', 'e', 'a'),    //!< Horizontal metrics header.
    hmtx  = make('h', 'm', 't', 'x'),    //!< Horizontal metrics data.
    maxp  = make('m', 'a', 'x', 'p'),    //!< Maximum profile.
    name  = make('n', 'a', 'm', 'e'),    //!< Naming table.
    post  = make('p', 'o', 's', 't'),    //!< PostScript information.
    vhea  = make('v', 'h', 'e', 'a'),    //!< Vertical metrics header.
    vmtx  = make('v', 'm', 't', 'x'),    //!< Vertical metrics data.

    // Tables - TrueType Outlines
    // --------------------------

    cvt   = make('c', 'v', 't', ' '),    //!< Control value table.
    fpgm  = make('f', 'p', 'g', 'm'),    //!< Font program.
    glyf  = make('g', 'l', 'y', 'f'),    //!< Glyph data.
    loca  = make('l', 'o', 'c', 'a'),    //!< Index to location.
    prep  = make('p', 'r', 'e', 'p'),    //!< CVT program.
    gasp  = make('g', 'a', 's', 'p'),    //!< Grid-fitting / Scan-conversion.

    // Tables - CFF Outlines
    // ---------------------

    CFF   = make('C', 'F', 'F', ' '),    //!< Compact font format 1.0.
    CFF2  = make('C', 'F', 'F', '2'),    //!< Compact font format 2.0.
    VORG  = make('V', 'O', 'R', 'G'),    //!< Vertical origin.

    // Tables - Bitmap Glyphs
    // ----------------------

    CBDT  = make('C', 'B', 'D', 'T'),    //!< Color bitmap data.
    CBLC  = make('C', 'B', 'L', 'C'),    //!< Color bitmap location data.
    EBDT  = make('E', 'B', 'D', 'T'),    //!< Embedded bitmap data.
    EBLC  = make('E', 'B', 'L', 'C'),    //!< Embedded bitmap location data.
    EBSC  = make('E', 'B', 'S', 'C'),    //!< Embedded bitmap scaling data.
    sbix  = make('s', 'b', 'i', 'x'),    //!< Standard bitmap graphics.

    // Tables - Advanced Typography
    // ----------------------------

    BASE  = make('B', 'A', 'S', 'E'),    //!< Baseline data.
    GDEF  = make('G', 'D', 'E', 'F'),    //!< Glyph definition data.
    GPOS  = make('G', 'P', 'O', 'S'),    //!< Glyph positioning data.
    GSUB  = make('G', 'S', 'U', 'B'),    //!< Glyph substitution data.
    JSTF  = make('J', 'S', 'T', 'F'),    //!< Justification data.
    MATH  = make('M', 'A', 'T', 'H'),    //!< Math layout data.

    // Tables - Font Variations
    // ------------------------

    avar  = make('a', 'v', 'a', 'r'),    //!< Axis variations.
    cvar  = make('c', 'v', 'a', 'r'),    //!< CVT variations [TrueType].
    fvar  = make('f', 'v', 'a', 'r'),    //!< Font variations.
    gvar  = make('g', 'v', 'a', 'r'),    //!< Glyph variations [TrueType].
    HVAR  = make('H', 'V', 'A', 'R'),    //!< Horizontal metrics variations.
    MVAR  = make('M', 'V', 'A', 'R'),    //!< Metrics variations.
    STAT  = make('S', 'T', 'A', 'T'),    //!< Style attributes.
    VVAR  = make('V', 'V', 'A', 'R'),    //!< Vertical metrics variations.

    // Tables - Color Fonts / SVG
    // --------------------------

    COLR  = make('C', 'O', 'L', 'R'),    //!< Color table.
    CPAL  = make('C', 'P', 'A', 'L'),    //!< Color palette table.
    SVG   = make('S', 'V', 'G', ' '),    //!< SVG table.

    // Tables - Other
    // --------------

    DSIG  = make('D', 'S', 'I', 'G'),    //!< Digital signature.
    hdmx  = make('h', 'd', 'm', 'x'),    //!< Horizontal device metrics.
    LTSH  = make('L', 'T', 'S', 'H'),    //!< Linear threshold data.
    MERG  = make('M', 'E', 'R', 'G'),    //!< Merge.
    meta  = make('m', 'e', 't', 'a'),    //!< Metadata.
    PCLT  = make('P', 'C', 'L', 'T'),    //!< PCL 5 data.
    VDMX  = make('V', 'D', 'M', 'X'),    //!< Vertical device metrics.

    // Baseline Tags
    // -------------

    hang  = make('h', 'a', 'n', 'g'),    //!< The hanging baseline.
    icfb  = make('i', 'c', 'f', 'b'),    //!< Ideographic character face BOTTOM or LEFT edge baseline.
    icft  = make('i', 'c', 'f', 't'),    //!< Ideographic character face TOP or RIGHT edge baseline.
    ideo  = make('i', 'd', 'e', 'o'),    //!< Ideographic em-box BOTTOM or LEFT edge baseline.
    idtp  = make('i', 'd', 't', 'p'),    //!< Ideographic em-box TOP or RIGHT edge baseline.
    math  = make('m', 'a', 't', 'h'),    //!< The baseline about which mathematical characters are centered.
    romn  = make('r', 'o', 'm', 'n'),    //!< The baseline used by simple alphabetic scripts.

    // Feature Tags
    // ------------
    //
    //   - https://docs.microsoft.com/en-us/typography/opentype/spec/featurelist

    aalt  = make('s', 'u', 'b', 's'),    //!< Access all alternates.
    abvf  = make('a', 'b', 'v', 'f'),    //!< Above-base forms.
    abvm  = make('a', 'b', 'v', 'm'),    //!< Above-base mark positioning.
    abvs  = make('a', 'b', 'v', 's'),    //!< Above-base substitutions.
    afrc  = make('a', 'f', 'r', 'c'),    //!< Alternative fractions.
    akhn  = make('a', 'k', 'h', 'n'),    //!< Akhands.
    blwf  = make('b', 'l', 'w', 'f'),    //!< Below-base forms.
    blwm  = make('b', 'l', 'w', 'm'),    //!< Below-base mark positioning.
    blws  = make('b', 'l', 'w', 's'),    //!< Below-base substitutions.
    calt  = make('c', 'a', 'l', 't'),    //!< Contextual alternates.
    case_ = make('c', 'a', 's', 'e'),    //!< Case-sensitive forms.
    ccmp  = make('c', 'c', 'm', 'p'),    //!< Glyph composition / decomposition.
    cfar  = make('c', 'f', 'a', 'r'),    //!< Conjunct form after RO.
    cjct  = make('c', 'j', 'c', 't'),    //!< Conjunct forms.
    clig  = make('c', 'l', 'i', 'g'),    //!< Contextual ligatures.
    cpct  = make('c', 'p', 'c', 't'),    //!< Centered CJK punctuation.
    cpsp  = make('c', 'p', 's', 'p'),    //!< Capital spacing.
    cswh  = make('c', 's', 'w', 'h'),    //!< Contextual swash.
    curs  = make('c', 'u', 'r', 's'),    //!< Cursive positioning.
    cv__  = make('c', 'v',  0 ,  0 ),    //!< Character variants 01-99.
    c2pc  = make('c', '2', 'p', 'c'),    //!< Petite capitals from capitals.
    c2sc  = make('c', '2', 's', 'c'),    //!< Small capitals from capitals.
    dist  = make('d', 'i', 's', 't'),    //!< Distances.
    dlig  = make('d', 'l', 'i', 'g'),    //!< Discretionary ligatures.
    dnom  = make('d', 'n', 'o', 'm'),    //!< Denominators.
    dtls  = make('d', 't', 'l', 's'),    //!< Dotless forms.
    expt  = make('e', 'x', 'p', 't'),    //!< Expert forms.
    falt  = make('f', 'a', 'l', 't'),    //!< Final glyph on line alternates.
    fin2  = make('f', 'i', 'n', '2'),    //!< Terminal forms #2.
    fin3  = make('f', 'i', 'n', '3'),    //!< Terminal forms #3.
    fina  = make('f', 'i', 'n', 'a'),    //!< Terminal forms.
    flac  = make('f', 'l', 'a', 'c'),    //!< Flattened accent forms.
    frac  = make('f', 'r', 'a', 'c'),    //!< Fractions.
    fwid  = make('f', 'w', 'i', 'd'),    //!< Full widths.
    half  = make('h', 'a', 'l', 'f'),    //!< Half forms.
    haln  = make('h', 'a', 'l', 'n'),    //!< Halant forms.
    halt  = make('h', 'a', 'l', 't'),    //!< Alternate half widths.
    hist  = make('h', 'i', 's', 't'),    //!< Historical forms.
    hkna  = make('h', 'k', 'n', 'a'),    //!< Horizontal Kana alternates.
    hlig  = make('h', 'l', 'i', 'g'),    //!< Historical ligatures.
    hngl  = make('h', 'n', 'g', 'l'),    //!< Hangul.
    hojo  = make('h', 'o', 'j', 'o'),    //!< Hojo kanji forms (JIS X 0212-1990 kanji forms).
    hwid  = make('h', 'w', 'i', 'd'),    //!< Half widths.
    init  = make('i', 'n', 'i', 't'),    //!< Initial forms.
    isol  = make('i', 's', 'o', 'l'),    //!< Isolated forms.
    ital  = make('i', 't', 'a', 'l'),    //!< Italics.
    jalt  = make('j', 'a', 'l', 't'),    //!< Justification alternates.
    jp78  = make('j', 'p', '7', '8'),    //!< JIS78 forms.
    jp83  = make('j', 'p', '8', '3'),    //!< JIS83 forms.
    jp90  = make('j', 'p', '9', '0'),    //!< JIS90 forms.
    jp04  = make('j', 'p', '0', '4'),    //!< JIS2004 forms.
    kern  = make('k', 'e', 'r', 'n'),    //!< Kerning.
    lfbd  = make('l', 'f', 'b', 'd'),    //!< Left bounds.
    liga  = make('l', 'i', 'g', 'a'),    //!< Standard ligatures.
    ljmo  = make('l', 'j', 'm', 'o'),    //!< Leading Jamo forms.
    lnum  = make('l', 'n', 'u', 'm'),    //!< Lining figures.
    locl  = make('l', 'o', 'c', 'l'),    //!< Localized forms.
    ltra  = make('l', 't', 'r', 'a'),    //!< Left-to-right alternates.
    ltrm  = make('l', 't', 'r', 'm'),    //!< Left-to-right mirrored forms.
    mark  = make('m', 'a', 'r', 'k'),    //!< Mark positioning.
    med2  = make('m', 'e', 'd', '2'),    //!< Medial forms #2.
    medi  = make('m', 'e', 'd', 'i'),    //!< Medial forms.
    mgrk  = make('m', 'g', 'r', 'k'),    //!< Mathematical Greek.
    mkmk  = make('m', 'k', 'm', 'k'),    //!< Mark to mark sositioning.
    mset  = make('m', 's', 'e', 't'),    //!< Mark positioning via substitution.
    nalt  = make('n', 'a', 'l', 't'),    //!< Alternate annotation forms.
    nlck  = make('n', 'l', 'c', 'k'),    //!< NLC kanji forms.
    nukt  = make('n', 'u', 'k', 't'),    //!< Nukta forms.
    numr  = make('n', 'u', 'm', 'r'),    //!< Numerators.
    onum  = make('o', 'n', 'u', 'm'),    //!< Oldstyle figures.
    opbd  = make('o', 'p', 'b', 'd'),    //!< Optical bounds.
    ordn  = make('o', 'r', 'd', 'n'),    //!< Ordinals.
    ornm  = make('o', 'r', 'n', 'm'),    //!< Ornaments.
    palt  = make('p', 'a', 'l', 't'),    //!< Proportional alternate widths.
    pcap  = make('p', 'c', 'a', 'p'),    //!< Petite capitals.
    pkna  = make('p', 'k', 'n', 'a'),    //!< Proportional kana.
    pnum  = make('p', 'n', 'u', 'm'),    //!< Proportional figures.
    pref  = make('p', 'r', 'e', 'f'),    //!< Pre-base forms.
    pres  = make('p', 'r', 'e', 's'),    //!< Pre-base substitutions.
    pstf  = make('p', 's', 't', 'f'),    //!< Post-base forms.
    psts  = make('p', 's', 't', 's'),    //!< Post-base substitutions.
    pwid  = make('p', 'w', 'i', 'd'),    //!< Proportional widths.
    qwid  = make('q', 'w', 'i', 'd'),    //!< Quarter widths.
    rand  = make('r', 'a', 'n', 'd'),    //!< Randomize.
    rclt  = make('r', 'c', 'l', 't'),    //!< Required contextual alternates.
    rkrf  = make('r', 'k', 'r', 'f'),    //!< Rakar forms.
    rlig  = make('r', 'l', 'i', 'g'),    //!< Required ligatures.
    rphf  = make('r', 'p', 'h', 'f'),    //!< Reph forms.
    rtbd  = make('r', 't', 'b', 'd'),    //!< Right bounds.
    rtla  = make('r', 't', 'l', 'a'),    //!< Right-to-left alternates.
    rtlm  = make('r', 't', 'l', 'm'),    //!< Right-to-left mirrored forms.
    ruby  = make('r', 'u', 'b', 'y'),    //!< Ruby notation forms.
    rvrn  = make('r', 'v', 'r', 'n'),    //!< Required variation alternates.
    salt  = make('s', 'a', 'l', 't'),    //!< Stylistic alternates.
    sinf  = make('s', 'i', 'n', 'f'),    //!< Scientific inferiors.
    size  = make('s', 'i', 'z', 'e'),    //!< Optical size.
    smcp  = make('s', 'm', 'c', 'p'),    //!< Small capitals.
    smpl  = make('s', 'm', 'p', 'l'),    //!< Simplified Forms.
    ss__  = make('s', 's',  0 ,  0 ),    //!< Stylistic set 01-20.
    ssty  = make('s', 's', 't', 'y'),    //!< Math script style alternates.
    stch  = make('s', 't', 'c', 'h'),    //!< Stretching glyph decomposition.
    subs  = make('s', 'u', 'b', 's'),    //!< Subscript.
    sups  = make('s', 'u', 'p', 's'),    //!< Superscript.
    swsh  = make('s', 'w', 's', 'h'),    //!< Swash.
    titl  = make('t', 'i', 't', 'l'),    //!< Titling.
    tjmo  = make('t', 'j', 'm', 'o'),    //!< Trailing jamo forms.
    tnam  = make('t', 'n', 'a', 'm'),    //!< Traditional name forms.
    tnum  = make('t', 'n', 'u', 'm'),    //!< Tabular figures.
    trad  = make('t', 'r', 'a', 'd'),    //!< Traditional forms.
    twid  = make('t', 'w', 'i', 'd'),    //!< Third widths.
    unic  = make('u', 'n', 'i', 'c'),    //!< Unicase.
    valt  = make('v', 'a', 'l', 't'),    //!< Alternate vertical metrics.
    vatu  = make('v', 'a', 't', 'u'),    //!< Vattu variants.
    vert  = make('v', 'e', 'r', 't'),    //!< Vertical writing.
    vhal  = make('v', 'h', 'a', 'l'),    //!< Alternate vertical half metrics.
    vjmo  = make('v', 'j', 'm', 'o'),    //!< Vowel jamo forms.
    vkna  = make('v', 'k', 'n', 'a'),    //!< Vertical nana alternates.
    vkrn  = make('v', 'k', 'r', 'n'),    //!< Vertical kerning.
    vpal  = make('v', 'p', 'a', 'l'),    //!< Proportional alternate vertical metrics.
    vrt2  = make('v', 'r', 't', '2'),    //!< Vertical alternates and rotation.
    vrtr  = make('v', 'r', 't', 'r'),    //!< Vertical alternates for rotation.
    zero  = make('z', 'e', 'r', 'o')     //!< Slashed zero.
  };
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTTAG_H
