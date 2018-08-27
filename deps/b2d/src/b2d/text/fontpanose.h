// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTPANOSE_H
#define _B2D_TEXT_FONTPANOSE_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontPanose]
// ============================================================================

#pragma pack(push, 1)
//! PANOSE classification.
struct FontPanose {
  enum AnyValue : uint8_t {
    kAnyValueAny                          = 0,
    kAnyValueNoFit                        = 1
  };

  enum FamilyKind : uint8_t {
    kFamilyKindAny                        = 0,
    kFamilyKindNoFit                      = 1,
    kFamilyKindTextDisplay                = 2,
    kFamilyKindScript                     = 3,
    kFamilyKindDecorative                 = 4,
    kFamilyKindSymbol                     = 5
  };

  // --------------------------------------------------------------------------
  // [Text]
  // --------------------------------------------------------------------------

  struct Text {
    enum SerifStyle : uint8_t {
      kSerifStyleAny                      = 0,
      kSerifStyleNoFit                    = 1,
      kSerifStyleCove                     = 2,
      kSerifStyleObtuseCove               = 3,
      kSerifStyleSquareCove               = 4,
      kSerifStyleObtuseSquareCove         = 5,
      kSerifStyleSquare                   = 6,
      kSerifStyleThin                     = 7,
      kSerifStyleBone                     = 8,
      kSerifStyleExaggerated              = 9,
      kSerifStyleTriangle                 = 10,
      kSerifStyleNormalSans               = 11,
      kSerifStyleObtuseSans               = 12,
      kSerifStylePerpSans                 = 13,
      kSerifStyleFlared                   = 14,
      kSerifStyleRounded                  = 15
    };

    enum Weight : uint8_t {
      kWeightAny                          = 0,
      kWeightNoFit                        = 1,
      kWeightVeryLight                    = 2,
      kWeightLight                        = 3,
      kWeightThin                         = 4,
      kWeightBook                         = 5,
      kWeightMedium                       = 6,
      kWeightDemi                         = 7,
      kWeightBold                         = 8,
      kWeightHeavy                        = 9,
      kWeightBlack                        = 10,
      kWeightExtraBlack                   = 11
    };

    enum Proportion : uint8_t {
      kProportionAny                      = 0,
      kProportionNoFit                    = 1,
      kProportionOldStyle                 = 2,
      kProportionModern                   = 3,
      kProportionEvenWidth                = 4,
      kProportionExpanded                 = 5,
      kProportionCondensed                = 6,
      kProportionVeryExpanded             = 7,
      kProportionVeryCondensed            = 8,
      kProportionMonospaced               = 9
    };

    enum Contrast : uint8_t {
      kContrastAny                        = 0,
      kContrastNoFit                      = 1,
      kContrastNone                       = 2,
      kContrastVeryLow                    = 3,
      kContrastLow                        = 4,
      kContrastMediumLow                  = 5,
      kContrastMedium                     = 6,
      kContrastMediumHigh                 = 7,
      kContrastHigh                       = 8,
      kContrastVeryHigh                   = 9
    };

    enum StrokeVariation : uint8_t {
      kStrokeVariationAny                 = 0,
      kStrokeVariationNoFit               = 1,
      kStrokeVariationNoVariation         = 2,
      kStrokeVariationGradualDiagonal     = 3,
      kStrokeVariationGradualTransitional = 4,
      kStrokeVariationGradualVertical     = 5,
      kStrokeVariationGradualHorizontal   = 6,
      kStrokeVariationRapidVertical       = 7,
      kStrokeVariationRapidHorizontal     = 8,
      kStrokeVariationInstantVertical     = 9,
      kStrokeVariationInstantHorizontal   = 10
    };

    enum ArmStyle : uint8_t {
      kArmStyleAny                        = 0,
      kArmStyleNoFit                      = 1,
      kArmStyleStraightArmsHorizontal     = 2,
      kArmStyleStraightArmsWedge          = 3,
      kArmStyleStraightArmsVertical       = 4,
      kArmStyleStraightArmsSingleSerif    = 5,
      kArmStyleStraightArmsDoubleSerif    = 6,
      kArmStyleNonStraightArmsHorizontal  = 7,
      kArmStyleNonStraightArmsWedge       = 8,
      kArmStyleNonStraightArmsVertical    = 9,
      kArmStyleNonStraightArmsSingleSerif = 10,
      kArmStyleNonStraightArmsDoubleSerif = 11
    };

