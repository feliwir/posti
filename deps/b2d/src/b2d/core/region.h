// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_REGION_H
#define _B2D_CORE_REGION_H

// [Dependencies]
#include "../core/any.h"
#include "../core/container.h"
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class Region;

// ============================================================================
// [b2d::Region - Ops]
// ============================================================================

struct RegionOps {
  Error (B2D_CDECL* translate)(Region& dst, const Region& src, const IntPoint& pt) noexcept;
  bool (B2D_CDECL* eq)(const Region* a, const Region* b) noexcept;
};

B2D_VARAPI RegionOps _bRegionOps;

// ============================================================================
// [b2d::Region - Impl]
// ============================================================================

//! Region implementation.
struct RegionImpl : public SimpleImpl {
  B2D_INLINE RegionImpl() noexcept = default;

  // Used only to initialize statically allocated built-in RegionImpl.
  constexpr RegionImpl(uint32_t objectInfo, size_t size, int x0, int y0, int x1, int y1) noexcept
    : _capacity { size },
      _commonData { ATOMIC_VAR_INIT(0), objectInfo, { 0 } },
      _size { size },
      _boundingBox { x0, y0, x1, y1 },
      _data { { x0, y0, x1, y1 } } {}

  static constexpr size_t sizeOf(size_t capacity) noexcept {
    return sizeof(RegionImpl) - sizeof(IntBox) + capacity * sizeof(IntBox);
  }

  static B2D_INLINE RegionImpl* new_(size_t capacity) noexcept {
    size_t implSize = sizeOf(capacity);
    RegionImpl* impl = AnyInternal::allocSizedImplT<RegionImpl>(implSize);
    return impl ? impl->_initImpl(Any::kTypeIdRegion, capacity) : nullptr;
  }

  static B2D_INLINE RegionImpl* newTemporary(const Support::Temporary& temporary) noexcept {
    return temporary.data<RegionImpl>()->_initImpl(
      Any::kTypeIdRegion | Any::kFlagIsTemporary, temporary.size());
  }

  B2D_INLINE RegionImpl* _initImpl(uint32_t objectInfo, size_t capacity) noexcept {
    _commonData.reset(objectInfo);
    _size = 0;
    _capacity = capacity;
    _boundingBox.reset();
    return this;
  }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = RegionImpl>
  B2D_INLINE T* addRef() noexcept { return SimpleImpl::addRef<T>(); }

