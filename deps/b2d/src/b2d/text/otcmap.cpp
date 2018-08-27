// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../text/fontface.h"
#include "../text/glyphengine.h"
#include "../text/otcmap_p.h"
#include "../text/otface_p.h"
#include "../text/otplatform_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

// ============================================================================
// [b2d::OT::CMapImpl - Utilities]
// ============================================================================

static B2D_INLINE const OTUInt16* CMapImpl_Format4_findSegment(
  uint32_t uc,
  const OTUInt16* lastCharArray,
  size_t numSeg,
  size_t numSearchableSeg,
  uint32_t& ucFirst, uint32_t& ucLast) noexcept {

  for (size_t i = numSearchableSeg; i != 0; i >>= 1) {
    const OTUInt16* endCountPtr = Support::advancePtr(lastCharArray, (i & ~size_t(1)));

    ucLast = OTUtils::readU16(endCountPtr);
    if (ucLast < uc) {
      lastCharArray = endCountPtr + 1;
      i--;
      continue;
    }

    ucFirst = OTUtils::readU16(Support::advancePtr(endCountPtr, 2u + numSeg * 2u));
    if (ucFirst <= uc)
      return endCountPtr;
  }

  return nullptr;
}

static B2D_INLINE bool CMapImpl_Format12_13_findGroup(
  uint32_t uc,
  const CMapTable::Group* start,
  size_t count,
  uint32_t& ucFirst, uint32_t& ucLast, uint32_t& firstGlyphId) noexcept {

  for (size_t i = count; i != 0; i >>= 1) {
    const CMapTable::Group* group = start + (i >> 1);

    ucLast = group->last();
    if (ucLast < uc) {
      start = group + 1;
      i--;
      continue;
    }

    ucFirst = group->first();
    if (ucFirst > uc)
      continue;

    firstGlyphId = group->glyphId();
    return true;
  }

  return false;
}

// ============================================================================
// [b2d::OT::CMapImpl - Format0]
// ============================================================================

static Error B2D_CDECL CMapImpl_Format0_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  FontDataRange dataRange = self->_fontFace->impl<OT::OTFaceImpl>()->_cMapData.dataRange();
  const CMapTable::Format0* table = Support::advancePtr(self->_fontData.loadedTablePtrBySlot<CMapTable::Format0>(FontData::kSlotCMap), dataRange.offset());

  GlyphItem* glyphPtr = glyphItems;
  GlyphItem* glyphEnd = glyphItems + count;

  size_t undefinedCount = 0;
  state->_undefinedFirst = Globals::kInvalidIndex;

  const OTUInt8* glyphIdArray = table->glyphIdArray;

  while (glyphPtr != glyphEnd) {
    uint32_t codePoint = glyphPtr->codePoint();
    GlyphId glyphId = codePoint < 256 ? GlyphId(glyphIdArray[codePoint].value()) : GlyphId(0);

    glyphPtr->setGlyphIdAndFlags(glyphId, 0);

    if (B2D_UNLIKELY(glyphId == 0)) {
      if (!undefinedCount)
        state->_undefinedFirst = (size_t)(glyphPtr - glyphItems);
      undefinedCount++;
    }
  }

  state->_glyphCount = (size_t)(glyphPtr - glyphItems);
  state->_undefinedCount = undefinedCount;

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::CMapImpl - Format4]
// ============================================================================

