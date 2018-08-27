// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/array.h"
#include "../core/lookuptable_p.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/fonttag.h"
#include "../text/otglobals_p.h"
#include "../text/otsfnt_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::FontData - Globals]
// ============================================================================

static Wrap<FontDataImpl> gFontDataImplNone;

// ============================================================================
// [b2d::FontData - Impl]
// ============================================================================

FontDataImpl::FontDataImpl() noexcept
  : ObjectImpl(Any::kTypeIdFontData),
    _tableData {},
    _tableSize {} {}
FontDataImpl::~FontDataImpl() noexcept {}

// ============================================================================
// [b2d::FontData - Interface]
// ============================================================================

Error FontDataImpl::init() noexcept {
  return DebugUtils::errored(kErrorNotImplemented);
}

Error FontDataImpl::listTags(Array<uint32_t>& out) noexcept {
  out.reset();
  return kErrorOk;
}

Error FontDataImpl::loadSlots(uint32_t slotMask) noexcept {
  // If we went here it means this is either built-in none instance or the font
  // data doesn't need `loadSlots()` implemented (for example `MemFontDataImpl`).
  B2D_UNUSED(slotMask);
  return kErrorOk;
}

size_t FontDataImpl::queryTablesByTags(FontDataRegion* out, const uint32_t* tags, size_t count) noexcept {
  B2D_UNUSED(tags);

  for (size_t i = 0; i < count; i++)
    out[i].reset();

  return 0;
}

size_t FontDataImpl::queryTablesBySlots(FontDataRegion* out, const uint8_t* slots, size_t count) noexcept {
  B2D_UNUSED(slots);

  for (size_t i = 0; i < count; i++)
    out[i].reset();

  return 0;
}

// ============================================================================
// [b2d::MemFontData - Impl]
// ============================================================================

class MemFontDataImpl : public FontDataImpl {
public:
  MemFontDataImpl(const void* data, size_t size) noexcept;
  virtual ~MemFontDataImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  Error init() noexcept override;
  Error listTags(Array<uint32_t>& out) noexcept override;

  size_t queryTablesByTags(FontDataRegion* out, const uint32_t* tags, size_t count) noexcept override;
  size_t queryTablesBySlots(FontDataRegion* out, const uint8_t* slots, size_t count) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const void* _data;                     //!< Raw data (beginning of the font).
  size_t _dataSize;                      //!< Raw data size.
};

MemFontDataImpl::MemFontDataImpl(const void* data, size_t size) noexcept
  : FontDataImpl(),
    _data(data),
    _dataSize(size) {}
MemFontDataImpl::~MemFontDataImpl() noexcept {}

Error MemFontDataImpl::init() noexcept {
  if (B2D_UNLIKELY(_dataSize < OT::SFNTHeader::kMinSize))
    return DebugUtils::errored(kErrorFontInvalidData);

  const OT::SFNTHeader* sfnt = reinterpret_cast<const OT::SFNTHeader*>(_data);
  uint32_t versionTag = sfnt->versionTag();

  // Version check.
  //
  // NOTE: Version check is also performed by `FontLoader`, but be sure...
  if (versionTag != OT::SFNTHeader::kVersionTagOpenType  &&
      versionTag != OT::SFNTHeader::kVersionTagTrueTypeA &&
      versionTag != OT::SFNTHeader::kVersionTagTrueTypeB)
    return DebugUtils::errored(kErrorFontInvalidSignature);

  // Check table count.
  uint32_t tableCount = sfnt->numTables();
  if (_dataSize < sizeof(OT::SFNTHeader) + tableCount * sizeof(OT::SFNTHeader::TableRecord))
    return DebugUtils::errored(kErrorFontInvalidData);

  // Check whether the indexes to each table are within the bounds and build
  // a slot index from all tables that match `FontData::slotFromTag()`.
  const OT::SFNTHeader::TableRecord* tables = sfnt->tableRecords();
  for (uint32_t tableIndex = 0; tableIndex < tableCount; tableIndex++) {
    const OT::SFNTHeader::TableRecord& table = tables[tableIndex];

    uint32_t offset = table.offset();
    uint32_t length = table.length();

    if (B2D_UNLIKELY(offset >= _dataSize || length > _dataSize - offset))
      return DebugUtils::errored(kErrorFontInvalidData);

    uint32_t tag = table.tag();
    uint32_t slot = FontData::slotFromTag(tag);

    if (slot != FontData::kSlotInvalid) {
      B2D_ASSERT(slot < FontData::kSlotCount);

      // If assigned it would mean the table is specified multiple times.
      // This is of course error in the file, but since don't check
      // duplicates and always return the first table we would just
      // ignore everything that comes after.
      if (!Support::bitTest(slotMask(), slot)) {
        addToSlotMask(Support::bitMask<uint32_t>(slot));
        _tableData[slot] = static_cast<const uint8_t*>(_data) + offset;
        _tableSize[slot] = length;
      }
    }
  }

  return kErrorOk;
}

