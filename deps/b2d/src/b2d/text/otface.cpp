// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../text/otcff_p.h"
#include "../text/otcmap_p.h"
#include "../text/otface_p.h"
#include "../text/otglyf_p.h"
#include "../text/othead_p.h"
#include "../text/otkern_p.h"
#include "../text/otlayout_p.h"
#include "../text/otmaxp_p.h"
#include "../text/otmetrics_p.h"
#include "../text/otname_p.h"
#include "../text/otsfnt_p.h"
#include "../text/otos2_p.h"
#include "../text/otpost_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

// ============================================================================
// [b2d::OT::Face - Construction / Destruction]
// ============================================================================

OTFaceImpl::OTFaceImpl(const FontLoader& loader, uint32_t faceIndex) noexcept
  : FontFaceImpl(loader, faceIndex),
    _locaOffsetSize { 0 },
    _cMapData {},
    _metricsData {},
    _layoutData {},
    _cffInfo {} {}
OTFaceImpl::~OTFaceImpl() noexcept {}

// ============================================================================
// [b2d::OT::Face - Init]
// ============================================================================

Error OTFaceImpl::initFace(FontData& fontData) noexcept {
  FontDesignMetrics& dm = _designMetrics;

  // --------------------------------------------------------------------------
  // [MAXP]
  // --------------------------------------------------------------------------

  FontDataTable<MaxPTable> maxp;
  if (fontData.queryTableByTag(&maxp, FontTag::maxp) && maxp.fitsTable()) {
    // We don't know yet if the font is TrueType or OpenType, so only use v0.5 header.
    uint32_t glyphCount = maxp->v0_5()->glyphCount();
    if (glyphCount == 0)
      return DebugUtils::errored(kErrorFontInvalidData);

    _glyphCount = uint16_t(glyphCount);
  }
  else {
    return DebugUtils::errored(kErrorFontInvalidData);
  }

  // --------------------------------------------------------------------------
  // [OS/2]
  // --------------------------------------------------------------------------

  FontDataTable<OS2Table> os2;
  if (fontData.queryTableByTag(&os2, FontTag::OS_2)) {
    if (!os2.fitsTable())
      return DebugUtils::errored(kErrorFontInvalidData);

    // Read weight and stretch (width in OS/2 table).
    uint32_t weight = os2->v0a()->weightClass();
    uint32_t stretch = os2->v0a()->widthClass();

    // Fix design weight from 1..9 to 100..900 (reported by ~8% of fonts).
    if (weight >= 1 && weight <= 9) weight *= 100;

    // Use defaults if not provided.
    if (!weight) weight = Font::kWeightNormal;
    if (!stretch) stretch = Font::kStretchNormal;

    _weight = uint16_t(Math::bound<uint32_t>(weight, 1, 999));
    _stretch = uint8_t(Math::bound<uint32_t>(stretch, 1, 9));

    // Read PANOSE classification.
    std::memcpy(&_panose, os2->v0a()->panose, sizeof(FontPanose));
    if (!_panose.empty())
      _faceFlags |= FontFace::kFlagPanose;

    // Read unicode coverage.
    _faceFlags |= FontFace::kFlagUnicodeCoverage;
    _unicodeCoverage._data[0] = os2->v0a()->unicodeCoverage[0].value();
    _unicodeCoverage._data[1] = os2->v0a()->unicodeCoverage[1].value();
    _unicodeCoverage._data[2] = os2->v0a()->unicodeCoverage[2].value();
    _unicodeCoverage._data[3] = os2->v0a()->unicodeCoverage[3].value();

    dm._strikethroughPosition = os2->v0a()->yStrikeoutPosition();
    dm._strikethroughThickness = os2->v0a()->yStrikeoutSize();

    uint32_t version = os2->v0a()->version();
    if (os2.fitsTable<OS2Table::V0B>()) {
      // Read selection flags.
      uint32_t selectionFlags = os2->v0a()->selectionFlags();

      if (selectionFlags & OS2Table::kSelectionItalic)
        _style = Font::kStyleItalic;
      else if (selectionFlags & OS2Table::kSelectionOblique)
        _style = Font::kStyleOblique;

      if ((selectionFlags & OS2Table::kSelectionUseTypoMetrics) != 0)
        _faceFlags |= FontFace::kFlagTypographicMetrics;

      int16_t ascent  = os2->v0b()->typoAscender();
      int16_t descent = os2->v0b()->typoDescender();
      int16_t lineGap = os2->v0b()->typoLineGap();

      descent = OTUtils::safeAbs(descent);
      dm._ascent[TextOrientation::kHorizontal] = ascent;
      dm._descent[TextOrientation::kHorizontal] = descent;
      dm._lineGap = lineGap;

      if (os2.fitsTable<OS2Table::V2>() && version >= 2) {
        dm._xHeight = os2->v2()->xHeight();
        dm._capHeight = os2->v2()->capHeight();
      }
    }
  }

  // --------------------------------------------------------------------------
  // [POST]
  // --------------------------------------------------------------------------

  FontDataTable<PostTable> post;
  if (fontData.queryTableByTag(&post, FontTag::post)) {
    if (post.fitsTable()) {
      int16_t underlinePosition = post->underlinePosition();
      int16_t underlineThickness = post->underlineThickness();

      dm._underlinePosition = underlinePosition;
      dm._underlineThickness = underlineThickness;
    }
  }

  // --------------------------------------------------------------------------
  // [HEAD]
  // --------------------------------------------------------------------------

  FontDataTable<HeadTable> head;
  if (fontData.queryTableByTag(&head, FontTag::head) && head.fitsTable()) {
    constexpr uint16_t kMinUnitsPerEm = 16;
    constexpr uint16_t kMaxUnitsPerEm = 16384;

    uint32_t headFlags = head->flags();

    if (headFlags & HeadTable::kFlagBaselineYEquals0)
      _faceFlags |= FontFace::kFlagBaselineYEquals0;

    if (headFlags & HeadTable::kFlagLSBPointXEquals0)
      _faceFlags |= FontFace::kFlagLSBPointXEquals0;

    if (headFlags & HeadTable::kFlagLastResortFont)
      _faceFlags |= FontFace::kFlagLastResortFont;

    uint16_t unitsPerEm = head->unitsPerEm();
    if (B2D_UNLIKELY(unitsPerEm < kMinUnitsPerEm || unitsPerEm > kMaxUnitsPerEm))
      return DebugUtils::errored(kErrorFontInvalidData);
    _unitsPerEm = unitsPerEm;
  }
  else {
    return DebugUtils::errored(kErrorFontInvalidData);
  }

  // --------------------------------------------------------------------------
  // []
  // --------------------------------------------------------------------------

  B2D_PROPAGATE(NameImpl::initName(this, fontData));
  B2D_PROPAGATE(CMapImpl::initCMap(this, fontData));

  // --------------------------------------------------------------------------
  // [CFF | GLYF]
  // --------------------------------------------------------------------------

  FontDataTable<CFFTable> cff;
  if (fontData.queryTableBySlot(&cff, FontData::kSlotCFF)) {
    _implType = FontFace::kImplTypeOT_CFFv1;
    B2D_PROPAGATE(CFFImpl::initCFF(this, cff, 1));
  }
  else {
    _implType = FontFace::kImplTypeTT;
    B2D_PROPAGATE(GlyfImpl::initGlyf(this, fontData));
  }

  B2D_PROPAGATE(MetricsImpl::initMetrics(this, fontData));
  B2D_PROPAGATE(LayoutImpl::initLayout(this, fontData));
  B2D_PROPAGATE(KernImpl::initKern(this, fontData));

  return kErrorOk;
}

B2D_END_SUB_NAMESPACE