  B2D_INLINE void release() noexcept {
    if (deref() && !isTemporary())
      Allocator::systemRelease(this);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isInfinite() const noexcept { return (objectInfo() & Any::kFlagIsInfinite) != 0; }

  B2D_INLINE IntBox* data() noexcept { return _data; }
  B2D_INLINE const IntBox* data() const noexcept { return _data; }

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE const IntBox& boundingBox() const noexcept { return _boundingBox; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _capacity;                      //!< Region capacity (rectangles).
  CommonData _commonData;                //!< Common data.
  size_t _size;                          //!< Region size (count of rectangles in region).

  IntBox _boundingBox;                   //!< Bounding box.
  IntBox _data[1];                       //!< Rectangles array.
};

// ============================================================================
// [b2d::Region]
// ============================================================================

//! Region defined by set of YX sorted rectangle(s).
class Region : public AnyBase {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdRegion };

  // Region limits.
  enum Limits : size_t {
    kInitSize = 6,
    kMaxSize = (std::numeric_limits<size_t>::max() - (sizeof(RegionImpl) - sizeof(IntBox))) / sizeof(IntBox)
  };

  //! Region type.
  enum Type : uint32_t {
    kTypeNone         = 0,               //!< Region has no rectangles.
    kTypeRect         = 1,               //!< Region has only one rectangle (rectangular).
    kTypeComplex      = 2,               //!< Region has more YX sorted rectangles.
    kTypeInfinite     = 3                //!< Region is infinite (special region instance).
  };

  //! Region hit-test status.
  enum HitTest : uint32_t {
    kHitTestOut       = 0,               //!< Object isn't in region.
    kHitTestIn        = 1,               //!< Object is in region.
    kHitTestPartial   = 2                //!< Object is partially in region.
  };

  //! Region operator.
  enum Op : uint32_t {
    kOpReplace        = 0,               //!< Replacement (B).
    kOpIntersect      = 1,               //!< Intersection (A & B).
    kOpUnion          = 2,               //!< Union (A | B).
    kOpXor            = 3,               //!< Xor (A ^ B).
    kOpSubtract       = 4,               //!< Subtraction (A - B).
    kOpSubRev         = 5,               //!< Reversed subtraction (B - A).

    kOpCount          = 6                //!< Count of region operators.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Region() noexcept
    : AnyBase(none().impl()) {}

  B2D_INLINE Region(const Region& other) noexcept
    : AnyBase(other.impl()->addRef()) {}

  B2D_INLINE Region(Region&& other) noexcept
    : AnyBase(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Region(RegionImpl* impl) noexcept
    : AnyBase(impl) {}

  B2D_INLINE explicit Region(const Support::Temporary& temporary) noexcept
    : AnyBase(RegionImpl::newTemporary(temporary)) {}

  B2D_INLINE ~Region() noexcept { AnyInternal::releaseAnyImpl(impl()); }

  //! Get a built-in none `Region`.
  static B2D_INLINE const Region& none() noexcept { return Any::noneOf<Region>(); }
  //! Get a built-in infinite `Region`.
  static B2D_INLINE const Region& infinite() noexcept { return Any::noneByTypeId<Region>(Any::kTypeIdRegionInfiniteSlot); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = RegionImpl>
  B2D_INLINE T* impl() const noexcept { return AnyBase::impl<T>(); }

  //! \internal
  B2D_API Error _detach() noexcept;
  //! \copydoc Doxygen::Implicit::detach().
  B2D_INLINE Error detach() noexcept { return isShared() ? _detach() : Error(kErrorOk); }

  // --------------------------------------------------------------------------
  // [Container]
  // --------------------------------------------------------------------------

  //! Get region size.
  B2D_INLINE size_t size() const noexcept { return impl()->size(); }
  //! Get region capacity.
  B2D_INLINE size_t capacity() const noexcept { return impl()->capacity(); }

  //! Reserve at least `n` boxes in region and detach it.
  //!
  //! NOTE: It's not possible to detach an infinite region.
  B2D_API Error reserve(size_t capacity) noexcept;
  //! Prepare `size` of boxes in region and clear it.
  B2D_API Error prepare(size_t size) noexcept;
  //! Compact the region.
  B2D_API Error compact() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  //! Get const pointer to the region data.
  B2D_INLINE const IntBox* data() const noexcept { return impl()->data(); }
  //! Get region bounding box.
  B2D_INLINE const IntBox& boundingBox() const noexcept { return impl()->boundingBox(); }

  //! Get type of the region, see `Region::Type`.
  B2D_INLINE uint32_t type() const noexcept {
    const RegionImpl* thisI = impl();
    uint32_t type = uint32_t(Math::min<size_t>(thisI->size(), 2));
    return !thisI->isInfinite() ? type : kTypeInfinite;
  }

  //! Get whether the region is empty (zero size and not infinite).
  B2D_INLINE bool empty() const noexcept { return size() == 0; }
  //! Get whether the region is one rectangle.
  B2D_INLINE bool isRect() const noexcept { return size() == 1; }
  //! Get whether the region is complex.
  B2D_INLINE bool isComplex() const noexcept { return size() > 1; }
  //! Get whether the region is infinite.
  B2D_INLINE bool isInfinite() const noexcept { return impl()->isInfinite(); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const Region& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }

  B2D_API Error setRegion(const Region& region) noexcept;
  B2D_API Error setDeep(const Region& region) noexcept;

  B2D_API Error setBox(const IntBox& box) noexcept;
  B2D_API Error setBoxList(const IntBox* data, size_t size) noexcept;

  B2D_API Error setRect(const IntRect& rect) noexcept;
  B2D_API Error setRectList(const IntRect* data, size_t size) noexcept;

  // --------------------------------------------------------------------------
  // [Combine]
  // --------------------------------------------------------------------------

  static B2D_API Error combine(Region& dst, const Region&  a, const Region&  b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const Region&  a, const IntBox&  b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const Region&  a, const IntRect& b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const IntBox&  a, const Region&  b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const IntRect& a, const Region&  b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const IntBox&  a, const IntBox&  b, uint32_t op) noexcept;
  static B2D_API Error combine(Region& dst, const IntRect& a, const IntRect& b, uint32_t op) noexcept;

  B2D_INLINE Error combine(const Region& region, uint32_t op) noexcept { return combine(*this, *this, region, op); }
  B2D_INLINE Error combine(const IntBox& box, uint32_t op) noexcept { return combine(*this, *this, box, op); }
  B2D_INLINE Error combine(const IntRect& rect, uint32_t op) noexcept { return combine(*this, *this, rect, op); }

  static B2D_INLINE Error union_(Region& dst, const Region&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const Region&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const Region&  a, const IntRect& b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const IntBox&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const IntRect& a, const Region&  b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const IntBox&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpUnion); }
  static B2D_INLINE Error union_(Region& dst, const IntRect& a, const IntRect& b) noexcept { return combine(dst, a, b, kOpUnion); }

  B2D_INLINE Error union_(const Region& region) noexcept { return combine(*this, *this, region, kOpUnion); }
  B2D_INLINE Error union_(const IntBox& box) noexcept { return combine(*this, *this, box, kOpUnion); }
  B2D_INLINE Error union_(const IntRect& rect) noexcept { return combine(*this, *this, rect, kOpUnion); }

  static B2D_INLINE Error intersect(Region& dst, const Region&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const Region&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const Region&  a, const IntRect& b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const IntBox&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const IntRect& a, const Region&  b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const IntBox&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpIntersect); }
  static B2D_INLINE Error intersect(Region& dst, const IntRect& a, const IntRect& b) noexcept { return combine(dst, a, b, kOpIntersect); }

  B2D_INLINE Error intersect(const Region& region) noexcept { return combine(*this, *this, region, kOpIntersect); }
  B2D_INLINE Error intersect(const IntBox& box) noexcept { return combine(*this, *this, box, kOpIntersect); }
  B2D_INLINE Error intersect(const IntRect& rect) noexcept { return combine(*this, *this, rect, kOpIntersect); }

  static B2D_INLINE Error xor_(Region& dst, const Region&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const Region&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const Region&  a, const IntRect& b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const IntBox&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const IntRect& a, const Region&  b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const IntBox&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpXor); }
  static B2D_INLINE Error xor_(Region& dst, const IntRect& a, const IntRect& b) noexcept { return combine(dst, a, b, kOpXor); }

  B2D_INLINE Error xor_(const Region& region) noexcept { return combine(*this, *this, region, kOpXor); }
  B2D_INLINE Error xor_(const IntBox& box) noexcept { return combine(*this, *this, box, kOpXor); }
  B2D_INLINE Error xor_(const IntRect& rect) noexcept { return combine(*this, *this, rect, kOpXor); }

  static B2D_INLINE Error subtract(Region& dst, const Region&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const Region&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const Region&  a, const IntRect& b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const IntBox&  a, const Region&  b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const IntRect& a, const Region&  b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const IntBox&  a, const IntBox&  b) noexcept { return combine(dst, a, b, kOpSubtract); }
  static B2D_INLINE Error subtract(Region& dst, const IntRect& a, const IntRect& b) noexcept { return combine(dst, a, b, kOpSubtract); }

  B2D_INLINE Error subtract(const Region& region) noexcept { return combine(*this, *this, region, kOpSubtract); }
  B2D_INLINE Error subtract(const IntBox& box) noexcept { return combine(*this, *this, box, kOpSubtract); }
  B2D_INLINE Error subtract(const IntRect& rect) noexcept { return combine(*this, *this, rect, kOpSubtract); }

  B2D_INLINE Error subtractReverse(const Region& region) noexcept { return combine(*this, *this, region, kOpSubRev); }
  B2D_INLINE Error subtractReverse(const IntBox& box) noexcept { return combine(*this, *this, box, kOpSubRev); }
  B2D_INLINE Error subtractReverse(const IntRect& rect) noexcept { return combine(*this, *this, rect, kOpSubRev); }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  //! Translate region by @a pt.
  //!
  //! @note If region is infinite then translation is NOP; if translation point
  //! makes region out of bounds it would be clipped.
  B2D_INLINE Error translate(const IntPoint& pt) noexcept { return translate(*this, *this, pt); }
  //! \overload
  B2D_INLINE Error translate(int x, int y) noexcept { return translate(*this, *this, IntPoint(x, y)); }

  static B2D_INLINE Error translate(Region& dst, const Region& src, const IntPoint& pt) noexcept {
    return _bRegionOps.translate(dst, src, pt);
  }

  // --------------------------------------------------------------------------
  // [TranslateClip]
  // --------------------------------------------------------------------------

  static B2D_API Error translateClip(Region& dst, const Region& src, const IntPoint& pt, const IntBox& clipBox) noexcept;

  //! Translate current region by @a pt and clip it into @a clipBox.
  //!
  //! This is special function designed for windowing systems where the region
  //! used by UI component needs to be translated and clipped to the view-box.
  //! The result of the operation is the same as translating the region by
  //! @a pt and then intersecting it with @a clipBox. The advantage of using
  //! this method is increased performance.
  B2D_INLINE Error translateClip(const IntPoint& pt, const IntBox& clipBox) noexcept {
    return translateClip(*this, *this, pt, clipBox);
  }

  // --------------------------------------------------------------------------
  // [IntersectClip]
  // --------------------------------------------------------------------------

  static B2D_API Error intersectClip(Region& dst, const Region& a, const Region& b, const IntBox& clipBox) noexcept;

  //! Intersect current region with the @a region and clip it into @a clipBox.
  //!
  //! This is a special function designed for windowing systems. The result of
  //! this operation is the same as intersecting the current region with the
  //! @a region and then intersecting it again with the @a clipBox. The
  //! advantage of using this method is increased performance.
  B2D_INLINE Error intersectClip(const Region& region, const IntBox& clipBox) noexcept {
    return intersectClip(*this, *this, region, clipBox);
  }

  // --------------------------------------------------------------------------
  // [HitTest]
  // --------------------------------------------------------------------------

  //! Tests if a given point is in region, see \ref HitTest.
  B2D_API uint32_t hitTest(const IntPoint& p) const noexcept;
  //! \overload
  B2D_INLINE uint32_t hitTest(int x, int y) const noexcept { return hitTest(IntPoint(x, y)); }

  //! \overload
  B2D_API uint32_t hitTest(const IntBox& box) const noexcept;
  //! \overload
  B2D_API uint32_t hitTest(const IntRect& rect) const noexcept;

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  B2D_INLINE bool eq(const Region& other) const noexcept { return _bRegionOps.eq(this, &other); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Region& operator=(const Region& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Region& operator=(Region&& other) noexcept {
    RegionImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const Region& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Region& other) const noexcept { return !eq(other); }
};

// ============================================================================
// [b2d::RegionTmp<>]
// ============================================================================

//! Temporary \ref Region.
template<size_t N = (Globals::kMemTmpSize - sizeof(RegionImpl) - sizeof(IntBox)) / sizeof(IntBox)>
class RegionTmp : public Region {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE RegionTmp() noexcept
    : Region(Temporary(&_storage, N)) {}

  B2D_INLINE RegionTmp(const RegionTmp<N>& other) noexcept
    : Region(Temporary(&_storage, N)) { setDeep(other); }

  B2D_INLINE RegionTmp(const Region& other) noexcept
    : Region(Temporary(&_storage, N)) { setDeep(other); }

  B2D_INLINE explicit RegionTmp(const IntBox& box) noexcept
    : Region(Temporary(&_storage, N)) { setBox(box); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE RegionTmp& operator=(const RegionTmp<N>& r) noexcept { setRegion(r); return *this; }
  B2D_INLINE RegionTmp& operator=(const Region& r) noexcept { setRegion(r); return *this; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  struct _Storage {
    RegionImpl d;
    IntBox data[N > 1 ? N - 1 : 1];
  } _storage;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_REGION_H
