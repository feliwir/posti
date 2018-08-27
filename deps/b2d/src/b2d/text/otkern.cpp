// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/support.h"
#include "../text/fontface.h"
#include "../text/glyphengine.h"
#include "../text/otface_p.h"
#include "../text/otkern_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

#if defined(B2D_TRACE_OT) && !defined(B2D_TRACE_OT_KERN)
  #define B2D_TRACE_OT_KERN
#endif

#ifdef B2D_TRACE_OT_KERN
  #define B2D_LOG_OT_KERN(...) DebugUtils::debugFormat(__VA_ARGS__)
#else
  #define B2D_LOG_OT_KERN(...) (void)0
#endif

namespace KernImpl {

// ============================================================================
// [b2d::OT::KernImpl - Utilities]
// ============================================================================

// Used to define a range of unsorted kerning pairs.
struct KernUnsortedRange {
  B2D_INLINE void reset(uint32_t start, uint32_t end) noexcept {
    this->start = start;
    this->end = end;
  }

  uint32_t start;
  uint32_t end;
};

// Checks whether the pairs in `pairArray` are sorted and can be b-searched.
// The last `start` arguments specifies the start index from which the check
// should start as this is required by some utilities here.
static size_t checkKernPairs(const KernTable::Pair* pairArray, size_t pairCount, size_t start) noexcept {
  if (start >= pairCount)
    return pairCount;

  size_t i;
  uint32_t prev = pairArray[start].combined();

  for (i = start; i < pairCount; i++) {
    uint32_t pair = pairArray[i].combined();
    // We must use `prev > pair`, because some fonts have kerning pairs
    // duplicated for no reason (the same values repeated). This doesn't
    // violate the binary search requirements so we are okay with it.
    if (B2D_UNLIKELY(prev > pair))
      break;
    prev = pair;
  }

  return i;
}

// Finds ranges of sorted pairs that can be used and creates ranges of unsorted
// pairs that will be merged into a single (synthesized) range of pairs. This
// function is only called if the kerning data in 'kern' is not sorted, and thus
// has to be fixed.
static Error fixUnsortedKernPairs(KernData& kernData, const KernTable::Format0* fmtData, uint32_t dataOffset, uint32_t pairCount, size_t currentIndex) noexcept {
  typedef KernTable::Pair Pair;

  enum : uint32_t {
    kMaxGroups    = 8, // Maximum number of sub-ranges of sorted pairs.
    kMinPairCount = 32 // Minimum number of pairs in a sub-range.
  };

  size_t rangeStart = 0;
  size_t unsortedStart = 0;
  size_t threshold = Math::max<size_t>((pairCount - rangeStart) / kMaxGroups, kMinPairCount);

  // Small ranges that are unsorted will be copied into a single one and then sorted.
  // Number of ranges must be `kMaxGroups + 1` to consider also a last trailing range.
  KernUnsortedRange unsortedRanges[kMaxGroups + 1];
  size_t unsortedCount = 0;
  size_t unsortedPairSum = 0;

  B2D_PROPAGATE(kernData._groups.reserve(kernData._groups.size() + kMaxGroups + 1));

  for (;;) {
    size_t rangeLength = (currentIndex - rangeStart);

    if (rangeLength >= threshold) {
      if (rangeStart != unsortedStart) {
        B2D_ASSERT(unsortedCount < B2D_ARRAY_SIZE(unsortedRanges));

        unsortedRanges[unsortedCount].reset(uint32_t(unsortedStart), uint32_t(rangeStart));
        unsortedPairSum += rangeStart - unsortedStart;
        unsortedCount++;
      }

      unsortedStart = currentIndex;
      uint32_t subOffset = uint32_t(dataOffset + rangeStart * sizeof(Pair));

      // Cannot fail as we reserved enough.
      B2D_LOG_OT_KERN("    [FIXUP] Adding Sorted Range [%zu:%zu]\n", rangeStart, currentIndex);
      kernData._groups.append(KernData::Group::makeLinked(subOffset, uint32_t(rangeLength)));
    }

    rangeStart = currentIndex;
    if (currentIndex == pairCount)
      break;

    currentIndex = checkKernPairs(fmtData->pairArray(), pairCount, currentIndex);
  }

  // Trailing unsorted range.
  if (unsortedStart != pairCount) {
    B2D_ASSERT(unsortedCount < B2D_ARRAY_SIZE(unsortedRanges));

    unsortedRanges[unsortedCount].reset(uint32_t(unsortedStart), uint32_t(rangeStart));
    unsortedPairSum += pairCount - unsortedStart;
    unsortedCount++;
  }

  if (unsortedPairSum) {
    Pair* synthesizedPairs = static_cast<Pair*>(Allocator::systemAlloc(unsortedPairSum * sizeof(Pair)));
    if (B2D_UNLIKELY(!synthesizedPairs))
      return DebugUtils::errored(kErrorNoMemory);

    size_t synthesizedIndex = 0;
    for (size_t i = 0; i < unsortedCount; i++) {
      KernUnsortedRange& r = unsortedRanges[i];
      size_t rangeLength = (r.end - r.start);

      B2D_LOG_OT_KERN("    [FIXUP] Adding Synthesized Range [%zu:%zu]\n", size_t(r.start), size_t(r.end));
      std::memcpy(synthesizedPairs + synthesizedIndex, fmtData->pairArray() + r.start, rangeLength * sizeof(Pair));

      synthesizedIndex += rangeLength;
    }
    B2D_ASSERT(synthesizedIndex == unsortedPairSum);

    Support::qSort(synthesizedPairs, unsortedPairSum, [](const Pair& a, const Pair& b) noexcept -> int {
      uint32_t aCombined = a.combined();
      uint32_t bCombined = b.combined();
      return aCombined < bCombined ? -1 : aCombined > bCombined ? 1 : 0;
    });

    // Cannot fail as we reserved enough.
    kernData._groups.append(KernData::Group::makeSynthesized(synthesizedPairs, uint32_t(unsortedPairSum)));
  }

  return kErrorOk;
}

static B2D_INLINE const KernTable::Pair* findKernPair(const KernTable::Pair* pairs, size_t count, uint32_t pair) noexcept {
  for (size_t i = count; i != 0; i >>= 1) {
    const KernTable::Pair* candidate = pairs + (i >> 1);
    uint32_t combined = candidate->combined();

    if (pair < combined)
      continue;

    i--;
    pairs = candidate + 1;
    if (pair > combined)
      continue;

    return candidate;
  }

  return nullptr;
}

// ============================================================================
// [b2d::OT::KernImpl - Format 0]
// ============================================================================

static Error B2D_CDECL Format0_applyPairAdjustment(const GlyphEngine* self, GlyphItem* glyphItems, Point* glyphOffsets, size_t count) noexcept {
  typedef KernData::Group Group;
  typedef KernTable::Pair Pair;

  if (count < 2)
    return kErrorOk;

  OTFaceImpl* faceI = self->fontFace().impl<OTFaceImpl>();
  const void* basePtr = self->fontData().loadedTablePtrBySlot<void>(FontData::kSlotKern);

  const KernData& kernData = faceI->_kernData[0];
  const Group* groupArray = kernData._groups.data();
  size_t groupCount = kernData._groups.size();

  uint32_t pair = uint32_t(glyphItems[0].glyphId()) << 16;
  for (size_t i = 1; i < count; i++, pair <<= 16) {
    pair |= uint32_t(glyphItems[i].glyphId());

    for (size_t groupIndex = 0; groupIndex < groupCount; groupIndex++) {
      const Group& group = groupArray[groupIndex];
      const Pair* match = findKernPair(group.pairs(basePtr), group.pairCount(), pair);

      if (!match)
        continue;

      glyphOffsets[i - 1]._x += double(match->value());
      break;
    }
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::KernImpl - Init]
// ============================================================================

Error initKern(OTFaceImpl* faceI, FontData& fontData) noexcept {
  FontDataTable<KernTable> kern;
  if (!fontData.queryTableByTag(&kern, FontTag::kern))
    return kErrorOk;

  typedef TextOrientation TO;
  typedef KernTable::WinGroupHeader WinGroupHeader;
  typedef KernTable::MacGroupHeader MacGroupHeader;

  B2D_LOG_OT_KERN("OT::Init 'kern'\n");
  B2D_LOG_OT_KERN("  Size: %zu\n", size_t(kern.size()));

  if (B2D_UNLIKELY(!kern.fitsTable<KernTable>()))
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagTruncatedKernData);

  const uint8_t* dataPtr = kern.data<uint8_t>();
  const uint8_t* dataEnd = dataPtr + kern.size();

  // --------------------------------------------------------------------------
  // [Header]
  // --------------------------------------------------------------------------

  // Detect the header format. Windows header uses 16-bit field describing the
  // version of the table and only defines version 0. Apple uses a different
  // header format which uses a 32-bit version number (`OTFixed`). Luckily we
  // can distinguish between these two easily.
  uint32_t majorVersion = OTUtils::readU16(dataPtr);

  uint32_t headerType = KernData::kHeaderNone;
  uint32_t headerSize = 0;
  uint32_t groupCount = 0;

  if (majorVersion == 0) {
    headerType = KernData::kHeaderWindows;
    headerSize = uint32_t(sizeof(WinGroupHeader));
    groupCount = OTUtils::readU16(dataPtr + 2u);

    B2D_LOG_OT_KERN("  Version: 0 (WINDOWS)\n");
    B2D_LOG_OT_KERN("  GroupCount: %u\n", groupCount);

    // Not forbidden by the spec, just ignore the table if true.
    if (!groupCount) {
      B2D_LOG_OT_KERN("  [WARN] No kerning pairs defined\n");
      return kErrorOk;
    }

    dataPtr += 4;
  }
  else if (majorVersion == 1) {
    uint32_t minorVersion = OTUtils::readU16(dataPtr + 2u);
    B2D_LOG_OT_KERN("  Version: 1 (MAC)\n");

    if (minorVersion != 0) {
      B2D_LOG_OT_KERN("  [WARN] Invalid minor version (%u)\n", minorVersion);
      return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidKernData);
    }

    // Minimum mac header is 8 bytes. We have to check this explicitly as the
    // minimum size of "any" header is 4 bytes, so make sure we won't read beyond.
    if (kern.size() < 8u) {
      B2D_LOG_OT_KERN("  [WARN] InvalidSize: %zu\n", size_t(kern.size()));
      return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagTruncatedKernData);
    }