static Error B2D_CDECL CMapImpl_Format4_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  FontDataRange dataRange = self->_fontFace->impl<OT::OTFaceImpl>()->_cMapData.dataRange();
  const CMapTable::Format4* table = Support::advancePtr(self->_fontData.loadedTablePtrBySlot<CMapTable::Format4>(FontData::kSlotCMap), dataRange.offset());

  GlyphItem* glyphPtr = glyphItems;
  GlyphItem* glyphEnd = glyphItems + count;

  size_t undefinedCount = 0;
  state->_undefinedFirst = Globals::kInvalidIndex;

  size_t numSeg = size_t(table->numSegX2()) >> 1;
  const OTUInt16* lastCharArray = table->lastCharArray();

  // We want to read 2 bytes from the glyphIdsArray, that's why we decrement by one here.
  const uint8_t* dataEnd = reinterpret_cast<const uint8_t*>(table) + dataRange.length() - 1;

  size_t numSearchableSeg = self->_fontFace->impl<OTFaceImpl>()->_cMapData.entryCount();
  size_t kIdDeltaArrayOffset = 2u + numSeg * 4u;
  size_t kIdOffsetArrayOffset = 2u + numSeg * 6u;

  uint32_t uc;
  uint32_t ucFirst;
  uint32_t ucLast;

  while (glyphPtr != glyphEnd) {
    uc = glyphPtr->codePoint();

NewMatch:
    const OTUInt16* match = CMapImpl_Format4_findSegment(uc, lastCharArray, numSeg, numSearchableSeg, ucFirst, ucLast);
    if (match) {
      // `match` points to `endChar[]`, based on CMAP-Format4:
      //   - match[0             ] == lastCharArray [Segment]
      //   - match[2 + numSeg * 2] == firstCharArray[Segment]
      //   - match[2 + numSeg * 4] == idDeltaArray  [Segment]
      //   - match[2 + numSeg * 6] == idOffsetArray [Segment]
      uint32_t offset = Support::advancePtr(match, kIdOffsetArrayOffset)->value();
      for (;;) {
        // If the `offset` is not zero then we have to get the GlyphId from `glyphArray`.
        if (offset != 0) {
          size_t rawRemain = (size_t)(dataEnd - reinterpret_cast<const uint8_t*>(match));
          size_t rawOffset = kIdOffsetArrayOffset + (uc - ucFirst) * 2u + offset;

          // This shouldn't happen if the table was properly validated.
          if (B2D_UNLIKELY(rawOffset >= rawRemain))
            goto UndefinedGlyph;

          uc = Support::advancePtr(match, rawOffset)->value();
        }

        uc += Support::advancePtr(match, kIdDeltaArrayOffset)->value();
        uc &= 0xFFFFu;

        if (B2D_UNLIKELY(uc == 0))
          goto UndefinedGlyph;

        glyphPtr->setGlyphIdAndFlags(GlyphId(uc), 0);
        if (++glyphPtr == glyphEnd)
          goto Done;

        uc = glyphPtr->codePoint();
        if (uc < ucFirst || uc > ucLast)
          goto NewMatch;
      }
    }
    else {
UndefinedGlyph:
      if (!undefinedCount)
        state->_undefinedFirst = (size_t)(glyphPtr - glyphItems);

      glyphPtr->setGlyphIdAndFlags(GlyphId(0), 0);
      glyphPtr++;
      undefinedCount++;
    }
  }

Done:
  state->_glyphCount = (size_t)(glyphPtr - glyphItems);
  state->_undefinedCount = undefinedCount;

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::CMapImpl - Format6]
// ============================================================================

static Error B2D_CDECL CMapImpl_Format6_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  FontDataRange dataRange = self->_fontFace->impl<OT::OTFaceImpl>()->_cMapData.dataRange();
  const CMapTable::Format6* table = Support::advancePtr(self->_fontData.loadedTablePtrBySlot<CMapTable::Format6>(FontData::kSlotCMap), dataRange.offset());

  GlyphItem* glyphPtr = glyphItems;
  GlyphItem* glyphEnd = glyphItems + count;

  size_t undefinedCount = 0;
  state->_undefinedFirst = Globals::kInvalidIndex;

  uint32_t ucFirst = table->first();
  uint32_t ucCount = table->count();
  const OTUInt16* glyphIdArray = table->glyphIdArray();

  while (glyphPtr != glyphEnd) {
    uint32_t uc = glyphPtr->codePoint() - ucFirst;
    GlyphId glyphId = uc < ucCount ? GlyphId(glyphIdArray[uc].value()) : GlyphId(0);

    glyphPtr->setGlyphIdAndFlags(glyphId, 0);
    glyphPtr++;

    if (B2D_UNLIKELY(glyphId == 0)) {
      if (!undefinedCount)
        state->_undefinedFirst = (size_t)(glyphPtr - glyphItems) - 1;
      undefinedCount++;
    }
  }

  state->_glyphCount = (size_t)(glyphPtr - glyphItems);
  state->_undefinedCount = undefinedCount;

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::CMapImpl - Format10]
// ============================================================================

