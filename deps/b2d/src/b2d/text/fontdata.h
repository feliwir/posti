// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTDATA_H
#define _B2D_TEXT_FONTDATA_H

// [Dependencies]
#include "../core/any.h"
#include "../core/array.h"
#include "../core/support.h"
#include "../text/fonttag.h"
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontDataRange]
// ============================================================================

//! A range that specifies offset and length relative to start of font-face data.
struct FontDataRange {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0, 0); }
  B2D_INLINE void reset(uint32_t offset, uint32_t length) noexcept {
    _offset = offset;
    _length = length;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr uint32_t offset() const noexcept { return _offset; }
  constexpr uint32_t length() const noexcept { return _length; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _offset;
  uint32_t _length;
};

// ============================================================================
// [b2d::FontDataRegion]
// ============================================================================

class FontDataRegion {
public:
  inline FontDataRegion() noexcept = default;
  constexpr FontDataRegion(const void* data, size_t size) noexcept
    : _data(data),
      _size(size) {}
  constexpr FontDataRegion(const FontDataRegion& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(nullptr, 0); }
  B2D_INLINE void reset(const void* data, size_t size) noexcept { _data = data; _size = size; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool empty() const noexcept { return _size == 0; }
  constexpr size_t size() const noexcept { return _size; }

  template<typename T = void>
  constexpr const T* data() const noexcept { return static_cast<const T*>(_data); }

  constexpr bool fitsSize(size_t size) const noexcept { return _size >= size; }
  constexpr bool fitsRange(size_t offset, size_t length) const noexcept { return _size >= offset && _size - offset >= length; }
  constexpr bool fitsRange(const FontDataRange& range) const noexcept { return fitsRange(range.offset(), range.length()); }
  template<typename T>
  constexpr bool fitsTable() const noexcept { return fitsSize(T::kMinSize); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  constexpr FontDataRegion subRegion(size_t offset, size_t length) const noexcept {
    return FontDataRegion { data<uint8_t>() + offset, length };
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline FontDataRegion& operator=(const FontDataRegion& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const void* _data;
  size_t _size;
};

// ============================================================================
// [b2d::FontDataTable]
// ============================================================================

//! A convenience class that maps a `FontDataRegion` to a particular TrueType or
//! OpenType table directly.
template<typename Table>
class FontDataTable : public FontDataRegion {
public:
  inline FontDataTable() noexcept = default;

  constexpr FontDataTable(const FontDataRegion& other) noexcept
    : FontDataRegion(other) {}

  constexpr FontDataTable(const FontDataTable& other) noexcept = default;

  constexpr FontDataTable(const void* data, size_t size) noexcept
    : FontDataRegion(data, size) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T = Table>
  constexpr bool fitsTable() const noexcept { return _size >= T::kMinSize; }

  template<typename T = Table>
  constexpr const T* data() const noexcept { return static_cast<const T*>(_data); }

  constexpr const Table* operator->() const noexcept { return data<Table>(); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline FontDataTable& operator=(const FontDataTable& other) noexcept = default;
};

// ============================================================================
// [b2d::FontData - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontDataImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontDataImpl)

  enum : uint32_t {
    kSlotCount = 32
  };

  B2D_API FontDataImpl() noexcept;
  B2D_API virtual ~FontDataImpl() noexcept;

  static B2D_API FontDataImpl* newFromMemory(const void* data, size_t size) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t slotMask() const noexcept { return _commonData._extra32; }
  B2D_INLINE void addToSlotMask(uint32_t mask) noexcept { _commonData._extra32 |= mask; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual Error init() noexcept;

  B2D_API virtual Error listTags(Array<uint32_t>& out) noexcept;
  B2D_API virtual Error loadSlots(uint32_t slotMask) noexcept;

  template<typename T>
  B2D_INLINE const T* loadedTablePtrBySlot(uint32_t slot) const noexcept {
    B2D_ASSERT(slot < kSlotCount);
    return static_cast<const T*>(
      static_cast<const void*>(_tableData[slot]));
  }

  B2D_INLINE uint32_t loadedTableSizeBySlot(uint32_t slot) const noexcept {
    B2D_ASSERT(slot < kSlotCount);
    return _tableSize[slot];
  }

  B2D_INLINE FontDataRegion loadedTableBySlot(uint32_t slot) const noexcept {
    B2D_ASSERT(slot < kSlotCount);
    return FontDataRegion { _tableData[slot], _tableSize[slot] };
  }

  B2D_API virtual size_t queryTablesByTags(FontDataRegion* out, const uint32_t* tags, size_t count) noexcept;
  B2D_API virtual size_t queryTablesBySlots(FontDataRegion* out, const uint8_t* slots, size_t count) noexcept;

  B2D_INLINE size_t queryTableByTag(FontDataRegion* out, uint32_t tag) noexcept { return queryTablesByTags(out, &tag, 1); }
  B2D_INLINE size_t queryTableBySlot(FontDataRegion* out, uint8_t slot) noexcept { return queryTablesBySlots(out, &slot, 1); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const uint8_t* _tableData[kSlotCount]; //!< Table data by slot-id.
  uint32_t _tableSize[kSlotCount];       //!< Table size by slot-id.
};

// ============================================================================
// [b2d::FontData]
// ============================================================================

class FontData : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdFontData };

  // --------------------------------------------------------------------------
  // [Slot]
  // --------------------------------------------------------------------------

  //! Table slot.
  //!
  //! The most used tables have a slot index that is used to retrieve them faster
  //! than going through generic `queryTableByTag()` function. There are 32 slots
  //! reserved for such tables at the moment.
  //!
  //! Tables that are typically read only once to setup the font-face were omitted
  //! as they would reserve slots and would be only used once. These include 'OS/2',
  //! 'head', 'maxp', and similar.
  //!
  //! HACKING: If you are adding new slots then don't forget to add new slots also
  //! to `slotFromTag()` and `tagFromSlot()` functions, otherwise the slots won't
  //! be visible to `FontData`. The rest is handled automatically.
  enum Slot : uint8_t {
    kSlotBase    = 0,
    kSlotCMap    = 1,
    kSlotGDef    = 2,
    kSlotGPos    = 3,
    kSlotGSub    = 4,
    kSlotKern    = 5,
    kSlotHMtx    = 6,
    kSlotVMtx    = 7,
    kSlotJSTF    = 8,
    kSlotMath    = 9,
    kSlotAVar    = 10,
    kSlotCVar    = 11,
    kSlotFVar    = 12,
    kSlotGVar    = 13,
    kSlotMVar    = 14,
    kSlotHVar    = 15,
    kSlotVVar    = 16,
    kSlotLoca    = 17,
    kSlotGlyf    = 18,
    kSlotCvt     = 19,
    kSlotGasp    = 20,
    kSlotCFF     = 21,
    kSlotCFF2    = 22,

    kSlotCount   = 32,
    kSlotInvalid = 0xFFu
  };

  static_assert(FontDataImpl::kSlotCount == kSlotCount,
                "b2d::FontDataImpl::kSlotCount must match b2d::FontData::kSlotCount");

  // These are mostly for internal purposes and useful in cases when a conversion
  // happens at compile-time, which is used by `FontData::table()`, for example.
  static constexpr uint8_t slotFromTag(uint32_t tag) noexcept {
    return tag == FontTag::BASE ? FontData::kSlotBase :
           tag == FontTag::cmap ? FontData::kSlotCMap :
           tag == FontTag::GDEF ? FontData::kSlotGDef :
           tag == FontTag::GPOS ? FontData::kSlotGPos :
           tag == FontTag::GSUB ? FontData::kSlotGSub :
           tag == FontTag::kern ? FontData::kSlotKern :
           tag == FontTag::hmtx ? FontData::kSlotHMtx :
           tag == FontTag::vmtx ? FontData::kSlotVMtx :
           tag == FontTag::JSTF ? FontData::kSlotJSTF :
           tag == FontTag::MATH ? FontData::kSlotMath :
           tag == FontTag::avar ? FontData::kSlotAVar :
           tag == FontTag::cvar ? FontData::kSlotCVar :
           tag == FontTag::fvar ? FontData::kSlotFVar :
           tag == FontTag::gvar ? FontData::kSlotGVar :
           tag == FontTag::MVAR ? FontData::kSlotMVar :
           tag == FontTag::HVAR ? FontData::kSlotHVar :
           tag == FontTag::VVAR ? FontData::kSlotVVar :
           tag == FontTag::loca ? FontData::kSlotLoca :
           tag == FontTag::glyf ? FontData::kSlotGlyf :
           tag == FontTag::cvt  ? FontData::kSlotCvt  :
           tag == FontTag::gasp ? FontData::kSlotGasp :
           tag == FontTag::CFF  ? FontData::kSlotCFF  :
           tag == FontTag::CFF2 ? FontData::kSlotCFF2 : kSlotInvalid;
  }

  static constexpr uint32_t tagFromSlot(uint8_t slot) noexcept {
    return slot == FontData::kSlotBase ? FontTag::BASE :
           slot == FontData::kSlotCMap ? FontTag::cmap :
           slot == FontData::kSlotGDef ? FontTag::GDEF :
           slot == FontData::kSlotGPos ? FontTag::GPOS :
           slot == FontData::kSlotGSub ? FontTag::GSUB :
           slot == FontData::kSlotKern ? FontTag::kern :
           slot == FontData::kSlotHMtx ? FontTag::hmtx :
           slot == FontData::kSlotVMtx ? FontTag::vmtx :
           slot == FontData::kSlotJSTF ? FontTag::JSTF :
           slot == FontData::kSlotMath ? FontTag::MATH :
           slot == FontData::kSlotAVar ? FontTag::avar :
           slot == FontData::kSlotCVar ? FontTag::cvar :
           slot == FontData::kSlotFVar ? FontTag::fvar :
           slot == FontData::kSlotGVar ? FontTag::gvar :
           slot == FontData::kSlotMVar ? FontTag::MVAR :
           slot == FontData::kSlotHVar ? FontTag::HVAR :
           slot == FontData::kSlotVVar ? FontTag::VVAR :
           slot == FontData::kSlotLoca ? FontTag::loca :
           slot == FontData::kSlotGlyf ? FontTag::glyf :
           slot == FontData::kSlotCvt  ? FontTag::cvt  :
           slot == FontData::kSlotGasp ? FontTag::gasp :
           slot == FontData::kSlotCFF  ? FontTag::CFF  :
           slot == FontData::kSlotCFF2 ? FontTag::CFF2 : FontTag::none;
  }

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE FontData() noexcept
    : Object(none().impl()) {}

  B2D_INLINE FontData(const FontData& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE FontData(FontData&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit FontData(FontDataImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const FontData& none() noexcept { return Any::noneOf<FontData>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = FontDataImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE Error reset() noexcept { return AnyInternal::replaceImpl(this, none().impl()); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return isNone(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE Error listTags(Array<uint32_t>& out) const noexcept {
    return impl()->listTags(out);
  }

  B2D_INLINE bool canLoadSlot(uint32_t slot) const noexcept { return Support::bitTest(impl()->slotMask(), slot); }
  B2D_INLINE bool isSlotLoaded(uint32_t slot) const noexcept { return impl()->_tableData[slot] != nullptr; }

  B2D_INLINE uint32_t slotMask() const noexcept { return impl()->slotMask(); }
  B2D_INLINE Error loadSlots(uint32_t slotMask) noexcept { return impl()->loadSlots(slotMask); }

  template<typename T>
  B2D_INLINE const T* loadedTablePtrBySlot(uint32_t slot) const noexcept { return impl()->loadedTablePtrBySlot<T>(slot); }

  B2D_INLINE uint32_t loadedTableSizeBySlot(uint32_t slot) const noexcept { return impl()->loadedTableSizeBySlot(slot); }
  B2D_INLINE FontDataRegion loadedTableBySlot(uint32_t slot) const noexcept { return impl()->loadedTableBySlot(slot); }

  B2D_INLINE size_t queryTableByTag(FontDataRegion* out, uint32_t tag) noexcept {
    return impl()->queryTableByTag(out, tag);
  }

  B2D_INLINE size_t queryTablesByTags(FontDataRegion* out, const uint32_t* tags, size_t count) noexcept {
    return impl()->queryTablesByTags(out, tags, count);
  }

  B2D_INLINE size_t queryTableBySlot(FontDataRegion* out, uint8_t slot) noexcept {
    return impl()->queryTableBySlot(out, slot);
  }

  B2D_INLINE size_t queryTablesBySlots(FontDataRegion* out, const uint8_t* slots, size_t count) noexcept {
    return impl()->queryTablesBySlots(out, slots, count);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FontData& operator=(const FontData& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE FontData& operator=(FontData&& other) noexcept {
    FontDataImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTDATA_H