    headerType = KernData::kHeaderMac;
    headerSize = uint32_t(sizeof(MacGroupHeader));

    groupCount = OTUtils::readU32(dataPtr + 4u);
    B2D_LOG_OT_KERN("  GroupCount: %u\n", groupCount);

    // Not forbidden by the spec, just ignore the table if true.
    if (!groupCount) {
      B2D_LOG_OT_KERN("  [WARN] No kerning pairs defined\n");
      return kErrorOk;
    }

    dataPtr += 8;
  }
  else {
    B2D_LOG_OT_KERN("  Version: %u (UNKNOWN)\n", majorVersion);

    // No other major version is defined by OpenType. Since KERN table has
    // been superseded by "GPOS" table there will never be any other version.
    return faceI->bailWithDiagnostics(FontDiagnosticsInfo::kFlagInvalidKernData);
  }

  // --------------------------------------------------------------------------
  // [Groups]
  // --------------------------------------------------------------------------

  uint32_t groupIndex = 0;
  do {
    size_t remainingSize = (size_t)(dataEnd - dataPtr);
    if (B2D_UNLIKELY(remainingSize < headerSize)) {
      B2D_LOG_OT_KERN("  [WARN] No more data for group #%u\n", groupIndex);
      break;
    }

    uint32_t length = 0;
    uint32_t format = 0;
    uint32_t coverage = 0;
    B2D_LOG_OT_KERN("  Group #%u\n", groupIndex);

    if (headerType == KernData::kHeaderWindows) {
      const WinGroupHeader* group = reinterpret_cast<const WinGroupHeader*>(dataPtr);

      format = group->format();
      length = group->length();

      // Some fonts having only one group have an incorrect length set to the
      // same value as the as the whole 'kern' table. Detect it and fix it.
      if (length == kern.size() && groupCount == 1) {
        length = uint32_t(remainingSize);
        B2D_LOG_OT_KERN("    [FIXUP] Group length is same as the table length, fixed to %u\n", length);
      }

      // The last sub-table can have truncated length to 16 bits even when it
      // needs more to represent all kerning pairs. This is not covered by the
      // specification, but it's a common practice.
      if (length != remainingSize && groupIndex == groupCount - 1) {
        B2D_LOG_OT_KERN("    [FIXUP] Fixing reported length from %u to %zu\n", length, remainingSize);
        length = uint32_t(remainingSize);
      }

      // We don't have to translate coverage flags to KernData::Coverage as they are the same.
      coverage = group->coverage() & ~WinGroupHeader::kCoverageReservedBits;
    }
    else {
      const MacGroupHeader* group = reinterpret_cast<const MacGroupHeader*>(dataPtr);

      format = group->format();
      length = group->length();

      uint32_t macCoverage = group->coverage();
      if ((macCoverage & MacGroupHeader::kCoverageVertical   ) == 0) coverage |= KernData::kCoverageHorizontal;
      if ((macCoverage & MacGroupHeader::kCoverageCrossStream) != 0) coverage |= KernData::kCoverageCrossStream;
    }

    if (length < headerSize) {
      B2D_LOG_OT_KERN("    [ERROR] Group length too small [Length=%u RemainingSize=%zu]\n", length, remainingSize);
      return DebugUtils::errored(kErrorFontInvalidData);
    }

    if (length > remainingSize) {
      B2D_LOG_OT_KERN("    [ERROR] Group length exceeds the remaining space [Length=%u RemainingSize=%zu]\n", length, remainingSize);
      return DebugUtils::errored(kErrorFontInvalidData);
    }

    // Move to the beginning of the content of the group.
    dataPtr += headerSize;

    // It's easier to calculate everything without the header (as its size is
    // variable), so make `length` raw data size that we will store in KernData.
    length -= headerSize;
    remainingSize -= headerSize;

    // Even on 64-bit machine this cannot overflow as a table length in SFNT
    // header is stored as UInt32.
    uint32_t offset = (uint32_t)(size_t)(dataPtr - kern.data<uint8_t>());
    uint32_t orientation = (coverage & KernData::kCoverageHorizontal) ? TO::kHorizontal : TO::kVertical;

    B2D_LOG_OT_KERN("    Format: %u%s\n", format, format > 3 ? " (UNKNOWN)" : "");
    B2D_LOG_OT_KERN("    Coverage: %u\n", coverage);
    B2D_LOG_OT_KERN("    Orientation: %s\n", orientation == TO::kHorizontal ? "Horizontal" : "Vertical");

    KernData& kernData = faceI->_kernData[orientation];
    if (kernData.empty() || (kernData.format() == format && kernData.coverage() == coverage)) {
      switch (format) {
        // --------------------------------------------------------------------
        // [Format 0]
        // --------------------------------------------------------------------

        case 0: {
          if (length < sizeof(KernTable::Format0))
            break;

          const KernTable::Format0* fmtData = reinterpret_cast<const KernTable::Format0*>(dataPtr);
          uint32_t pairCount = fmtData->pairCount();

          B2D_LOG_OT_KERN("    PairCount=%zu\n", pairCount);

          if (pairCount == 0)
            break;

          uint32_t pairDataOffset = offset + 8;
          uint32_t pairDataSize = pairCount * uint32_t(sizeof(KernTable::Pair)) + uint32_t(sizeof(KernTable::Format0));

          if (B2D_UNLIKELY(pairDataSize > length)) {
            uint32_t fixedPairCount = (length - uint32_t(sizeof(KernTable::Format0))) / 6;
            B2D_LOG_OT_KERN("    [FIXUP] Fixing the number of pairs from %u to %u to match the remaining size [Required=%u, Remaining=%u]\n",
              pairCount,
              fixedPairCount,
              pairDataSize,
              length);

            pairCount = fixedPairCount;
            faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagTruncatedKernData;
          }

          // Check whether the pairs are sorted.
          const KernTable::Pair* pairData = fmtData->pairArray();
          size_t unsortedIndex = checkKernPairs(pairData, pairCount, 0);

          if (unsortedIndex != pairCount) {
            B2D_LOG_OT_KERN("    [FIXUP] Kerning pairs aren't sorted [ViolationIndex=#%zu]\n", unsortedIndex);
            faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagUnsortedKernPairs;

            Error err = fixUnsortedKernPairs(kernData, fmtData, pairDataOffset, pairCount, unsortedIndex);
            if (err) {
              B2D_LOG_OT_KERN("    [ERROR] Cannot allocate data for synthesized pairs\n");
              return err;
            }
          }
          else {
            Error err = kernData._groups.append(KernData::Group::makeLinked(pairDataOffset, uint32_t(pairCount)));
            if (err) {
              B2D_LOG_OT_KERN("    [ERROR] Cannot allocate data for linked pairs\n");
              return err;
            }
          }

          break;
        }

        // --------------------------------------------------------------------
        // [Unknown]
        // --------------------------------------------------------------------

        default:
          faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagInvalidKernFormat;
          break;
      }

      if (!kernData.empty()) {
        kernData._format = uint8_t(format);
        kernData._coverage = uint8_t(coverage);
      }
    }
    else {
      B2D_LOG_OT_KERN("    [Skipped]\n");
    }

    dataPtr += length;
  } while (++groupIndex < groupCount);

  if (!faceI->_kernData[TO::kHorizontal].empty()) {
    switch (faceI->_kernData[TO::kHorizontal].format()) {
      case 0:
        faceI->_funcs.applyPairAdjustment = Format0_applyPairAdjustment;
        faceI->_faceFlags |= FontFace::kFlagLegacyKerning;
        break;

      default:
        break;
    }
  }

  return kErrorOk;
}

} // KernImpl namespace

B2D_END_SUB_NAMESPACE