static Error B2D_CDECL CMapImpl_Format10_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  FontDataRange dataRange = self->_fontFace->impl<OT::OTFaceImpl>()->_cMapData.dataRange();
  const CMapTable::Format10* table = Support::advancePtr(self->_fontData.loadedTablePtrBySlot<CMapTable::Format10>(FontData::kSlotCMap), dataRange.offset());

  GlyphItem* glyphPtr = glyphItems;
  GlyphItem* glyphEnd = glyphItems + count;

  size_t undefinedCount = 0;
  state->_undefinedFirst = Globals::kInvalidIndex;

  uint32_t ucFirst = table->first();
  uint32_t ucCount = table->count();
  const OTUInt16* glyphIdArray = table->glyphIdArray();

  while (glyphPtr != glyphEnd) {
    uint32_t uc = glyphPtr->codePoint() - ucFirst;
    GlyphId glyphId = uc < ucCount ? GlyphId(glyphIdArray[uc].value()) : GlyphId(0);

    glyphPtr->setGlyphIdAndFlags(glyphId, 0);
    glyphPtr++;

    if (B2D_UNLIKELY(glyphId == 0)) {
      if (!undefinedCount)
        state->_undefinedFirst = (size_t)(glyphPtr - glyphItems) - 1;
      undefinedCount++;
    }
  }

  state->_glyphCount = (size_t)(glyphPtr - glyphItems);
  state->_undefinedCount = undefinedCount;

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::CMapImpl - Format12 / Format13]
// ============================================================================

template<uint32_t FormatId>
static Error CMapImpl_Format12_13_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  FontDataRange dataRange = self->_fontFace->impl<OT::OTFaceImpl>()->_cMapData.dataRange();
  const CMapTable::Format12_13* table = Support::advancePtr(self->_fontData.loadedTablePtrBySlot<CMapTable::Format12_13>(FontData::kSlotCMap), dataRange.offset());

  GlyphItem* glyphPtr = glyphItems;
  GlyphItem* glyphEnd = glyphItems + count;

  size_t undefinedCount = 0;
  state->_undefinedFirst = Globals::kInvalidIndex;

  const CMapTable::Group* groupArray = table->groupArray();
  size_t groupCount = self->_fontFace->impl<OTFaceImpl>()->_cMapData.entryCount();

  while (glyphPtr != glyphEnd) {
    uint32_t uc;
    uint32_t ucFirst;
    uint32_t ucLast;
    uint32_t startGlyphId;

    uc = glyphPtr->codePoint();

NewMatch:
    if (CMapImpl_Format12_13_findGroup(uc, groupArray, groupCount, ucFirst, ucLast, startGlyphId)) {
      for (;;) {
        GlyphId glyphId = FormatId == 12 ? GlyphId((uc - ucFirst + startGlyphId) & 0xFFFFu)
                                         : GlyphId((startGlyphId) & 0xFFFFu);
        if (!glyphId)
          goto UndefinedGlyph;

        glyphPtr->setGlyphIdAndFlags(glyphId, 0);
        if (++glyphPtr == glyphEnd)
          goto Done;

        uc = glyphPtr->codePoint();
        if (uc < ucFirst || uc > ucLast)
          goto NewMatch;
      }
    }
    else {
UndefinedGlyph:
      if (!undefinedCount)
        state->_undefinedFirst = (size_t)(glyphPtr - glyphItems);

      glyphPtr->setGlyphIdAndFlags(GlyphId(0), 0);
      glyphPtr++;
      undefinedCount++;
    }
  }

Done:
  state->_glyphCount = (size_t)(glyphPtr - glyphItems);
  state->_undefinedCount = undefinedCount;

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::CMapImpl - Validate]
// ============================================================================

