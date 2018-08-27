// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../text/otface_p.h"
#include "../text/otlayout_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

#if defined(B2D_TRACE_OT) && !defined(B2D_TRACE_OT_LAYOUT)
  #define B2D_TRACE_OT_LAYOUT
#endif

#ifdef B2D_TRACE_OT_LAYOUT
  #define B2D_LOG_OT_LAYOUT(...) DebugUtils::debugFormat(__VA_ARGS__)
#else
  #define B2D_LOG_OT_LAYOUT(...) (void)0
#endif

namespace LayoutImpl {

// ============================================================================
// [b2d::OT::LayoutImpl - LayoutTables]
// ============================================================================

struct LayoutTables {
  B2D_INLINE FontDataRegion* asArray() noexcept { return &gdef; }

  FontDataTable<GDefTable> gdef;
  FontDataTable<GPosTable> gpos;
  FontDataTable<GSubTable> gsub;
};

// ============================================================================
// [b2d::OT::LayoutImpl - ClassDefinitionTable]
// ============================================================================

// TODO: Should we validate also class values like OTS does?

static Error initClassDefinitionTable(OTFaceImpl* faceI, FontDataTable<ClassDefinitionTable> data, uint32_t tableOffset) noexcept {
  B2D_LOG_OT_LAYOUT("OT::ClassDefinitionTable:\n");
  B2D_LOG_OT_LAYOUT("  Size: %zu\n", data.size());

  // Ignore if it doesn't fit.
  if (!data.fitsTable()) {
    B2D_LOG_OT_LAYOUT("  [WARN] Invalid size, ignoring...\n");
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
  }

  uint32_t format = data->format();
  uint32_t glyphCount = faceI->glyphCount();

  B2D_LOG_OT_LAYOUT("  Format: %u\n", format);

  switch (format) {
    case 1: {
      if (data.size() < ClassDefinitionTable::Format1::kMinSize) {
        B2D_LOG_OT_LAYOUT("  [WARN] Invalid size, ignoring...\n");
        faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
      }

      const ClassDefinitionTable::Format1* f = data->format1();
      uint32_t first = f->firstGlyph();
      uint32_t count = f->glyphCount();

      B2D_LOG_OT_LAYOUT("  FirstGlyph: %u\n", first);
      B2D_LOG_OT_LAYOUT("  GlyphCount: %u\n", count);

      // We won't fail, but we won't consider we have class definition either.
      // If class definition is required by other tables then we will fail.
      if (!count) {
        B2D_LOG_OT_LAYOUT("  [WARN] No glyph ids specified, ignoring...\n");
        return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
      }

      if (data.size() < ClassDefinitionTable::Format1::kMinSize + count * 2u) {
        B2D_LOG_OT_LAYOUT("  [ERROR] Table is truncated\n");
        return DebugUtils::errored(kErrorFontInvalidData);
      }

      if (first >= glyphCount || first + count > glyphCount) {
        B2D_LOG_OT_LAYOUT("  [ERROR] FirstGlyph/GlyphCount exceed FontFace::glyphCount(%u)\n", glyphCount);
        return DebugUtils::errored(kErrorFontInvalidData);
      }

      break;
    }

    case 2: {
      if (data.size() < ClassDefinitionTable::Format2::kMinSize) {
        B2D_LOG_OT_LAYOUT("  [WARN] Invalid size, ignoring...\n");
        return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
      }

      const ClassDefinitionTable::Format2* f = data->format2();
      uint32_t count = f->rangeCount();

      B2D_LOG_OT_LAYOUT("  RangeCount: %u\n", count);

      // We won't fail, but we won't consider we have class definition either.
      // If class definition is required by other tables then we will fail.
      if (!count) {
        B2D_LOG_OT_LAYOUT("  [WARN] No range specified, ignoring...\n");
        return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
      }

      if (data.size() < ClassDefinitionTable::Format2::kMinSize + count * sizeof(ClassDefinitionTable::RangeRecord)) {
        B2D_LOG_OT_LAYOUT("  [ERROR] Table is truncated\n");
        return DebugUtils::errored(kErrorFontInvalidData);
      }

      const ClassDefinitionTable::RangeRecord* rangeArray = f->rangeArray();
      uint32_t lastGlyph = rangeArray[0].lastGlyph();

      if (B2D_UNLIKELY(rangeArray[0].firstGlyph() > lastGlyph)) {
        B2D_LOG_OT_LAYOUT("  [ERROR] Table is invalid\n");
        return DebugUtils::errored(kErrorFontInvalidData);
      }

      for (uint32_t i = 1; i < count; i++) {
        const ClassDefinitionTable::RangeRecord& range = rangeArray[i];
        uint32_t firstGlyph = range.firstGlyph();

        if (B2D_UNLIKELY(firstGlyph <= lastGlyph)) {
          B2D_LOG_OT_LAYOUT("  [ERROR] RangeRecord #%u: FirstGlyph(%u) not greater than previous LastGlyph(%u) \n", i, firstGlyph, lastGlyph);
          return DebugUtils::errored(kErrorFontInvalidData);
        }

        lastGlyph = range.lastGlyph();
        if (B2D_UNLIKELY(firstGlyph > lastGlyph)) {
          B2D_LOG_OT_LAYOUT("  [ERROR] RangeRecord #%u: FirstGlyph(%u) greater than LastGlyph(%u)\n", i, firstGlyph, lastGlyph);
          return DebugUtils::errored(kErrorFontInvalidData);
        }
      }

      if (lastGlyph >= glyphCount) {
        B2D_LOG_OT_LAYOUT("  [ERROR] Glyph range (LastGlyph=%u) exceeds FontFace::glyphCount(%u)\n", lastGlyph, glyphCount);
        return DebugUtils::errored(kErrorFontInvalidData);
      }
      break;
    }

    default: {
      B2D_LOG_OT_LAYOUT("  [WARN] ClassDefinitionTable format %u is invalid\n", format);
      return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
    }
  }

  faceI->_layoutData._gdef.classDefinitionFormat = uint8_t(format);
  faceI->_layoutData._gdef.classDefinitionOffset = tableOffset;
  return kErrorOk;
}

// ============================================================================
// [b2d::OT::LayoutImpl - GDEF]
// ============================================================================

static Error initGDef(OTFaceImpl* faceI, LayoutTables& tables) noexcept {
  FontDataTable<GDefTable>& gdef = tables.gdef;

  uint32_t version = gdef->v1_0()->version();
  uint32_t minSize = GDefTable::HeaderV1_0::kMinSize;

  if (version >= 0x00010002u) minSize = GDefTable::HeaderV1_2::kMinSize;
  if (version >= 0x00010003u) minSize = GDefTable::HeaderV1_3::kMinSize;

  B2D_LOG_OT_LAYOUT("OT::Init 'GDEF'\n");
  B2D_LOG_OT_LAYOUT("  Size: %zu\n", gdef.size());
  B2D_LOG_OT_LAYOUT("  Version: %u.%u\n", version >> 16, version & 0xFFFFu);

  if (B2D_UNLIKELY(!gdef.fitsSize(minSize))) {
    B2D_LOG_OT_LAYOUT("  [ERROR] Table too small [MinSize=%u]\n", minSize);
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGDefData);
  }