    enum Letterform : uint8_t {
      kLetterformAny                      = 0,
      kLetterformNoFit                    = 1,
      kLetterformNormalContact            = 2,
      kLetterformNormalWeighted           = 3,
      kLetterformNormalBoxed              = 4,
      kLetterformNormalFlattened          = 5,
      kLetterformNormalRounded            = 6,
      kLetterformNormalOffCenter          = 7,
      kLetterformNormalSquare             = 8,
      kLetterformObliqueContact           = 9,
      kLetterformObliqueWeighted          = 10,
      kLetterformObliqueBoxed             = 11,
      kLetterformObliqueFlattened         = 12,
      kLetterformObliqueRounded           = 13,
      kLetterformObliqueOffCenter         = 14,
      kLetterformObliqueSquare            = 15
    };

    enum Midline : uint8_t {
      kMidlineAny                         = 0,
      kMidlineNoFit                       = 1,
      kMidlineStandardTrimmed             = 2,
      kMidlineStandardPointed             = 3,
      kMidlineStandardSerifed             = 4,
      kMidlineHighTrimmed                 = 5,
      kMidlineHighPointed                 = 6,
      kMidlineHighSerifed                 = 7,
      kMidlineConstantTrimmed             = 8,
      kMidlineConstantPointed             = 9,
      kMidlineConstantSerifed             = 10,
      kMidlineLowTrimmed                  = 11,
      kMidlineLowPointed                  = 12,
      kMidlineLowSerifed                  = 13
    };

    enum XHeight : uint8_t {
      kXHeightAny                         = 0,
      kXHeightNoFit                       = 1,
      kXHeightConstantSmall               = 2,
      kXHeightConstantStandard            = 3,
      kXHeightConstantLarge               = 4,
      kXHeightDuckingSmall                = 5,
      kXHeightDuckingStandard             = 6,
      kXHeightDuckingLarge                = 7
    };

    uint8_t familyKind;
    uint8_t serifStyle;
    uint8_t weight;
    uint8_t proportion;
    uint8_t contrast;
    uint8_t strokeVariation;
    uint8_t armStyle;
    uint8_t letterform;
    uint8_t midline;
    uint8_t xHeight;
  };

  // --------------------------------------------------------------------------
  // [Script]
  // --------------------------------------------------------------------------

  struct Script {
    enum ToolKind : uint8_t {
      kToolKindAny                        = 0,
      kToolKindNoFit                      = 1,
      kToolKindFlatNib                    = 2,
      kToolKindPressurePoint              = 3,
      kToolKindEngraved                   = 4,
      kToolKindBall                       = 5,
      kToolKindBrush                      = 6,
      kToolKindRough                      = 7,
      kToolKindFeltPen                    = 8,
      kToolKindWildBrush                  = 9
    };

    enum Weight : uint8_t {
      kWeightAny                          = 0,
      kWeightNoFit                        = 1,
      kWeightVeryLight                    = 2,
      kWeightLight                        = 3,
      kWeightThin                         = 4,
      kWeightBook                         = 5,
      kWeightMedium                       = 6,
      kWeightDemi                         = 7,
      kWeightBold                         = 8,
      kWeightHeavy                        = 9,
      kWeightBlack                        = 10,
      kWeightExtraBlack                   = 11
    };

    enum Spacing : uint8_t {
      kSpacingAny                         = 0,
      kSpacingNoFit                       = 1,
      kSpacingProportionalSpaced          = 2,
      kSpacingMonospaced                  = 3
    };

    enum AspectRatio : uint8_t {
      kAspectRatioAny                     = 0,
      kAspectRatioNoFit                   = 1,
      kAspectRatioVeryCondensed           = 2,
      kAspectRatioCondensed               = 3,
      kAspectRatioNormal                  = 4,
      kAspectRatioExpanded                = 5,
      kAspectRatioVeryExpanded            = 6
    };

    enum Contrast : uint8_t {
      kContrastAny                        = 0,
      kContrastNoFit                      = 1,
      kContrastNone                       = 2,
      kContrastVeryLow                    = 3,
      kContrastLow                        = 4,
      kContrastMediumLow                  = 5,
      kContrastMedium                     = 6,
      kContrastMediumHigh                 = 7,
      kContrastHigh                       = 8,
      kContrastVeryHigh                   = 9
    };

    enum Topology : uint8_t {
      kTopologyAny                        = 0,
      kTopologyNoFit                      = 1,
      kTopologyRomanDisconnected          = 2,
      kTopologyRomanTrailing              = 3,
      kTopologyRomanConnected             = 4,
      kTopologyCursiveDisconnected        = 5,
      kTopologyCursiveTrailing            = 6,
      kTopologyCursiveConnected           = 7,
      kTopologyBlackletterDisconnected    = 8,
      kTopologyBlackletterTrailing        = 9,
      kTopologyBlackletterConnected       = 10
    };