Error CMapImpl::validateEncoding(FontDataRegion dataRegion, uint32_t dataOffset, CMapData& cMapInfo) noexcept {
  if (dataRegion.size() < 4)
    return DebugUtils::errored(kErrorFontInvalidCMap);

  uint32_t format = dataRegion.data<OTUInt16>()->value();
  switch (format) {
    // ------------------------------------------------------------------------
    // [Format 0 - Byte Encoding Table]
    // ------------------------------------------------------------------------

    case 0: {
      if (!dataRegion.fitsTable<CMapTable::Format0>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Format0* table = dataRegion.data<CMapTable::Format0>();
      uint32_t tableSize = table->length();

      if (tableSize < CMapTable::Format0::kMinSize || tableSize > dataRegion.size())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      cMapInfo.reset(format, CMapData::kFlagInitialized, 256, FontDataRange { dataOffset, uint32_t(CMapTable::Format0::kMinSize) });
      return kErrorOk;
    }

    // ------------------------------------------------------------------------
    // [Format 2 - High-Byte Mapping Through Table]
    // ------------------------------------------------------------------------

    case 2: {
      return DebugUtils::errored(kErrorFontUnsupportedCMap);
    }

    // ------------------------------------------------------------------------
    // [Format 4 - Segment Mapping to Delta Values]
    // ------------------------------------------------------------------------

    case 4: {
      if (!dataRegion.fitsTable<CMapTable::Format4>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Format4* table = dataRegion.data<CMapTable::Format4>();
      uint32_t tableSize = table->length();

      if (tableSize < CMapTable::Format4::kMinSize || tableSize > dataRegion.size())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      uint32_t numSegX2 = table->numSegX2();
      if (!numSegX2 || (numSegX2 & 1) != 0)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      uint32_t numSeg = numSegX2 / 2;
      if (tableSize < 16 + numSeg * 8)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const OTUInt16* lastCharArray = table->lastCharArray();
      const OTUInt16* firstCharArray = table->firstCharArray(numSeg);
      const OTUInt16* idOffsetArray = table->idOffsetArray(numSeg);

      uint32_t previousEnd = 0;
      uint32_t numSegAfterCheck = numSeg;

      for (uint32_t i = 0; i < numSeg; i++) {
        uint32_t last = lastCharArray[i].value();
        uint32_t first = firstCharArray[i].value();
        uint32_t idOffset = idOffsetArray[i].value();

        if (first == 0xFFFF && last == 0xFFFF) {
          // We prefer number of segments without the ending mark(s). This
          // handles also the case of data with multiple ending marks.
          numSegAfterCheck = Math::min(numSegAfterCheck, i);
        }
        else {
          if (first < previousEnd || first > last)
            return DebugUtils::errored(kErrorFontInvalidCMap);

          if (i != 0 && first == previousEnd)
            return DebugUtils::errored(kErrorFontInvalidCMap);

          if (idOffset != 0) {
            // Offset to 16-bit data must be even.
            if (idOffset & 1)
              return DebugUtils::errored(kErrorFontInvalidCMap);

            // This just validates whether the table doesn't want us to jump
            // somewhere outside, it doesn't validate whether GlyphIds are not
            // outside the limit.
            uint32_t indexInTable = 16 + numSeg * 6u + idOffset + (last - first) * 2u;
            if (indexInTable >= tableSize)
              return DebugUtils::errored(kErrorFontInvalidCMap);
          }
        }

        previousEnd = last;
      }

      if (!numSegAfterCheck)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      cMapInfo.reset(format, CMapData::kFlagInitialized, numSegAfterCheck, FontDataRange { dataOffset, tableSize });
      return kErrorOk;
    }

    // ------------------------------------------------------------------------
    // [Format 6 - Trimmed Table Mapping]
    // ------------------------------------------------------------------------

    case 6: {
      if (!dataRegion.fitsTable<CMapTable::Format6>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Format6* table = dataRegion.data<CMapTable::Format6>();
      uint32_t tableSize = table->length();

      if (tableSize < CMapTable::Format6::kMinSize || tableSize > dataRegion.size())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      uint32_t first = table->first();
      uint32_t count = table->count();

      if (!count || first + count > 0xFFFFu)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      if (tableSize < sizeof(CMapTable::Format10) + count * 2u)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      cMapInfo.reset(format, CMapData::kFlagInitialized, count, FontDataRange { dataOffset, tableSize });
      return kErrorOk;
    }

    // ------------------------------------------------------------------------
    // [Format 8 - Mixed 16-Bit and 32-Bit Coverage]
    // ------------------------------------------------------------------------

    case 8: {
      return DebugUtils::errored(kErrorFontUnsupportedCMap);
    };

    // ------------------------------------------------------------------------
    // [Format 10 - Trimmed Array]
    // ------------------------------------------------------------------------

    case 10: {
      if (!dataRegion.fitsTable<CMapTable::Format10>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Format10* table = dataRegion.data<CMapTable::Format10>();
      uint32_t tableSize = table->length();

      if (tableSize < CMapTable::Format10::kMinSize || tableSize > dataRegion.size())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      uint32_t first = table->first();
      uint32_t count = table->count();

      if (first >= Unicode::kCharMax || !count || count > Unicode::kCharMax || first + count > Unicode::kCharMax)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      if (tableSize < sizeof(CMapTable::Format10) + count * 2u)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      cMapInfo.reset(format, CMapData::kFlagInitialized, count, FontDataRange { dataOffset, tableSize });
      return kErrorOk;
    }

    // ------------------------------------------------------------------------
    // [Format 12 / 13 - Segmented Coverage / Many-To-One Range Mappings]
    // ------------------------------------------------------------------------

    case 12:
    case 13: {
      if (!dataRegion.fitsTable<CMapTable::Format12_13>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Format12_13* table = dataRegion.data<CMapTable::Format12_13>();
      uint32_t tableSize = table->length();

      if (tableSize < CMapTable::Format12_13::kMinSize || tableSize > dataRegion.size())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      uint32_t count = table->count();
      if (count > Unicode::kCharMax || tableSize < sizeof(CMapTable::Format12_13) + count * sizeof(CMapTable::Group))
        return DebugUtils::errored(kErrorFontInvalidCMap);

      const CMapTable::Group* groupArray = table->groupArray();
      uint32_t first = groupArray[0].first();
      uint32_t last = groupArray[0].last();

      if (first > last || last > Unicode::kCharMax)
        return DebugUtils::errored(kErrorFontInvalidCMap);

      for (uint32_t i = 1; i < count; i++) {
        first = groupArray[i].first();
        if (first <= last)
          return DebugUtils::errored(kErrorFontInvalidCMap);

        last = groupArray[i].last();
        if (first > last || last > Unicode::kCharMax)
          return DebugUtils::errored(kErrorFontInvalidCMap);
      }

      cMapInfo.reset(format, CMapData::kFlagInitialized, count, FontDataRange { dataOffset, tableSize });
      return kErrorOk;
    }

    // ------------------------------------------------------------------------
    // [Format 14 - Unicode Variation Sequences]
    // ------------------------------------------------------------------------

    case 14: {
      if (!dataRegion.fitsTable<CMapTable::Format14>())
        return DebugUtils::errored(kErrorFontInvalidCMap);

      // TODO: CMAP Format14 not implemented.
      return DebugUtils::errored(kErrorFontUnsupportedCMap);
    }

    // ------------------------------------------------------------------------
    // [Invalid / Unknown]
    // ------------------------------------------------------------------------

    default: {
      return DebugUtils::errored(kErrorFontInvalidCMap);
    }
  }
}

// ============================================================================
// [b2d::OT::CMapImpl - InitFuncs]
// ============================================================================

static void CMapImpl_initFuncs(OTFaceImpl* faceI) noexcept {
  FontFaceFuncs& funcs = faceI->_funcs;
  switch (faceI->_cMapData.format()) {
    case 0:
      funcs.mapGlyphItems = CMapImpl_Format0_mapGlyphItems;
      break;

    case 4:
      funcs.mapGlyphItems = CMapImpl_Format4_mapGlyphItems;
      break;

    case 6:
      funcs.mapGlyphItems = CMapImpl_Format6_mapGlyphItems;
      break;

    case 10:
      funcs.mapGlyphItems = CMapImpl_Format10_mapGlyphItems;
      break;

    case 12:
      funcs.mapGlyphItems = CMapImpl_Format12_13_mapGlyphItems<12>;
      break;

    case 13:
      funcs.mapGlyphItems = CMapImpl_Format12_13_mapGlyphItems<13>;
      break;

    default:
      funcs.mapGlyphItems = FontFace::none().impl()->_funcs.mapGlyphItems;
      break;
  }
}

// ============================================================================
// [b2d::OT::CMapImpl - InitFace]
// ============================================================================

Error CMapImpl::initCMap(OTFaceImpl* faceI, FontData& fontData) noexcept {
  FontDataTable<CMapTable> cMap;
  if (!fontData.queryTableBySlot(&cMap, FontData::kSlotCMap))
    return kErrorOk;

  if (!cMap.fitsTable<CMapTable>())
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidCMapData);

  uint32_t encodingCount = cMap->encodingCount();
  if (cMap.size() < sizeof(CMapTable) + encodingCount * sizeof(CMapTable::Encoding)) {
    faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagInvalidCMapData;
    return DebugUtils::errored(kErrorFontInvalidData);
  }

  enum Score : uint32_t {
    kScoreNothing    = 0x00000u,
    kScoreMacRoman   = 0x00001u, // Not sure this would ever be used, but OpenType sanitizer passes this one.
    kScoreSymbolFont = 0x00002u,
    kScoreAnyUnicode = 0x10000u,
    kScoreWinUnicode = 0x20000u  // Prefer Windows-Unicode CMAP over Unicode.
  };

  CMapData matchedCMap {};
  uint32_t matchedScore = kScoreNothing;
  bool validationFailed = false;

  for (uint32_t i = 0; i < encodingCount; i++) {
    uint32_t thisScore = kScoreNothing;
    const CMapTable::Encoding& encoding = cMap->encodingArray()[i];

    uint32_t platformId = encoding.platformId();
    uint32_t encodingId = encoding.encodingId();
    uint32_t offset = encoding.offset();

    if (offset >= cMap.size() - 4)
      continue;

    uint32_t format = Support::advancePtr(cMap.data<OTUInt16>(), offset)->value();
    if (format == 8)
      continue;

    switch (platformId) {
      case Platform::kPlatformUnicode:
        thisScore = kScoreAnyUnicode + encodingId;
        break;

      case Platform::kPlatformWindows:
        if (encodingId == Platform::kWindowsEncodingSymbol) {
          thisScore = kScoreSymbolFont;
          faceI->_faceFlags |= FontFace::kFlagSymbolFont;
        }
        else if (encodingId == Platform::kWindowsEncodingUCS2 || encodingId == Platform::kWindowsEncodingUCS4) {
          thisScore = kScoreWinUnicode + encodingId;
        }
        break;

      case Platform::kPlatformMac:
        if (encodingId == Platform::kMacEncodingRoman && format == 0)
          thisScore = kScoreMacRoman;
        break;
    }

    if (thisScore > matchedScore) {
      CMapData thisCMap {};
      Error err = validateEncoding(cMap.subRegion(offset, cMap.size() - offset), offset, thisCMap);

      if (err != kErrorOk) {
        if (err == kErrorFontUnsupportedCMap) {
          faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagInvalidCMapFormat;
        }
        else {
          validationFailed = true;
          faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagInvalidCMapData;
        }
        continue;
      }

      matchedCMap = thisCMap;
      matchedScore = thisScore;
    }
  }

  if (matchedScore) {
    faceI->_faceFlags |= FontFace::kFlagCharToGlyphMapping;
    faceI->_cMapData = matchedCMap;
    CMapImpl_initFuncs(faceI);

    return kErrorOk;
  }
  else {
    faceI->_cMapData.reset();
    // There is a big difference between these two, so always report the correct one.
    return DebugUtils::errored(validationFailed ? kErrorFontInvalidCMap : kErrorFontUnsupportedCMap);
  }
}

B2D_END_SUB_NAMESPACE
