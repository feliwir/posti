// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/support.h"
#include "../text/otface_p.h"
#include "../text/otmetrics_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

namespace MetricsImpl {

// ============================================================================
// [b2d::OT::MetricsImpl - GetGlyphAdvances]
// ============================================================================

static Error B2D_CDECL getGlyphAdvances(const GlyphEngine* self, const GlyphItem* glyphItems, Point* glyphOffsets, size_t count) noexcept {
  if (B2D_UNLIKELY(self->empty()))
    return DebugUtils::errored(kErrorFontNotInitialized);

  const OT::OTFaceImpl* faceI = self->fontFace().impl<OT::OTFaceImpl>();
  const OT::XMtxTable* mtxTable = self->fontData().loadedTablePtrBySlot<OT::XMtxTable>(FontData::kSlotHMtx);

  // Sanity check.
  uint32_t longMetricCount = faceI->_metricsData._longMetricCount[TextOrientation::kHorizontal];
  uint32_t longMetricMax = longMetricCount - 1u;

  if (B2D_UNLIKELY(!longMetricCount))
    return DebugUtils::errored(kErrorFontInvalidData);

  for (size_t i = 0; i < count; i++) {
    uint32_t glyphId = glyphItems[i].glyphId();
    uint32_t metricIndex = Math::min(glyphId, longMetricMax);

    int32_t advance = mtxTable->lmArray()[metricIndex].advance.value();
    glyphOffsets[i].reset(double(advance), 0.0);
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::MetricsImpl - Init]
// ============================================================================

Error initMetrics(OTFaceImpl* faceI, FontData& fontData) noexcept {
  FontDataTable<HHeaTable> hhea;
  FontDataTable<VHeaTable> vhea;

  typedef TextOrientation TO;
  FontDesignMetrics& dm = faceI->_designMetrics;

  if (fontData.queryTableByTag(&hhea, FontTag::hhea)) {
    if (!hhea.fitsTable())
      return DebugUtils::errored(kErrorFontInvalidData);

    if (!(faceI->_faceFlags & FontFace::kFlagTypographicMetrics)) {
      int16_t ascent  = hhea->ascender();
      int16_t descent = hhea->descender();
      int16_t lineGap = hhea->lineGap();

      descent = OTUtils::safeAbs(descent);

      dm._ascent[TO::kHorizontal] = ascent;
      dm._descent[TO::kHorizontal] = descent;
      dm._lineGap = lineGap;
    }

    dm._minLSB[TO::kHorizontal] = hhea->minLeadingBearing();
    dm._minTSB[TO::kHorizontal] = hhea->minTrailingBearing();
    dm._maxAdvance[TO::kHorizontal] = hhea->maxAdvance();

    FontDataTable<HMtxTable> hmtx;
    if (fontData.queryTableBySlot(&hmtx, FontData::kSlotHMtx)) {
      uint32_t longMetricCount = Math::min<uint32_t>(hhea->longMetricCount(), faceI->_glyphCount);
      uint32_t longMetricDataSize = longMetricCount * sizeof(XMtxTable::LongMetric);

      if (longMetricDataSize > hmtx.size())
        return DebugUtils::errored(kErrorFontInvalidData);

      size_t lsbCount = Math::min<size_t>((hmtx.size() - longMetricDataSize) / 2u, longMetricCount - faceI->_glyphCount);
      faceI->_metricsData._longMetricCount[TO::kHorizontal] = uint16_t(longMetricCount);
      faceI->_metricsData._lsbArraySize[TO::kHorizontal] = uint16_t(lsbCount);
    }

    faceI->_funcs.getGlyphAdvances = getGlyphAdvances;
  }

  if (fontData.queryTableByTag(&vhea, FontTag::vhea)) {
    if (!vhea.fitsTable())
      return DebugUtils::errored(kErrorFontInvalidData);

    dm._ascent[TO::kVertical] = vhea->ascender();
    dm._descent[TO::kVertical] = vhea->descender();
    dm._minLSB[TO::kVertical] = vhea->minLeadingBearing();
    dm._minTSB[TO::kVertical] = vhea->minTrailingBearing();
    dm._maxAdvance[TO::kVertical] = vhea->maxAdvance();

    FontDataTable<VMtxTable> vmtx;
    if (fontData.queryTableBySlot(&vmtx, FontData::kSlotVMtx)) {
      uint32_t longMetricCount = Math::min<uint32_t>(vhea->longMetricCount(), faceI->_glyphCount);
      uint32_t longMetricDataSize = longMetricCount * sizeof(XMtxTable::LongMetric);

      if (longMetricDataSize > vmtx.size())
        return DebugUtils::errored(kErrorFontInvalidData);

      size_t lsbCount = Math::min<size_t>((vmtx.size() - longMetricDataSize) / 2u, longMetricCount - faceI->_glyphCount);
      faceI->_metricsData._longMetricCount[TO::kVertical] = uint16_t(longMetricCount);
      faceI->_metricsData._lsbArraySize[TO::kVertical] = uint16_t(lsbCount);
    }
  }

  return kErrorOk;
}

} // MetricsImpl namespace

B2D_END_SUB_NAMESPACE