    enum Form : uint8_t {
      kFormAny                            = 0,
      kFormNoFit                          = 1,
      kFormUprightNoWrapping              = 2,
      kFormUprightSomeWrapping            = 3,
      kFormUprightMoreWrapping            = 4,
      kFormUprightExtremeWrapping         = 5,
      kFormObliqueNoWrapping              = 6,
      kFormObliqueSomeWrapping            = 7,
      kFormObliqueMoreWrapping            = 8,
      kFormObliqueExtremeWrapping         = 9,
      kFormExaggeratedNoWrapping          = 10,
      kFormExaggeratedSomeWrapping        = 11,
      kFormExaggeratedMoreWrapping        = 12,
      kFormExaggeratedExtremeWrapping     = 13
    };

    enum Finials : uint8_t {
      kFinialsAny                         = 0,
      kFinialsNoFit                       = 1,
      kFinialsNoneNoLoops                 = 2,
      kFinialsNoneClosedLoops             = 3,
      kFinialsNoneOpenLoops               = 4,
      kFinialsSharpNoLoops                = 5,
      kFinialsSharpClosedLoops            = 6,
      kFinialsSharpOpenLoops              = 7,
      kFinialsTaperedNoLoops              = 8,
      kFinialsTaperedClosedLoops          = 9,
      kFinialsTaperedOpenLoops            = 10,
      kFinialsRoundNoLoops                = 11,
      kFinialsRoundClosedLoops            = 12,
      kFinialsRoundOpenLoops              = 13
    };

    enum XAscent : uint8_t {
      kXAscentAny                         = 0,
      kXAscentNoFit                       = 1,
      kXAscentVeryLow                     = 2,
      kXAscentLow                         = 3,
      kXAscentMedium                      = 4,
      kXAscentHigh                        = 5,
      kXAscentVeryHigh                    = 6
    };

    uint8_t familyKind;
    uint8_t toolKind;
    uint8_t weight;
    uint8_t spacing;
    uint8_t aspectRatio;
    uint8_t contrast;
    uint8_t topology;
    uint8_t form;
    uint8_t finials;
    uint8_t xAscent;
  };

  // --------------------------------------------------------------------------
  // [Decorative]
  // --------------------------------------------------------------------------

  struct Decorative {
    enum Class : uint8_t {
      kClassAny                           = 0,
      kClassNoFit                         = 1,
      kClassDerivative                    = 2,
      kClassNonStandardTopology           = 3,
      kClassNonStandardElements           = 4,
      kClassNonStandardAspect             = 5,
      kClassInitials                      = 6,
      kClassCartoon                       = 7,
      kClassPictureStems                  = 8,
      kClassOrnamented                    = 9,
      kClassTextAndBackground             = 10,
      kClassCollage                       = 11,
      kClassMontage                       = 12
    };

    enum Weight : uint8_t {
      kWeightAny                          = 0,
      kWeightNoFit                        = 1,
      kWeightVeryLight                    = 2,
      kWeightLight                        = 3,
      kWeightThin                         = 4,
      kWeightBook                         = 5,
      kWeightMedium                       = 6,
      kWeightDemi                         = 7,
      kWeightBold                         = 8,
      kWeightHeavy                        = 9,
      kWeightBlack                        = 10,
      kWeightExtraBlack                   = 11
    };

    enum Aspect : uint8_t {
      kAspectAny                          = 0,
      kAspectNoFit                        = 1,
      kAspectSuperCondensed               = 2,
      kAspectVeryCondensed                = 3,
      kAspectCondensed                    = 4,
      kAspectNormal                       = 5,
      kAspectExtended                     = 6,
      kAspectVeryExtended                 = 7,
      kAspectSuperExtended                = 8,
      kAspectMonospaced                   = 9
    };

    enum Contrast : uint8_t {
      kContrastAny                        = 0,
      kContrastNoFit                      = 1,
      kContrastNone                       = 2,
      kContrastVeryLow                    = 3,
      kContrastLow                        = 4,
      kContrastMediumLow                  = 5,
      kContrastMedium                     = 6,
      kContrastMediumHigh                 = 7,
      kContrastHigh                       = 8,
      kContrastVeryHigh                   = 9,
      kContrastHorizontalLow              = 10,
      kContrastHorizontalMedium           = 11,
      kContrastHorizontalHigh             = 12,
      kContrastBroken                     = 13
    };

    enum SerifVariant : uint8_t {
      kSerifVariantAny                    = 0,
      kSerifVariantNoFit                  = 1,
      kSerifVariantCove                   = 2,
      kSerifVariantObtuseCove             = 3,
      kSerifVariantSquareCove             = 4,
      kSerifVariantObtuseSquareCove       = 5,
      kSerifVariantSquare                 = 6,
      kSerifVariantThin                   = 7,
      kSerifVariantOval                   = 8,
      kSerifVariantExaggerated            = 9,
      kSerifVariantTriangle               = 10,
      kSerifVariantNormalSans             = 11,
      kSerifVariantObtuseSans             = 12,
      kSerifVariantPerpendicularSans      = 13,
      kSerifVariantFlared                 = 14,
      kSerifVariantRounded                = 15,
      kSerifVariantScript                 = 16
    };

