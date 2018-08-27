// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTUNICODECOVERAGE_H
#define _B2D_TEXT_FONTUNICODECOVERAGE_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontUnicodeCoverage]
// ============================================================================

//! Unicode coverage stored in TrueType and OpenType fonts (OS/2 table).
struct FontUnicodeCoverage {
  enum Bits : uint32_t {
    kBasicLatin                          = 0,   //!< [000000-00007F] Basic Latin.
    kLatin1Supplement                    = 1,   //!< [000080-0000FF] Latin-1 Supplement.
    kLatinExtendedA                      = 2,   //!< [000100-00017F] Latin Extended-A.
    kLatinExtendedB                      = 3,   //!< [000180-00024F] Latin Extended-B.
    kIPAExtensions                       = 4,   //!< [000250-0002AF] IPA Extensions.
                                                //!< [001D00-001D7F] Phonetic Extensions.
                                                //!< [001D80-001DBF] Phonetic Extensions Supplement.
    kSpacingModifierLetters              = 5,   //!< [0002B0-0002FF] Spacing Modifier Letters.
                                                //!< [00A700-00A71F] Modifier Tone Letters.
    kCombiningDiacriticalMarks           = 6,   //!< [000300-00036F] Combining Diacritical Marks.
                                                //!< [001DC0-001DFF] Combining Diacritical Marks Supplement.
    kGreekAndCoptic                      = 7,   //!< [000370-0003FF] Greek and Coptic.
    kCoptic                              = 8,   //!< [002C80-002CFF] Coptic.
    kCyrillic                            = 9,   //!< [000400-0004FF] Cyrillic.
                                                //!< [000500-00052F] Cyrillic Supplement.
                                                //!< [002DE0-002DFF] Cyrillic Extended-A.
                                                //!< [00A640-00A69F] Cyrillic Extended-B.
    kArmenian                            = 10,  //!< [000530-00058F] Armenian.
    kHebrew                              = 11,  //!< [000590-0005FF] Hebrew.
    kVai                                 = 12,  //!< [00A500-00A63F] Vai.
    kArabic                              = 13,  //!< [000600-0006FF] Arabic.
                                                //!< [000750-00077F] Arabic Supplement.
    kNKo                                 = 14,  //!< [0007C0-0007FF] NKo.
    kDevanagari                          = 15,  //!< [000900-00097F] Devanagari.
    kBengali                             = 16,  //!< [000980-0009FF] Bengali.
    kGurmukhi                            = 17,  //!< [000A00-000A7F] Gurmukhi.
    kGujarati                            = 18,  //!< [000A80-000AFF] Gujarati.
    kOriya                               = 19,  //!< [000B00-000B7F] Oriya.
    kTamil                               = 20,  //!< [000B80-000BFF] Tamil.
    kTelugu                              = 21,  //!< [000C00-000C7F] Telugu.
    kKannada                             = 22,  //!< [000C80-000CFF] Kannada.
    kMalayalam                           = 23,  //!< [000D00-000D7F] Malayalam.
    kThai                                = 24,  //!< [000E00-000E7F] Thai.
    kLao                                 = 25,  //!< [000E80-000EFF] Lao.
    kGeorgian                            = 26,  //!< [0010A0-0010FF] Georgian.
                                                //!< [002D00-002D2F] Georgian Supplement.
    kBalinese                            = 27,  //!< [001B00-001B7F] Balinese.
    kHangulJamo                          = 28,  //!< [001100-0011FF] Hangul Jamo.
    kLatinExtendedAdditional             = 29,  //!< [001E00-001EFF] Latin Extended Additional.
                                                //!< [002C60-002C7F] Latin Extended-C.
                                                //!< [00A720-00A7FF] Latin Extended-D.
    kGreekExtended                       = 30,  //!< [001F00-001FFF] Greek Extended.
    kGeneralPunctuation                  = 31,  //!< [002000-00206F] General Punctuation.
                                                //!< [002E00-002E7F] Supplemental Punctuation.
    kSuperscriptsAndSubscripts           = 32,  //!< [002070-00209F] Superscripts And Subscripts.
    kCurrencySymbols                     = 33,  //!< [0020A0-0020CF] Currency Symbols.
    kCombiningDiacriticalMarksForSymbols = 34,  //!< [0020D0-0020FF] Combining Diacritical Marks For Symbols.
    kLetterlikeSymbols                   = 35,  //!< [002100-00214F] Letterlike Symbols.
    kNumberForms                         = 36,  //!< [002150-00218F] Number Forms.
    kArrows                              = 37,  //!< [002190-0021FF] Arrows.
                                                //!< [0027F0-0027FF] Supplemental Arrows-A.
                                                //!< [002900-00297F] Supplemental Arrows-B.
                                                //!< [002B00-002BFF] Miscellaneous Symbols and Arrows.
    kMathematicalOperators               = 38,  //!< [002200-0022FF] Mathematical Operators.
                                                //!< [002A00-002AFF] Supplemental Mathematical Operators.
                                                //!< [0027C0-0027EF] Miscellaneous Mathematical Symbols-A.
                                                //!< [002980-0029FF] Miscellaneous Mathematical Symbols-B.
    kMiscellaneousTechnical              = 39,  //!< [002300-0023FF] Miscellaneous Technical.
    kControlPictures                     = 40,  //!< [002400-00243F] Control Pictures.
    kOpticalCharacterRecognition         = 41,  //!< [002440-00245F] Optical Character Recognition.
    kEnclosedAlphanumerics               = 42,  //!< [002460-0024FF] Enclosed Alphanumerics.
    kBoxDrawing                          = 43,  //!< [002500-00257F] Box Drawing.
    kBlockElements                       = 44,  //!< [002580-00259F] Block Elements.
    kGeometricShapes                     = 45,  //!< [0025A0-0025FF] Geometric Shapes.
    kMiscellaneousSymbols                = 46,  //!< [002600-0026FF] Miscellaneous Symbols.
    kDingbats                            = 47,  //!< [002700-0027BF] Dingbats.
    kCJKSymbolsAndPunctuation            = 48,  //!< [003000-00303F] CJK Symbols And Punctuation.
    kHiragana                            = 49,  //!< [003040-00309F] Hiragana.
    kKatakana                            = 50,  //!< [0030A0-0030FF] Katakana.
                                                //!< [0031F0-0031FF] Katakana Phonetic Extensions.
    kBopomofo                            = 51,  //!< [003100-00312F] Bopomofo.
                                                //!< [0031A0-0031BF] Bopomofo Extended.
    kHangulCompatibilityJamo             = 52,  //!< [003130-00318F] Hangul Compatibility Jamo.
    kPhagsPa                             = 53,  //!< [00A840-00A87F] Phags-pa.
    kEnclosedCJKLettersAndMonths         = 54,  //!< [003200-0032FF] Enclosed CJK Letters And Months.
    kCJKCompatibility                    = 55,  //!< [003300-0033FF] CJK Compatibility.
    kHangulSyllables                     = 56,  //!< [00AC00-00D7AF] Hangul Syllables.
    kNonPlane                            = 57,  //!< [00D800-00DFFF] Non-Plane 0 *.
    kPhoenician                          = 58,  //!< [010900-01091F] Phoenician.
    kCJKUnifiedIdeographs                = 59,  //!< [004E00-009FFF] CJK Unified Ideographs.
                                                //!< [002E80-002EFF] CJK Radicals Supplement.
                                                //!< [002F00-002FDF] Kangxi Radicals.
                                                //!< [002FF0-002FFF] Ideographic Description Characters.
                                                //!< [003400-004DBF] CJK Unified Ideographs Extension A.
                                                //!< [020000-02A6DF] CJK Unified Ideographs Extension B.
                                                //!< [003190-00319F] Kanbun.
    kPrivateUsePlane0                    = 60,  //!< [00E000-00F8FF] Private Use (Plane 0).
    kCJKStrokes                          = 61,  //!< [0031C0-0031EF] CJK Strokes.
                                                //!< [00F900-00FAFF] CJK Compatibility Ideographs.
                                                //!< [02F800-02FA1F] CJK Compatibility Ideographs Supplement.
    kAlphabeticPresentationForms         = 62,  //!< [00FB00-00FB4F] Alphabetic Presentation Forms.
    kArabicPresentationFormsA            = 63,  //!< [00FB50-00FDFF] Arabic Presentation Forms-A.
    kCombiningHalfMarks                  = 64,  //!< [00FE20-00FE2F] Combining Half Marks.
    kVerticalForms                       = 65,  //!< [00FE10-00FE1F] Vertical Forms.
                                                //!< [00FE30-00FE4F] CJK Compatibility Forms.
    kSmallFormVariants                   = 66,  //!< [00FE50-00FE6F] Small Form Variants.
    kArabicPresentationFormsB            = 67,  //!< [00FE70-00FEFF] Arabic Presentation Forms-B.
    kHalfwidthAndFullwidthForms          = 68,  //!< [00FF00-00FFEF] Halfwidth And Fullwidth Forms.
    kSpecials                            = 69,  //!< [00FFF0-00FFFF] Specials.
    kTibetan                             = 70,  //!< [000F00-000FFF] Tibetan.
    kSyriac                              = 71,  //!< [000700-00074F] Syriac.
    kThaana                              = 72,  //!< [000780-0007BF] Thaana.
    kSinhala                             = 73,  //!< [000D80-000DFF] Sinhala.
    kMyanmar                             = 74,  //!< [001000-00109F] Myanmar.
    kEthiopic                            = 75,  //!< [001200-00137F] Ethiopic.
                                                //!< [001380-00139F] Ethiopic Supplement.
                                                //!< [002D80-002DDF] Ethiopic Extended.
    kCherokee                            = 76,  //!< [0013A0-0013FF] Cherokee.
    kUnifiedCanadianAboriginalSyllabics  = 77,  //!< [001400-00167F] Unified Canadian Aboriginal Syllabics.
    kOgham                               = 78,  //!< [001680-00169F] Ogham.
    kRunic                               = 79,  //!< [0016A0-0016FF] Runic.
    kKhmer                               = 80,  //!< [001780-0017FF] Khmer.
                                                //!< [0019E0-0019FF] Khmer Symbols.
    kMongolian                           = 81,  //!< [001800-0018AF] Mongolian.
    kBraillePatterns                     = 82,  //!< [002800-0028FF] Braille Patterns.
    kYiSyllablesAndRadicals              = 83,  //!< [00A000-00A48F] Yi Syllables.
                                                //!< [00A490-00A4CF] Yi Radicals.
    kTagalogHanunooBuhidTagbanwa         = 84,  //!< [001700-00171F] Tagalog.
                                                //!< [001720-00173F] Hanunoo.
                                                //!< [001740-00175F] Buhid.
                                                //!< [001760-00177F] Tagbanwa.
    kOldItalic                           = 85,  //!< [010300-01032F] Old Italic.
    kGothic                              = 86,  //!< [010330-01034F] Gothic.
    kDeseret                             = 87,  //!< [010400-01044F] Deseret.
    kMusicalSymbols                      = 88,  //!< [01D000-01D0FF] Byzantine Musical Symbols.
                                                //!< [01D100-01D1FF] Musical Symbols.
                                                //!< [01D200-01D24F] Ancient Greek Musical Notation.
    kMathematicalAlphanumericSymbols     = 89,  //!< [01D400-01D7FF] Mathematical Alphanumeric Symbols.
    kPrivateUsePlane15_16                = 90,  //!< [0F0000-0FFFFD] Private Use (Plane 15).
                                                //!< [100000-10FFFD] Private Use (Plane 16).
    kVariationSelectors                  = 91,  //!< [00FE00-00FE0F] Variation Selectors.
                                                //!< [0E0100-0E01EF] Variation Selectors Supplement.
    kTags                                = 92,  //!< [0E0000-0E007F] Tags.
    kLimbu                               = 93,  //!< [001900-00194F] Limbu.
    kTaiLe                               = 94,  //!< [001950-00197F] Tai Le.
    kNewTaiLue                           = 95,  //!< [001980-0019DF] New Tai Lue.
    kBuginese                            = 96,  //!< [001A00-001A1F] Buginese.
    kGlagolitic                          = 97,  //!< [002C00-002C5F] Glagolitic.
    kTifinagh                            = 98,  //!< [002D30-002D7F] Tifinagh.
    kYijingHexagramSymbols               = 99,  //!< [004DC0-004DFF] Yijing Hexagram Symbols.
    kSylotiNagri                         = 100, //!< [00A800-00A82F] Syloti Nagri.
    kLinearBSyllabaryAndIdeograms        = 101, //!< [010000-01007F] Linear B Syllabary.
                                                //!< [010080-0100FF] Linear B Ideograms.
                                                //!< [010100-01013F] Aegean Numbers.
    kAncientGreekNumbers                 = 102, //!< [010140-01018F] Ancient Greek Numbers.
    kUgaritic                            = 103, //!< [010380-01039F] Ugaritic.
    kOldPersian                          = 104, //!< [0103A0-0103DF] Old Persian.
    kShavian                             = 105, //!< [010450-01047F] Shavian.
    kOsmanya                             = 106, //!< [010480-0104AF] Osmanya.
    kCypriotSyllabary                    = 107, //!< [010800-01083F] Cypriot Syllabary.
    kKharoshthi                          = 108, //!< [010A00-010A5F] Kharoshthi.
    kTaiXuanJingSymbols                  = 109, //!< [01D300-01D35F] Tai Xuan Jing Symbols.
    kCuneiform                           = 110, //!< [012000-0123FF] Cuneiform.
                                                //!< [012400-01247F] Cuneiform Numbers and Punctuation.
    kCountingRodNumerals                 = 111, //!< [01D360-01D37F] Counting Rod Numerals.
    kSundanese                           = 112, //!< [001B80-001BBF] Sundanese.
    kLepcha                              = 113, //!< [001C00-001C4F] Lepcha.
    kOlChiki                             = 114, //!< [001C50-001C7F] Ol Chiki.
    kSaurashtra                          = 115, //!< [00A880-00A8DF] Saurashtra.
    kKayahLi                             = 116, //!< [00A900-00A92F] Kayah Li.
    kRejang                              = 117, //!< [00A930-00A95F] Rejang.
    kCham                                = 118, //!< [00AA00-00AA5F] Cham.
    kAncientSymbols                      = 119, //!< [010190-0101CF] Ancient Symbols.
    kPhaistosDisc                        = 120, //!< [0101D0-0101FF] Phaistos Disc.
    kCarianLycianLydian                  = 121, //!< [0102A0-0102DF] Carian.
                                                //!< [010280-01029F] Lycian.
                                                //!< [010920-01093F] Lydian.
    kDominoAndMahjongTiles               = 122, //!< [01F030-01F09F] Domino Tiles.
                                                //!< [01F000-01F02F] Mahjong Tiles.
    kInternalUsage123                    = 123, //!< Reserved for process-internal usage.
    kInternalUsage124                    = 124, //!< Reserved for process-internal usage.
    kInternalUsage125                    = 125, //!< Reserved for process-internal usage.
    kInternalUsage126                    = 126, //!< Reserved for process-internal usage.
    kInternalUsage127                    = 127  //!< Reserved for process-internal usage.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _data[0] = 0;
    _data[1] = 0;
    _data[2] = 0;
    _data[3] = 0;
  }

  constexpr bool hasBit(uint32_t index) const noexcept {
    return ((_data[index / 32] >> (index % 32)) & 0x1) != 0;
  }

  B2D_INLINE void setBit(uint32_t index) noexcept {
    _data[index / 32] |= uint32_t(1) << (index % 32);
  }

  B2D_INLINE void clearBit(uint32_t index) noexcept {
    _data[index / 32] &= ~(uint32_t(1) << (index % 32));
  }

  constexpr bool eq(const FontUnicodeCoverage& other) const noexcept {
    return (_data[0] == other._data[0]) &
           (_data[1] == other._data[1]) &
           (_data[2] == other._data[2]) &
           (_data[3] == other._data[3]) ;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  constexpr bool operator==(const FontUnicodeCoverage& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const FontUnicodeCoverage& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _data[4];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTUNICODECOVERAGE_H