Error MemFontDataImpl::listTags(Array<uint32_t>& out) noexcept {
  const OT::SFNTHeader* sfnt = static_cast<const OT::SFNTHeader*>(_data);
  const OT::SFNTHeader::TableRecord* tables = sfnt->tableRecords();

  size_t tableCount = sfnt->numTables();
  B2D_ASSERT(sizeof(OT::SFNTHeader) + tableCount * sizeof(OT::SFNTHeader::TableRecord) <= _dataSize);

  uint32_t* dst = out._commonOp(kContainerOpReplace, tableCount);
  if (B2D_UNLIKELY(!dst))
    return DebugUtils::errored(kErrorNoMemory);

  for (size_t tableIndex = 0; tableIndex < tableCount; tableIndex++) {
    const OT::SFNTHeader::TableRecord& table = tables[tableIndex];
    dst[tableIndex] = table.tag();
  }

  return kErrorOk;
}

size_t MemFontDataImpl::queryTablesByTags(FontDataRegion* out, const uint32_t* tags, size_t count) noexcept {
  if (B2D_UNLIKELY(!count))
    return 0;

  const OT::SFNTHeader* sfnt = static_cast<const OT::SFNTHeader*>(_data);
  const OT::SFNTHeader::TableRecord* tables = sfnt->tableRecords();

  size_t tableCount = sfnt->numTables();
  B2D_ASSERT(sizeof(OT::SFNTHeader) + tableCount * sizeof(OT::SFNTHeader::TableRecord) <= _dataSize);

  // Iterate over all tables and try to find all tables as specified by `tags`.
  size_t matchCount = 0;
  for (size_t i = 0; i < count; i++) {
    uint32_t tag = tags[i];
    out[i].reset();

    for (size_t tableIndex = 0; tableIndex < tableCount; tableIndex++) {
      const OT::SFNTHeader::TableRecord& table = tables[tableIndex];

      if (table.tag() == tag) {
        uint32_t tableOffset = table.offset();
        uint32_t tableSize = table.length();

        if (tableOffset < _dataSize && tableSize && tableSize <= _dataSize - tableOffset) {
          matchCount++;
          out[i].reset(Support::advancePtr(_data, tableOffset), tableSize);
        }

        break;
      }
    }
  }

  return matchCount;
}

size_t MemFontDataImpl::queryTablesBySlots(FontDataRegion* out, const uint8_t* slots, size_t count) noexcept {
  if (B2D_UNLIKELY(!count))
    return 0;

  // NOTE: Since we have already processed the header we know exactly whether
  // the tables we have slots for are within the font or not. In addition, we
  // already have pointers to their data, so this is quick.
  size_t matchCount = 0;
  for (size_t i = 0; i < count; i++) {
    uint32_t slot = slots[i];
    if (slot >= FontData::kSlotCount) {
      out[i].reset();
    }
    else {
      out[i].reset(_tableData[slot], _tableSize[slot]);
      matchCount += (_tableSize[slot] != 0);
    }
  }

  return matchCount;
}

FontDataImpl* FontDataImpl::newFromMemory(const void* data, size_t size) noexcept {
  return AnyInternal::newImplT<MemFontDataImpl>(data, size);
}

// ============================================================================
// [b2d::FontData - Runtime Handlers]
// ============================================================================

void FontDataOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontDataImpl* fontDataI = gFontDataImplNone.init();
  fontDataI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFontData, fontDataI);
}

B2D_END_NAMESPACE