    enum Treatment : uint8_t {
      kTreatmentAny                       = 0,
      kTreatmentNoFit                     = 1,
      kTreatmentNoneStandardSolidFill     = 2,
      kTreatmentWhiteNoFill               = 3,
      kTreatmentPatternedFill             = 4,
      kTreatmentComplexFill               = 5,
      kTreatmentShapedFill                = 6,
      kTreatmentDrawnDistressed           = 7
    };

    enum Lining : uint8_t {
      kLiningAny                          = 0,
      kLiningNoFit                        = 1,
      kLiningNone                         = 2,
      kLiningInline                       = 3,
      kLiningOutline                      = 4,
      kLiningEngraved                     = 5,
      kLiningShadow                       = 6,
      kLiningRelief                       = 7,
      kLiningBackdrop                     = 8
    };

    enum Topology : uint8_t {
      kTopologyAny                        = 0,
      kTopologyNoFit                      = 1,
      kTopologyStandard                   = 2,
      kTopologySquare                     = 3,
      kTopologyMultipleSegment            = 4,
      kTopologyDecoWacoMidlines           = 5,
      kTopologyUnevenWeighting            = 6,
      kTopologyDiverseArms                = 7,
      kTopologyDiverseForms               = 8,
      kTopologyLombardicForms             = 9,
      kTopologyUpperCaseInLowerCase       = 10,
      kTopologyImpliedTopology            = 11,
      kTopologyHorseshoeEandA             = 12,
      kTopologyCursive                    = 13,
      kTopologyBlackletter                = 14,
      kTopologySwashVariance              = 15
    };

    enum CharacterRange : uint8_t {
      kCharacterRangeAny                  = 0,
      kCharacterRangeNoFit                = 1,
      kCharacterRangeExtendedCollection   = 2,
      kCharacterRangeLitterals            = 3,
      kCharacterRangeNoLowerCase          = 4,
      kCharacterRangeSmallCaps            = 5
    };

    uint8_t familyKind;
    uint8_t decorativeClass;
    uint8_t weight;
    uint8_t aspect;
    uint8_t contrast;
    uint8_t serifVariant;
    uint8_t treatment;
    uint8_t lining;
    uint8_t topology;
    uint8_t characterRange;
  };

  // --------------------------------------------------------------------------
  // [Symbol]
  // --------------------------------------------------------------------------

  struct Symbol {
    enum Kind : uint8_t {
      kKindAny                            = 0,
      kKindNoFit                          = 1,
      kKindMontages                       = 2,
      kKindPictures                       = 3,
      kKindShapes                         = 4,
      kKindScientific                     = 5,
      kKindMusic                          = 6,
      kKindExpert                         = 7,
      kKindPatterns                       = 8,
      kKindBoarders                       = 9,
      kKindIcons                          = 10,
      kKindLogos                          = 11,
      kKindIndustrySpecific               = 12
    };

    enum Weight : uint8_t {
      kWeightAny                          = 0,
      kWeightNoFit                        = 1
    };

    enum Spacing : uint8_t {
      kSpacingAny                         = 0,
      kSpacingNoFit                       = 1,
      kSpacingProportionalSpaced          = 2,
      kSpacingMonospaced                  = 3
    };

    enum AspectRatioAndContrast : uint8_t {
      kAspectRatioAndContrastAny          = 0,
      kAspectRatioAndContrastNoFit        = 1
    };

    enum AspectRatio : uint8_t {
      kAspectRatioAny                     = 0,
      kAspectRatioNoFit                   = 1,
      kAspectRatioNoWidth                 = 2,
      kAspectRatioExceptionallyWide       = 3,
      kAspectRatioSuperWide               = 4,
      kAspectRatioVeryWide                = 5,
      kAspectRatioWide                    = 6,
      kAspectRatioNormal                  = 7,
      kAspectRatioNarrow                  = 8,
      kAspectRatioVeryNarrow              = 9
    };

    uint8_t familyKind;
    uint8_t symbolKind;
    uint8_t weight;
    uint8_t spacing;
    uint8_t aspectRatioAndContrast;
    uint8_t aspectRatio94;
    uint8_t aspectRatio119;
    uint8_t aspectRatio157;
    uint8_t aspectRatio163;
    uint8_t aspectRatio211;
  };

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    std::memset(this, 0, sizeof(*this));
  }

  B2D_INLINE bool empty() const noexcept {
    uint32_t x = 0;
    for (size_t i = 0 ; i < sizeof(*this); i++)
      x |= data[i];
    return x == 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint8_t data[10];
    uint8_t familyKind;

    Text text;
    Script script;
    Decorative decorative;
    Symbol symbol;
  };
};
#pragma pack(pop)

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTPANOSE_H