  uint32_t classDefinitionOffset    = gdef->v1_0()->classDefinitionOffset();
  /*
  uint32_t attachListOffset         = gdef->v1_0()->attachListOffset();
  uint32_t ligCaretListOffset       = gdef->v1_0()->ligCaretListOffset();
  uint32_t markAttachClassDefOffset = gdef->v1_0()->markAttachClassDefOffset();
  uint32_t markGlyphSetsDefOffset   = version >= 0x00010002u ? uint32_t(gdef->v1_2()->markGlyphSetsDefOffset()) : uint32_t(0);
  uint32_t itemVarStoreOffset       = version >= 0x00010003u ? uint32_t(gdef->v1_3()->itemVarStoreOffset()    ) : uint32_t(0);
  */

  if (classDefinitionOffset >= minSize && classDefinitionOffset < gdef.size()) {
    FontDataTable<ClassDefinitionTable> tab(gdef.subRegion(classDefinitionOffset, gdef.size() - classDefinitionOffset));
    B2D_PROPAGATE(initClassDefinitionTable(faceI, tab, classDefinitionOffset));
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::LayoutImpl - GSUB]
// ============================================================================

static Error initGSub(OTFaceImpl* faceI, LayoutTables& tables) noexcept {
  FontDataTable<GSubTable>& gsub = tables.gsub;

  uint32_t version = gsub->v1_0()->version();
  uint32_t minSize = GSubTable::HeaderV1_0::kMinSize;

  if (version >= 0x00010001u) minSize = GSubTable::HeaderV1_1::kMinSize;

  B2D_LOG_OT_LAYOUT("OT::Init 'GSUB'\n");
  B2D_LOG_OT_LAYOUT("  Size: %zu\n", gsub.size());
  B2D_LOG_OT_LAYOUT("  Version: %u.%u\n", version >> 16, version & 0xFFFFu);

  if (B2D_UNLIKELY(!gsub.fitsSize(minSize))) {
    B2D_LOG_OT_LAYOUT("  [ERROR] Invalid size [MinSize=%u]\n", minSize);
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidGSubData);
  }

  uint32_t scriptListOffset  = gsub->v1_0()->scriptListOffset();
  uint32_t featureListOffset = gsub->v1_0()->featureListOffset();
  uint32_t lookupListOffset  = gsub->v1_0()->lookupListOffset();

  // B2D_LOG_OT_LAYOUT("%u %u %u\n", scriptListOffset, featureListOffset, lookupListOffset);
  return kErrorOk;
}

// ============================================================================
// [b2d::OT::LayoutImpl - GPOS]
// ============================================================================

static Error initGPos(OTFaceImpl* faceI, LayoutTables& tables) noexcept {
  return kErrorOk;
}

// ============================================================================
// [b2d::OT::LayoutImpl - Init]
// ============================================================================

Error initLayout(OTFaceImpl* faceI, FontData& fontData) noexcept {
  LayoutTables tables;
  const uint8_t slots[] = { FontData::kSlotGDef, FontData::kSlotGPos, FontData::kSlotGSub };

  if (!fontData.queryTablesBySlots(tables.asArray(), slots, 3))
    return kErrorOk;

  if (tables.gdef.fitsTable())
    B2D_PROPAGATE(initGDef(faceI, tables));

  if (tables.gsub.fitsTable())
    B2D_PROPAGATE(initGSub(faceI, tables));

  if (tables.gpos.fitsTable())
    B2D_PROPAGATE(initGPos(faceI, tables));

  return kErrorOk;
}

} // LayoutImpl namespace

B2D_END_SUB_NAMESPACE
