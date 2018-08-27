// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math.h"
#include "../core/region.h"
#include "../core/regioninternal_p.h"
#include "../core/runtime.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Region - Ops]
// ============================================================================

RegionOps _bRegionOps;

// ============================================================================
// [b2d::Region - Impl]
// ============================================================================

static const constexpr RegionImpl RegionImplNone(
  Any::kTypeIdRegion | Any::kFlagIsNone, 0,
  0, 0, 0, 0
);

static const constexpr RegionImpl RegionImplInfinite(
  Any::kTypeIdRegion | Any::kFlagIsInfinite, 1,
  std::numeric_limits<int32_t>::min(),
  std::numeric_limits<int32_t>::min(),
  std::numeric_limits<int32_t>::max(),
  std::numeric_limits<int32_t>::max()
);

// ============================================================================
// [b2d::Region - Data - IsValid]
// ============================================================================

// Validate RegionImpl:
//
//   - If region is empty, its bounding box must be [0, 0, 0, 0]
//   - If region is rectangle, then the data and bounding box must match and
//     can't be invalid.
//   - If region is complex, validate all bands and check whether all bands
//     were coalesced properly.
static bool bRegionIsDataValid(const RegionImpl* impl) noexcept {
  B2D_ASSERT(impl->capacity() >= impl->size());
  size_t size = impl->size();

  const IntBox* data = impl->data();
  const IntBox& bbox = impl->boundingBox();

  if (size == 0) {
    if ((bbox.x0() | bbox.y0() | bbox.x1() | bbox.y1()) != 0)
      return false;
    else
      return true;
  }

  if (size == 1) {
    if (bbox != data[0] || !bbox.isValid())
      return false;
    else
      return true;
  }

  const IntBox* prevBand = nullptr;
  const IntBox* prevEnd = nullptr;

  const IntBox* curBand = data;
  const IntBox* curEnd;

  const IntBox* end = data + size;

  // Validated bounding box.
  IntBox vbox = data[0];
  vbox._y1 = data[size - 1].y1();

  do {
    curEnd = bRegionGetEndBand(curBand, end);

    // Validate the current band.
    {
      const IntBox* cur = curBand;
      if (cur[0].x0() >= cur[0].x1())
        return false;

      while (++cur != curEnd) {
        // Validate Y.
        if (cur[-1].y0() != cur[0].y0())
          return false;
        if (cur[-1].y1() != cur[0].y1())
          return false;

        // Validate X.
        if (cur[0].x0() >= cur[0].x1())
          return false;
        if (cur[0].x0() < cur[-1].x1())
          return false;
      }
    }

    if (prevBand && curBand->y0() < prevBand->y1())
      return false;

    // Validate whether the current band and the previous one are properly coalesced.
    if (prevBand && curBand->y0() == prevBand->y1() && (size_t)(prevEnd - prevBand) == (size_t)(curEnd - curBand)) {
      size_t i = 0;
      size_t bandSize = (size_t)(prevEnd - prevBand);

      do {
        if (prevBand[i].x0() != curBand[i].x0() || prevBand[i].x1() != curBand[i].x1())
          break;
      } while (++i < bandSize);

      // Improperly coalesced.
      if (i == bandSize)
        return false;
    }

    vbox._x0 = Math::min<int>(vbox.x0(), curBand[0].x0());
    vbox._x1 = Math::max<int>(vbox.x1(), curEnd[-1].x1());

    prevBand = curBand;
    prevEnd = curEnd;

    curBand = curEnd;
  } while (curBand != end);

  // Finally validate the bounding box.
  if (bbox != vbox)
    return false;

  return true;
}

// ============================================================================
// [b2d::Region - Data - Copy]
// ============================================================================

static B2D_INLINE void bRegionCopyData(IntBox* dst, const IntBox* src, size_t size) noexcept {
  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
}

static void bRegionCopyDataEx(IntBox* dst, const IntBox* src, size_t size, IntBox* bbox) noexcept {
  int minX = src[0].x0();
  int maxX = src[size - 1].x1();

  for (size_t i = 0; i < size; i++) {
    minX = Math::min(minX, src[i].x0());
    maxX = Math::max(maxX, src[i].x1());
    dst[i] = src[i];
  }

  bbox->reset(minX, src[0].y0(), maxX, src[size - 1].y1());
}

static void bRegionCopyDataEx(IntBox* dst, const IntRect* src, size_t size, IntBox* bbox) noexcept {
  int minX = src[0].x0();
  int maxX = src[size - 1].x1();

  for (size_t i = 0; i < size; i++) {
    int x0 = src[i].x0();
    int x1 = src[i].x1();
    minX = Math::min(minX, x0);
    maxX = Math::max(maxX, x1);
    dst[i].reset(x0, src[i].y0(), x1, src[i].y1());
  }

  bbox->reset(minX, src[0].y0(), maxX, src[size - 1].y1());
}

// ============================================================================
// [b2d::Region - Data - Alloc]
// ============================================================================

// Called also from region_sse2.cpp.
RegionImpl* bRegionAllocData(size_t capacity) noexcept {
  return capacity ? RegionImpl::new_(capacity)
                  : const_cast<RegionImpl*>(&RegionImplNone);
}

static RegionImpl* bRegionAllocDataBox(size_t capacity, const IntBox& box) noexcept {
  RegionImpl* impl = bRegionAllocData(capacity);

  if (B2D_UNLIKELY(!impl))
    return nullptr;

  impl->_size = 1;
  impl->_boundingBox = box;
  impl->_data[0] = box;

  return impl;
}

static RegionImpl* bRegionAllocData(size_t capacity, const IntBox* data, size_t size, const IntBox* bbox) noexcept {
  if (B2D_UNLIKELY(capacity < size))
    capacity = size;

  RegionImpl* impl = bRegionAllocData(capacity);
  if (B2D_UNLIKELY(!impl))
    return nullptr;

  impl->_size = size;
  if (B2D_UNLIKELY(!size)) {
    impl->_boundingBox.reset();
    impl->_data[0].reset();
    return impl;
  }

  if (B2D_UNLIKELY(!bbox)) {
    bRegionCopyDataEx(impl->data(), data, size, &impl->_boundingBox);
    return impl;
  }

  impl->_boundingBox = *bbox;
  bRegionCopyData(impl->data(), data, size);
  return impl;
}

// ============================================================================
// [b2d::Region - Data - Realloc]
// ============================================================================

static RegionImpl* _bRegionDataRealloc(RegionImpl* impl, size_t capacity) noexcept {
  RegionImpl* newI = bRegionAllocData(capacity, impl->data(), impl->size(), &impl->_boundingBox);

  if (B2D_UNLIKELY(!newI))
    return nullptr;

  impl->release();
  return newI;
}

// ============================================================================
// [b2d::Region - Sharing]
// ============================================================================

Error Region::_detach() noexcept {
  if (!isShared())
    return kErrorOk;

  RegionImpl* thisI = impl();
  if (thisI->size() > 0) {
    RegionImpl* newI = bRegionAllocData(thisI->size(), thisI->data(), thisI->size(), &thisI->_boundingBox);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);
    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    RegionImpl* newI = bRegionAllocData(thisI->capacity());
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);
    newI->_boundingBox.reset();
    return AnyInternal::replaceImpl(this, newI);
  }
}

// ============================================================================
// [b2d::Region - Container]
// ============================================================================

Error Region::reserve(size_t capacity) noexcept {
  if (isShared()) {
    RegionImpl* newI = bRegionAllocData(capacity, data(), size(), &impl()->_boundingBox);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    if (capacity > impl()->capacity()) {
      RegionImpl* newI = _bRegionDataRealloc(impl(), capacity);
      if (B2D_UNLIKELY(!newI))
        return DebugUtils::errored(kErrorNoMemory);

      _impl = newI;
    }
    return kErrorOk;
  }
}

Error Region::prepare(size_t size) noexcept {
  if (isShared() || size > capacity()) {
    RegionImpl* newI = bRegionAllocData(size);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    impl()->_size = 0;
    impl()->_boundingBox.reset();
    return kErrorOk;
  }
}

Error Region::compact() noexcept {
  RegionImpl* thisI = impl();
  size_t size = thisI->size();

  if (thisI->isTemporary() || size == thisI->capacity())
    return kErrorOk;

  if (thisI->isShared()) {
    RegionImpl* newI = bRegionAllocData(size);

    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    newI->_size = size;
    newI->_boundingBox = thisI->boundingBox();
    bRegionCopyData(newI->data(), thisI->data(), size);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    RegionImpl* newI = _bRegionDataRealloc(thisI, size);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    _impl = newI;
    return kErrorOk;
  }
}

// ============================================================================
// [b2d::Region - Clear / Reset]
// ============================================================================

Error Region::reset(uint32_t resetPolicy) noexcept {
  RegionImpl* thisI = impl();
  if (thisI->isShared() || resetPolicy == Globals::kResetHard)
    return AnyInternal::replaceImpl(this, none().impl());

  thisI->_size = 0;
  thisI->_boundingBox.reset();
  return kErrorOk;
}

// ============================================================================
// [b2d::Region - Set]
// ============================================================================

Error Region::setRegion(const Region& other) noexcept {
  return AnyInternal::replaceImpl(this, other.impl()->addRef());
}

Error Region::setDeep(const Region& other) noexcept {
  RegionImpl* thisI = this->impl();
  RegionImpl* otherI = other.impl();

  if (thisI->isInfinite())
    return AnyInternal::replaceImpl(this, otherI->addRef());

  if (thisI == otherI)
    return detach();

  size_t size = otherI->size();
  B2D_PROPAGATE(prepare(size));

  thisI = impl();
  thisI->_size = size;
  thisI->_boundingBox = otherI->boundingBox();

  bRegionCopyData(thisI->_data, otherI->_data, size);
  return kErrorOk;
}

Error Region::setBox(const IntBox& box) noexcept {
  if (B2D_UNLIKELY(!box.isValid()))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(prepare(1));
  RegionImpl* thisI = impl();

  thisI->_size = 1;
  thisI->_boundingBox = box;
  thisI->_data[0] = box;
  return kErrorOk;
}

Error Region::setBoxList(const IntBox* data, size_t size) noexcept {
  if (bRegionIsBoxListSorted(data, size)) {
    B2D_PROPAGATE(prepare(size));
    RegionImpl* thisI = impl();

    thisI->_size = size;
    bRegionCopyDataEx(thisI->data(), data, size, &thisI->_boundingBox);
    return kErrorOk;
  }
  else {
    // TODO: Region::setBoxList() - Not optimal.
    B2D_PROPAGATE(prepare(size));
    for (size_t i = 0; i < size; i++)
      union_(data[i]);
    return kErrorOk;
  }
}

Error Region::setRect(const IntRect& rect) noexcept {
  if (B2D_UNLIKELY(!rect.isValid()))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(prepare(1));
  RegionImpl* thisI = impl();

  thisI->_size = 1;
  thisI->_boundingBox = rect;
  thisI->_data[0] = thisI->boundingBox();
  return kErrorOk;
}

Error Region::setRectList(const IntRect* data, size_t size) noexcept {
  if (bRegionIsRectListSorted(data, size)) {
    B2D_PROPAGATE(prepare(size));
    RegionImpl* thisI = impl();

    thisI->_size = size;
    bRegionCopyDataEx(thisI->data(), data, size, &thisI->_boundingBox);
    return kErrorOk;
  }
  else {
    // TODO: Region::setRectList() - Not optimal.
    B2D_PROPAGATE(prepare(size));
    for (size_t i = 0; i < size; i++)
      union_(data[i]);
    return kErrorOk;
  }
}

// ============================================================================
// [b2d::Region - HitTest]
// ============================================================================

uint32_t Region::hitTest(const IntPoint& p) const noexcept {
  const RegionImpl* thisI = impl();

  int x = p.x();
  int y = p.y();

  // Check whether the point is in the region bounding-box.
  if (!thisI->boundingBox().contains(x, y))
    return kHitTestOut;

  // If the region is a rectangle, then we are finished.
  size_t size = thisI->size();
  if (size == 1)
    return kHitTestIn;

  // If the region is small enough then do a naive search.
  const IntBox* data = thisI->data();
  const IntBox* end = data + size;

  // Find the first interesting rectangle.
  data = bRegionGetClosestBox(data, size, y);
  if (data == nullptr)
    return kHitTestOut;

  B2D_ASSERT(data != end);
  if (y < data->y0())
    return kHitTestOut;

  do {
    if (x >= data->x0() && x < data->x1())
      return kHitTestIn;
    data++;
  } while (data != end && y >= data->y0());

  return kHitTestOut;
}

uint32_t Region::hitTest(const IntBox& box) const noexcept {
  const RegionImpl* thisI = impl();

  if (!box.isValid())
    return kHitTestOut;

  if (thisI->isInfinite())
    return kHitTestIn;

  size_t size = thisI->size();
  if (!size || !thisI->boundingBox().overlaps(box))
    return kHitTestOut;

  const IntBox* data = thisI->data();
  const IntBox* end = data + size;

  int bx0 = box.x0();
  int by0 = box.y0();
  int bx1 = box.x1();
  int by1 = box.y1();

  data = bRegionGetClosestBox(data, size, by0);
  if (data == nullptr)
    return kHitTestOut;

  // Now we have the start band. If the band.y0 position is greater than
  // the by1 then we know that a given box is completely outside of the
  // region.
  B2D_ASSERT(data != end);
  if (by1 <= data->y0())
    return kHitTestOut;

  // There are two main loops, each is optimized for one special case. The
  // first loop is used in case that we know that a given box can only partially
  // hit the region. The algorithm is optimized to find whether there is any
  // intersection (hit). The second loop is optimized to check whether the
  // given box completely covers the region.
  int bandY0 = data->y0();

  if (thisI->boundingBox().x0() > bx0 || bandY0 > by0 ||
      thisI->boundingBox().x1() < bx1 ||
      thisI->boundingBox().y1() < by1) {
    // Partially or completely outside.
    if (bandY0 >= by1)
      return kHitTestOut;

    for (;;) {
      // Skip leading boxes which do not intersect.
      while (data->x1() <= bx0) {
        if (++data == end)
          return kHitTestOut;
        if (data->y0() != bandY0)
          goto OnPartNextBand;
      }

      // If there is an intersection then we are done.
      if (data->x0() < bx1)
        return kHitTestPartial;

      // Skip remaining boxes.
      for (;;) {
        if (++data == end)
          return kHitTestOut;
        if (data->y0() != bandY0)
          break;
      }

OnPartNextBand:
      bandY0 = data->y0();
      if (bandY0 >= by1)
        return kHitTestOut;
    }
  }
  else {
    // Partially or completely inside.
    uint32_t result = kHitTestOut;

    for (;;) {
      // Skip leading boxes which do not intersect.
      while (data->x1() <= bx0){
        if (++data == end)
          return result;
        if (data->y0() != bandY0)
          goto OnCompNoIntersection;
      }

      // The intersection (if any) must cover the entire bx0...bx1. If the
      // condition is not true or there is no intersection at all, then we
      // jump into the first loop, because we know that the given box doesn't
      // cover the entire region.
      if (data->x0() > bx0 || data->x1() < bx1) {
OnCompNoIntersection:
        if (result != kHitTestOut)
          return result;
        else
          goto OnPartNextBand;
      }

      // From now we know that the box is partially in the region, but we are
      // not sure whether the hit test is only partial or complete.
      result = kHitTestPartial;

      // Skip remaining boxes.
      for (;;) {
        if (++data == end)
          return result;
        if (data->y0() != bandY0)
          break;
      }

      // If this was the last interesting band, then we can safely return.
      int previousY1 = data[-1].y1();
      if (previousY1 >= by1)
        return kHitTestIn;

      bandY0 = data->y0();
      if (previousY1 != bandY0 || bandY0 >= by1)
        return kHitTestPartial;
    }
  }
}

uint32_t Region::hitTest(const IntRect& rect) const noexcept {
  IntBox box(rect);
  return box.isValid() ? hitTest(box) : kHitTestOut;
}

// ============================================================================
// [b2d::Region - Coalesce]
// ============================================================================

static B2D_INLINE IntBox* _bRegionCoalesce(IntBox* p, IntBox** _prevBand, IntBox** _curBand, int y1) noexcept {
  IntBox* prevBand = *_prevBand;
  IntBox* curBand  = *_curBand;

  if (prevBand->y1() == curBand->y0()) {
    size_t i = (size_t)(curBand - prevBand);
    size_t size = (size_t)(p - curBand);

    if (i == size) {
      B2D_ASSERT(size != 0);
      for (;;) {
        if ((prevBand[i].x0() != curBand[i].x0()) | (prevBand[i].x1() != curBand[i].x1()))
          break;

        if (++i == size) {
          for (i = 0; i < size; i++)
            prevBand[i]._y1 = y1;
          return curBand;
        }
      }
    }
  }

  *_prevBand = curBand;
  return p;
}

// ============================================================================
// [b2d::Region - JoinTo]
// ============================================================================

// Get whether it is possible to join box B after box A.
static B2D_INLINE bool _bRegionCanJoin(const IntBox& a, const IntBox& b) noexcept {
  return a.y0() == b.y0() &&
         a.y1() == b.y1() &&
         a.x1() <= b.x0() ;
}

// Join src with the destination.
static Error _bRegionJoinTo(Region& dst, const IntBox* sData, size_t sSize, const IntBox& sBoundingBox) noexcept {
  size_t dSize = dst.size();
  B2D_PROPAGATE(dst.reserve(dSize + sSize));

  RegionImpl* dstI = dst.impl();

  IntBox* pData = dstI->data();
  IntBox* p = pData + dSize;
  IntBox* prevBand = nullptr;

  const IntBox* s = sData;
  const IntBox* sEnd = sData + sSize;

  // --------------------------------------------------------------------------
  // [Join the Continuous Part]
  // --------------------------------------------------------------------------

  // First try to join the first rectangle in SRC with the last rectangle in
  // DST. This is very special case used by UNION. The image below should
  // clarify the situation:
  //
  // 1) The first source rectangle immediately follows the last destination one.
  // 2) The first source rectangle follows the last destination band.
  //
  //       1)            2)
  //
  //   ..........   ..........
  //   ..DDDDDDDD   ..DDDDDDDD   D - Destination rectangle(s)
  //   DDDDDDSSSS   DDDD   SSS
  //   SSSSSSSS..   SSSSSSSS..   S - Source rectangle(s)
  //   ..........   ..........
  if (p != pData && p[-1].y0() == s->y0()) {
    IntBox* pMark = p;
    int y0 = p[-1].y0();
    int y1 = p[-1].y1();

    // This special case require such condition.
    B2D_ASSERT(p[-1].y1() == s->y1());

    // Merge the last destination rectangle with the first source one? (Case 1).
    if (p[-1].x1() == s->x0()) {
      p[-1]._x1 = s->x1();
      s++;
    }

    // Append all other bands (Case 1, 2).
    while (s != sEnd && s->y0() == y0) {
      *p++ = *s++;
    }

    // Coalesce. If we know that boundingBox.y0() != y0 then it means that there
    // are at least two bands in the destination region. We added rectangles
    // to the last band, but this means that the last band can now be coalesced
    // with the previous one.
    //
    // Instead of finding two bands and then comparing them from left-to-right,
    // we find the end of bands and compare then right-to-left.
    if (dstI->boundingBox().y0() != y0) {
      IntBox* band1 = pMark - 1;
      IntBox* band2 = p - 1;

      while (band1->y0() == y0) {
        B2D_ASSERT(band1 != pData);
        band1--;
      }

      prevBand = band1 + 1;

      // The 'band1' points to the end of the previous band.
      // The 'band2' points to the end of the current band.
      B2D_ASSERT(band1->y0() != band2->y0());

      if (band1->y1() == band2->y0()) {
        while (band1->x0() == band2->x0() && band1->x1() == band2->x1()) {
          bool finished1 = (band1[-1].y0() != band1[0].y0()) || band1 == pData;
          bool finished2 = (band2[-1].y0() != band2[0].y0());

          if (finished1 & finished2)
            goto OnDoFirstCoalesce;
          else if (finished1 | finished2)
            goto OnSkipFirstCoalesce;

          band1--;
          band2--;
        }
      }
      goto OnSkipFirstCoalesce;

OnDoFirstCoalesce:
      prevBand = band1;
      p = band2;

      do {
        band1->_y1 = y1;
      } while (++band1 != band2);
    }
  }

  // --------------------------------------------------------------------------
  // [Join and Attempt to Coalesce the Second Band]
  // --------------------------------------------------------------------------

OnSkipFirstCoalesce:
  if (s == sEnd)
    goto OnEnd;

  if (p[-1].y1() == s->y0()) {
    if (prevBand == nullptr) {
      prevBand = &p[-1];
      while (prevBand != pData && prevBand[-1].y0() == prevBand[0].y0())
        prevBand--;
    }

    int y0 = s->y0();
    int y1 = s->y1();

    IntBox* band1 = prevBand;
    IntBox* band2 = p;
    IntBox* mark = p;

    do {
      *p++ = *s++;
    } while (s != sEnd && s->y0() == y0);

    // Now check whether the bands have to be coalesced.
    while (band1 != mark && band1->x0() == band2->x0() && band1->x1() == band2->x1()) {
      band1++;
      band2++;
    }

    if (band1 == mark) {
      for (band1 = prevBand; band1 != mark; band1++)
        band1->_y1 = y1;
      p = mark;
    }
  }

  // --------------------------------------------------------------------------
  // [Join the Rest]
  // --------------------------------------------------------------------------

  // We do not need coalese from here, because the bands we process from the
  // source region should be already coalesced.
  while (s != sEnd)
    *p++ = *s++;

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

OnEnd:
  dstI->_size = (size_t)(p - dstI->data());
  IntBox::bound(dstI->_boundingBox, dstI->_boundingBox, sBoundingBox);

  B2D_ASSERT(bRegionIsDataValid(dstI));
  return kErrorOk;
}

// ============================================================================
// [b2d::Region - Combine - JoinAB]
// ============================================================================

static Error _bRegionCombineJoinAB(Region& dst,
  const IntBox* a, size_t aSize, const IntBox& aBoundingBox,
  const IntBox* b, size_t bSize, const IntBox& bBoundingBox) noexcept {

  size_t zSize = aSize + bSize;

  if (zSize < aSize || zSize < bSize)
    return DebugUtils::errored(kErrorNoMemory);

  B2D_PROPAGATE(dst.prepare(zSize));
  RegionImpl* dstI = dst.impl();

  dstI->_size = aSize;
  dstI->_boundingBox = aBoundingBox;

  IntBox* data = dstI->data();
  for (size_t i = 0; i < aSize; i++)
    data[i] = a[i];

  return _bRegionJoinTo(dst, b, bSize, bBoundingBox);
}

// ============================================================================
// [b2d::Region - Combine - Clip]
// ============================================================================

static Error _bRegionCombineClip(Region& dst, const Region& src, const IntBox& clipBox) noexcept {
  RegionImpl* dstI = dst.impl();
  RegionImpl* srcI = src.impl();

  size_t size = srcI->_size;
  B2D_ASSERT(size > 0);

  if (dstI->isShared() || size > dstI->capacity()) {
    dstI = bRegionAllocData(size);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }

  B2D_ASSERT(dstI->capacity() >= size);

  IntBox* dstDataPtr = dstI->data();
  #ifdef B2D_BUILD_DEBUG
  IntBox* dstDataEnd = dstI->data() + size;
  #endif

  IntBox* dstBandPrev = nullptr;
  IntBox* dstBandPtr;

  IntBox* srcDataPtr = srcI->data();
  IntBox* srcDataEnd = srcDataPtr + size;

  int cx0 = clipBox.x0();
  int cy0 = clipBox.y0();
  int cx1 = clipBox.x1();
  int cy1 = clipBox.y1();

  int dstBBoxX0 = std::numeric_limits<int>::max();
  int dstBBoxX1 = std::numeric_limits<int>::min();

  // Skip boxes which do not intersect with the clip-box.
  while (srcDataPtr->y1() <= cy0) {
    if (++srcDataPtr == srcDataEnd)
      goto OnEnd;
  }

  // Do the intersection part.
  for (;;) {
    B2D_ASSERT(srcDataPtr != srcDataEnd);

    int bandY0 = srcDataPtr->y0();
    if (bandY0 >= cy1)
      break;

    int y0;
    int y1 = 0; // Be quiet.

    // Skip leading boxes which do not intersect with the clip-box.
    while (srcDataPtr->x1() <= cx0) {
      if (++srcDataPtr == srcDataEnd) goto OnEnd;
      if (srcDataPtr->y0() != bandY0) goto OnSkip;
    }

    dstBandPtr = dstDataPtr;

    // Do the inner part.
    if (srcDataPtr->x0() < cx1) {
      y0 = Math::max(srcDataPtr->y0(), cy0);
      y1 = Math::min(srcDataPtr->y1(), cy1);

      // First box.
      #ifdef B2D_BUILD_DEBUG
      B2D_ASSERT(dstDataPtr != dstDataEnd);
      #endif

      dstDataPtr->reset(Math::max(srcDataPtr->x0(), cx0), y0, Math::min(srcDataPtr->x1(), cx1), y1);
      dstDataPtr++;

      if (++srcDataPtr == srcDataEnd || srcDataPtr->y0() != bandY0)
        goto OnMerge;

      // Inner boxes.
      while (srcDataPtr->x1() <= cx1) {
        #ifdef B2D_BUILD_DEBUG
        B2D_ASSERT(dstDataPtr != dstDataEnd);
        #endif
        B2D_ASSERT(srcDataPtr->x0() >= cx0 && srcDataPtr->x1() <= cx1);

        dstDataPtr->reset(srcDataPtr->x0(), y0, srcDataPtr->x1(), y1);
        dstDataPtr++;

        if (++srcDataPtr == srcDataEnd || srcDataPtr->y0() != bandY0)
          goto OnMerge;
      }

      // Last box.
      if (srcDataPtr->x0() < cx1) {
        #ifdef B2D_BUILD_DEBUG
        B2D_ASSERT(dstDataPtr != dstDataEnd);
        #endif
        B2D_ASSERT(srcDataPtr->x0() >= cx0);

        dstDataPtr->reset(srcDataPtr->x0(), y0, Math::min(srcDataPtr->x1(), cx1), y1);
        dstDataPtr++;

        if (++srcDataPtr == srcDataEnd || srcDataPtr->y0() != bandY0)
          goto OnMerge;
      }

      B2D_ASSERT(srcDataPtr->x0() >= cx1);
    }

    // Skip trailing boxes which do not intersect with the clip-box.
    while (srcDataPtr->x0() >= cx1) {
      if (++srcDataPtr == srcDataEnd || srcDataPtr->y0() != bandY0)
        break;
    }

OnMerge:
    if (dstBandPtr != dstDataPtr) {
      if (dstBBoxX0 > dstBandPtr[0].x0()) dstBBoxX0 = dstBandPtr[0].x0();
      if (dstBBoxX1 < dstDataPtr   [-1].x1()) dstBBoxX1 = dstDataPtr   [-1].x1();

      if (dstBandPrev)
        dstDataPtr = _bRegionCoalesce(dstDataPtr, &dstBandPrev, &dstBandPtr, y1);
      else
        dstBandPrev = dstBandPtr;
    }

OnSkip:
    if (srcDataPtr == srcDataEnd)
      break;
  }

OnEnd:
  size = (size_t)(dstDataPtr - dstI->_data);
  dstI->_size = size;

  if (size == 0)
    dstI->_boundingBox.reset();
  else
    dstI->_boundingBox.reset(dstBBoxX0, dstI->_data[0].y0(), dstBBoxX1, dstI->_data[size - 1].y1());
  B2D_ASSERT(bRegionIsDataValid(dstI));

  return dstI != dst.impl() ? AnyInternal::replaceImpl(&dst, dstI) : kErrorOk;
}

// ============================================================================
// [b2d::Region - Combine - AB]
// ============================================================================

static Error B2D_CDECL Region_combineAB(
  Region& dst,
  const IntBox* aCur, size_t aSize, const IntBox& aBoundingBox,
  const IntBox* bCur, size_t bSize, const IntBox& bBoundingBox,
  uint32_t op,
  bool memOverlap) noexcept {

  // We don't handle these operators here.
  B2D_ASSERT(op != Region::kOpReplace);
  B2D_ASSERT(op != Region::kOpSubRev);
  B2D_ASSERT(op <  Region::kOpCount);

  B2D_ASSERT(aSize > 0);
  B2D_ASSERT(bSize > 0);

  // --------------------------------------------------------------------------
  // [Estimated Size]
  // --------------------------------------------------------------------------

  // The resulting count of rectangles after (A && B) can't be larger than
  // (A + B) * 2. For other operators this value is only hint (the resulting
  // count of rectangles can be greater than the estimated value when run into
  // special cases). To prevent checking if there is space for a new rectangle
  // in the output buffer, use the B2D_REGION_ENSURE_SPACE() macro. It should
  // be called for each band.
  size_t estimatedSize = 8 + (aSize + bSize) * 2;

  if (estimatedSize < aSize || estimatedSize < bSize)
    return DebugUtils::errored(kErrorNoMemory);

  // --------------------------------------------------------------------------
  // [Destination]
  // --------------------------------------------------------------------------

  RegionImpl* dstI = dst.impl();
  if (dstI->isShared() || estimatedSize > dstI->capacity() || memOverlap) {
    dstI = bRegionAllocData(estimatedSize);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }

  size_t dCapacity = dstI->capacity();
  IntBox* dCur = dstI->data();
  IntBox* dEnd = dstI->data() + dCapacity;

  IntBox* dPrevBand = nullptr;
  IntBox* dCurBand = nullptr;

  int dstBBoxX0 = std::numeric_limits<int>::max();
  int dstBBoxX1 = std::numeric_limits<int>::min();

#define B2D_REGION_ENSURE_SPACE(SPACE, IS_FINAL)                 \
  B2D_MACRO_BEGIN                                                \
    size_t remain = (size_t)(dEnd - dCur);                       \
    size_t needed = (size_t)(SPACE);                             \
                                                                 \
    if (B2D_UNLIKELY(remain < needed)) {                         \
      size_t currentSize = (size_t)(dCur - dstI->data());        \
      size_t prevBandIndex = (size_t)(dPrevBand - dstI->data()); \
                                                                 \
      dstI->_size = currentSize;                                 \
                                                                 \
      size_t newCapacity = currentSize + needed;                 \
      if (!IS_FINAL)                                             \
        newCapacity = Math::max<size_t>(64, newCapacity * 2);     \
                                                                 \
      RegionImpl* newI = _bRegionDataRealloc(dstI, newCapacity); \
      if (B2D_UNLIKELY(!newI))                                   \
        goto OnOutOfMemory;                                      \
                                                                 \
      dstI = newI;                                               \
      dCapacity = dstI->capacity();                              \
      dCur = dstI->data() + currentSize;                         \
      dEnd = dstI->data() + dCapacity;                           \
                                                                 \
      if (dPrevBand)                                             \
        dPrevBand = dstI->data() + prevBandIndex;                \
    }                                                            \
  B2D_MACRO_END

  // --------------------------------------------------------------------------
  // [Source - A & B]
  // --------------------------------------------------------------------------

  const IntBox* aEnd = aCur + aSize;
  const IntBox* bEnd = bCur + bSize;

  const IntBox* aBandEnd = nullptr;
  const IntBox* bBandEnd = nullptr;

  int y0, y1;

  switch (op) {
    // ------------------------------------------------------------------------
    // [Intersect]
    // ------------------------------------------------------------------------

    case Region::kOpIntersect: {
      int yStop = Math::min(aBoundingBox.y1(), bBoundingBox.y1());

      // Skip all parts which do not intersect. If there is no intersection
      // detected then this loop can skip into the `OnClear` label.
      for (;;) {
        if (aCur->y1() <= bCur->y0()) { if (++aCur == aEnd) goto OnClear; else continue; }
        if (bCur->y1() <= aCur->y0()) { if (++bCur == bEnd) goto OnClear; else continue; }
        break;
      }

      B2D_ASSERT(aCur != aEnd);
      B2D_ASSERT(bCur != bEnd);

      aBandEnd = bRegionGetEndBand(aCur, aEnd);
      bBandEnd = bRegionGetEndBand(bCur, bEnd);

      for (;;) {
        // Vertical intersection of current A and B bands.
        y0 = Math::max(aCur->y0(), bCur->y0());
        y1 = Math::min(aCur->y1(), bCur->y1());

        if (y0 < y1) {
          const IntBox* aBand = aCur;
          const IntBox* bBand = bCur;

          dCurBand = dCur;

          for (;;) {
            // Skip boxes which do not intersect.
            if (aBand->x1() <= bBand->x0()) { if (++aBand == aBandEnd) goto OnIntersectBandDone; else continue; }
            if (bBand->x1() <= aBand->x0()) { if (++bBand == bBandEnd) goto OnIntersectBandDone; else continue; }

            // Horizontal intersection of current A and B boxes.
            int x0 = Math::max(aBand->x0(), bBand->x0());
            int x1 = Math::min(aBand->x1(), bBand->x1());

            B2D_ASSERT(x0 < x1);
            B2D_ASSERT(dCur != dEnd);

            dCur->reset(x0, y0, x1, y1);
            dCur++;

            // Advance.
            if (aBand->x1() == x1 && ++aBand == aBandEnd) break;
            if (bBand->x1() == x1 && ++bBand == bBandEnd) break;
          }

          // Update bounding box and coalesce.
OnIntersectBandDone:
          if (dCurBand != dCur) {
            if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
            if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

            if (dPrevBand)
              dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
            else
              dPrevBand = dCurBand;
          }
        }

        // Advance A.
        if (aCur->y1() == y1) {
          if ((aCur = aBandEnd) == aEnd || aCur->y0() >= yStop) break;
          aBandEnd = bRegionGetEndBand(aCur, aEnd);
        }

        // Advance B.
        if (bCur->y1() == y1) {
          if ((bCur = bBandEnd) == bEnd || bCur->y0() >= yStop) break;
          bBandEnd = bRegionGetEndBand(bCur, bEnd);
        }
      }
      break;
    }

    // ------------------------------------------------------------------------
    // [Union]
    // ------------------------------------------------------------------------

    case Region::kOpUnion: {
      dstBBoxX0 = Math::min(aBoundingBox.x0(), bBoundingBox.x0());
      dstBBoxX1 = Math::max(aBoundingBox.x1(), bBoundingBox.x1());

      aBandEnd = bRegionGetEndBand(aCur, aEnd);
      bBandEnd = bRegionGetEndBand(bCur, bEnd);

      y0 = Math::min(aCur->y0(), bCur->y0());
      for (;;) {
        const IntBox* aBand = aCur;
        const IntBox* bBand = bCur;

        B2D_REGION_ENSURE_SPACE((size_t)(aBandEnd - aBand) + (size_t)(bBandEnd - bBand), false);
        dCurBand = dCur;

        // Merge bands which do not intersect.
        if (bBand->y0() > y0) {
          y1 = Math::min(aBand->y1(), bBand->y0());
          do {
            B2D_ASSERT(dCur != dEnd);
            dCur->reset(aBand->x0(), y0, aBand->x1(), y1);
            dCur++;
          } while (++aBand != aBandEnd);
          goto OnUnionBandDone;
        }

        if (aBand->y0() > y0) {
          y1 = Math::min(bBand->y1(), aBand->y0());
          do {
            B2D_ASSERT(dCur != dEnd);
            dCur->reset(bBand->x0(), y0, bBand->x1(), y1);
            dCur++;
          } while (++bBand != bBandEnd);
          goto OnUnionBandDone;
        }

        // Vertical intersection of current A and B bands.
        y1 = Math::min(aBand->y1(), bBand->y1());
        B2D_ASSERT(y0 < y1);

        do {
          int x0;
          int x1;

          if (aBand->x0() < bBand->x0()) {
            x0 = aBand->x0();
            x1 = aBand->x1();
            aBand++;
          }
          else {
            x0 = bBand->x0();
            x1 = bBand->x1();
            bBand++;
          }

          for (;;) {
            bool didAdvance = false;

            while (aBand != aBandEnd && aBand->x0() <= x1) {
              x1 = Math::max(x1, aBand->x1());
              aBand++;
              didAdvance = true;
            }

            while (bBand != bBandEnd && bBand->x0() <= x1) {
              x1 = Math::max(x1, bBand->x1());
              bBand++;
              didAdvance = true;
            }

            if (!didAdvance)
              break;
          }

          #ifdef B2D_BUILD_DEBUG
          B2D_ASSERT(dCur != dEnd);
          if (dCur != dCurBand)
            B2D_ASSERT(dCur[-1].x1() < x0);
          #endif

          dCur->reset(x0, y0, x1, y1);
          dCur++;
        } while (aBand != aBandEnd && bBand != bBandEnd);

        // Merge boxes which do not intersect.
        while (aBand != aBandEnd) {
          #ifdef B2D_BUILD_DEBUG
          B2D_ASSERT(dCur != dEnd);
          if (dCur != dCurBand)
            B2D_ASSERT(dCur[-1].x1() < aBand->x0());
          #endif

          dCur->reset(aBand->x0(), y0, aBand->x1(), y1);
          dCur++;
          aBand++;
        }

        while (bBand != bBandEnd) {
          #ifdef B2D_BUILD_DEBUG
          B2D_ASSERT(dCur != dEnd);
          if (dCur != dCurBand)
            B2D_ASSERT(dCur[-1].x1() < bBand->x0());
          #endif

          dCur->reset(bBand->x0(), y0, bBand->x1(), y1);
          dCur++;
          bBand++;
        }

        // Coalesce.
OnUnionBandDone:
        if (dPrevBand)
          dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
        else
          dPrevBand = dCurBand;

        y0 = y1;

        // Advance A.
        if (aCur->y1() == y1) {
          if ((aCur = aBandEnd) == aEnd) break;
          aBandEnd = bRegionGetEndBand(aCur, aEnd);
        }

        // Advance B.
        if (bCur->y1() == y1) {
          if ((bCur = bBandEnd) == bEnd) break;
          bBandEnd = bRegionGetEndBand(bCur, bEnd);
        }

        y0 = Math::max(y0, Math::min(aCur->y0(), bCur->y0()));
      }

      if (aCur != aEnd) goto OnMergeA;
      if (bCur != bEnd) goto OnMergeB;
      break;
    }

    // ------------------------------------------------------------------------
    // [Xor]
    // ------------------------------------------------------------------------

    case Region::kOpXor: {
      aBandEnd = bRegionGetEndBand(aCur, aEnd);
      bBandEnd = bRegionGetEndBand(bCur, bEnd);

      y0 = Math::min(aCur->y0(), bCur->y0());
      for (;;) {
        const IntBox* aBand = aCur;
        const IntBox* bBand = bCur;

        B2D_REGION_ENSURE_SPACE(((size_t)(aBandEnd - aBand) + (size_t)(bBandEnd - bBand)) * 2, false);
        dCurBand = dCur;

        // Merge bands which do not intersect.
        if (bBand->y0() > y0) {
          y1 = Math::min(aBand->y1(), bBand->y0());
          do {
            B2D_ASSERT(dCur != dEnd);
            dCur->reset(aBand->x0(), y0, aBand->x1(), y1);
            dCur++;
          } while (++aBand != aBandEnd);
          goto OnXorBandDone;
        }

        if (aBand->y0() > y0) {
          y1 = Math::min(bBand->y1(), aBand->y0());
          do {
            B2D_ASSERT(dCur != dEnd);
            dCur->reset(bBand->x0(), y0, bBand->x1(), y1);
            dCur++;
          } while (++bBand != bBandEnd);
          goto OnXorBandDone;
        }

        // Vertical intersection of current A and B bands.
        y1 = Math::min(aBand->y1(), bBand->y1());
        B2D_ASSERT(y0 < y1);

        {
          int pos = Math::min(aBand->x0(), bBand->x0());
          int x0;
          int x1;

          for (;;) {
            if (aBand->x1() <= bBand->x0()) {
              x0 = Math::max(aBand->x0(), pos);
              x1 = aBand->x1();
              pos = x1;
              goto OnXorMerge;
            }

            if (bBand->x1() <= aBand->x0()) {
              x0 = Math::max(bBand->x0(), pos);
              x1 = bBand->x1();
              pos = x1;
              goto OnXorMerge;
            }

            x0 = pos;
            x1 = Math::max(aBand->x0(), bBand->x0());
            pos = Math::min(aBand->x1(), bBand->x1());

            if (x0 >= x1)
              goto OnXorSkip;

OnXorMerge:
            B2D_ASSERT(x0 < x1);
            if (dCur != dCurBand && dCur[-1].x1() == x0) {
              dCur[-1]._x1 = x1;
            }
            else {
              dCur->reset(x0, y0, x1, y1);
              dCur++;
            }

OnXorSkip:
            if (aBand->x1() <= pos) aBand++;
            if (bBand->x1() <= pos) bBand++;

            if (aBand == aBandEnd || bBand == bBandEnd)
              break;
            pos = Math::max(pos, Math::min(aBand->x0(), bBand->x0()));
          }

          // Merge boxes which do not intersect.
          if (aBand != aBandEnd) {
            x0 = Math::max(aBand->x0(), pos);
            for (;;) {
              x1 = aBand->x1();
              B2D_ASSERT(x0 < x1);
              B2D_ASSERT(dCur != dEnd);

              if (dCur != dCurBand && dCur[-1].x1() == x0) {
                dCur[-1]._x1 = x1;
              }
              else {
                dCur->reset(x0, y0, x1, y1);
                dCur++;
              }

              if (++aBand == aBandEnd)
                break;
              x0 = aBand->x0();
            }
          }

          if (bBand != bBandEnd) {
            x0 = Math::max(bBand->x0(), pos);
            for (;;) {
              x1 = bBand->x1();
              B2D_ASSERT(x0 < x1);
              B2D_ASSERT(dCur != dEnd);

              if (dCur != dCurBand && dCur[-1].x1() == x0) {
                dCur[-1]._x1 = x1;
              }
              else {
                dCur->reset(x0, y0, x1, y1);
                dCur++;
              }

              if (++bBand == bBandEnd)
                break;
              x0 = bBand->x0();
            }
          }
        }

        // Update bounding box and coalesce.
OnXorBandDone:
        if (dCurBand != dCur) {
          if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
          if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

          if (dPrevBand)
            dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
          else
            dPrevBand = dCurBand;
        }

        y0 = y1;

        // Advance A.
        if (aCur->y1() == y1) {
          if ((aCur = aBandEnd) == aEnd)
            break;
          aBandEnd = bRegionGetEndBand(aCur, aEnd);
        }

        // Advance B.
        if (bCur->y1() == y1) {
          if ((bCur = bBandEnd) == bEnd)
            break;
          bBandEnd = bRegionGetEndBand(bCur, bEnd);
        }

        y0 = Math::max(y0, Math::min(aCur->y0(), bCur->y0()));
      }

      if (aCur != aEnd) goto OnMergeA;
      if (bCur != bEnd) goto OnMergeB;
      break;
    }

    // ------------------------------------------------------------------------
    // [Subtract]
    // ------------------------------------------------------------------------

    case Region::kOpSubtract: {
      aBandEnd = bRegionGetEndBand(aCur, aEnd);
      bBandEnd = bRegionGetEndBand(bCur, bEnd);

      y0 = Math::min(aCur->y0(), bCur->y0());
      for (;;) {
        const IntBox* aBand = aCur;
        const IntBox* bBand = bCur;

        B2D_REGION_ENSURE_SPACE(((size_t)(aBandEnd - aBand) + (size_t)(bBandEnd - bBand)) * 2, false);
        dCurBand = dCur;

        // Merge (A) / Skip (B) bands which do not intersect.
        if (bBand->y0() > y0) {
          y1 = Math::min(aBand->y1(), bBand->y0());
          do {
            B2D_ASSERT(dCur != dEnd);
            dCur->reset(aBand->x0(), y0, aBand->x1(), y1);
            dCur++;
          } while (++aBand != aBandEnd);
          goto OnSubtractBandDone;
        }

        if (aBand->y0() > y0) {
          y1 = Math::min(bBand->y1(), aBand->y0());
          goto OnSubtractBandSkip;
        }

        // Vertical intersection of current A and B bands.
        y1 = Math::min(aBand->y1(), bBand->y1());
        B2D_ASSERT(y0 < y1);

        {
          int pos = aBand->x0();
          int sub = bBand->x0();

          int x0;
          int x1;

          for (;;) {
            if (aBand->x1() <= sub) {
              x0 = pos;
              x1 = aBand->x1();
              pos = x1;

              if (x0 < x1)
                goto OnSubtractMerge;
              else
                goto OnSubtractSkip;
            }

            if (aBand->x0() >= sub) {
              pos = bBand->x1();
              goto OnSubtractSkip;
            }

            x0 = pos;
            x1 = bBand->x0();
            pos = bBand->x1();

OnSubtractMerge:
            B2D_ASSERT(x0 < x1);
            dCur->reset(x0, y0, x1, y1);
            dCur++;

OnSubtractSkip:
            if (aBand->x1() <= pos) aBand++;
            if (bBand->x1() <= pos) bBand++;

            if (aBand == aBandEnd || bBand == bBandEnd)
              break;

            sub = bBand->x0();
            pos = Math::max(pos, aBand->x0());
          }

          // Merge boxes (A) / Ignore boxes (B) which do not intersect.
          while (aBand != aBandEnd) {
            x0 = Math::max(aBand->x0(), pos);
            x1 = aBand->x1();

            if (x0 < x1) {
              B2D_ASSERT(dCur != dEnd);
              dCur->reset(x0, y0, x1, y1);
              dCur++;
            }
            aBand++;
          }
        }

        // Update bounding box and coalesce.
OnSubtractBandDone:
        if (dCurBand != dCur) {
          if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
          if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

          if (dPrevBand)
            dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
          else
            dPrevBand = dCurBand;
        }

OnSubtractBandSkip:
        y0 = y1;

        // Advance A.
        if (aCur->y1() == y1) {
          if ((aCur = aBandEnd) == aEnd)
            break;
          aBandEnd = bRegionGetEndBand(aCur, aEnd);
        }

        // Advance B.
        if (bCur->y1() == y1) {
          if ((bCur = bBandEnd) == bEnd)
            break;
          bBandEnd = bRegionGetEndBand(bCur, bEnd);
        }

        y0 = Math::max(y0, Math::min(aCur->y0(), bCur->y0()));
      }

      if (aCur != aEnd)
        goto OnMergeA;
      break;
    }

    default:
      B2D_NOT_REACHED();
  }

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

OnEnd:
  dstI->_size = (size_t)(dCur - dstI->data());

  if (dstI->_size == 0)
    dstI->_boundingBox.reset();
  else
    dstI->_boundingBox.reset(dstBBoxX0, dstI->_data[0].y0(), dstBBoxX1, dCur[-1].y1());

  B2D_ASSERT(bRegionIsDataValid(dstI));

  if (dst.impl() != dstI) {
    RegionImpl* oldI = dst.impl();
    dst._impl = dstI;
    oldI->release();
  }

  return kErrorOk;

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

OnClear:
  if (dst.impl() != dstI)
    dstI->release();
  return dst.reset();

  // --------------------------------------------------------------------------
  // [Out Of Memory]
  // --------------------------------------------------------------------------

OnOutOfMemory:
  if (dst.impl() != dstI)
    dstI->release();
  return DebugUtils::errored(kErrorNoMemory);

  // --------------------------------------------------------------------------
  // [Merge]
  // --------------------------------------------------------------------------

OnMergeB:
  B2D_ASSERT(aCur == aEnd);

  aCur = bCur;
  aEnd = bEnd;
  aBandEnd = bBandEnd;

OnMergeA:
  B2D_ASSERT(aCur != aEnd);

  if (y0 >= aCur->y1()) {
    if ((aCur = aBandEnd) == aEnd)
      goto OnEnd;
    aBandEnd = bRegionGetEndBand(aCur, aEnd);
  }

  y0 = Math::max(y0, aCur->y0());
  y1 = aCur->y1();

  B2D_REGION_ENSURE_SPACE((size_t)(aEnd - aCur), true);
  dCurBand = dCur;

  do {
    B2D_ASSERT(dCur != dEnd);
    dCur->reset(aCur->x0(), y0, aCur->x1(), y1);
    dCur++;
  } while (++aCur != aBandEnd);

  if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
  if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

  if (dPrevBand)
    dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);

  if (aCur == aEnd)
    goto OnEnd;

  if (op == Region::kOpUnion) {
    // Special case for UNION. The bounding box is easily calculated using A
    // and B bounding boxes. We don't need to contribute to the calculation
    // within this loop.
    do {
      B2D_ASSERT(dCur != dEnd);
      dCur->reset(aCur->x0(), aCur->y0(), aCur->x1(), aCur->y1());
      dCur++;
    } while (++aCur != aEnd);
  }
  else {
    do {
      B2D_ASSERT(dCur != dEnd);
      dCur->reset(aCur->x0(), aCur->y0(), aCur->x1(), aCur->y1());
      dCur++;

      if (dstBBoxX0 > aCur->x0()) dstBBoxX0 = aCur->x0();
      if (dstBBoxX1 < aCur->x1()) dstBBoxX1 = aCur->x1();
    } while (++aCur != aEnd);
  }

  goto OnEnd;
}

// ============================================================================
// [b2d::Region - Combine - Region + Region]
// ============================================================================

// Infinite regions require special handling to keep the special infinite
// region data with the Region instance. However, if the combining operator
// is one of XOR, SUBTRACT, or SUBTRACT_REV, then the infinity information
// might be lost.

Error Region::combine(Region& dst, const Region& a, const Region& b, uint32_t op) noexcept {
  const RegionImpl* aI = a.impl();
  const RegionImpl* bI = b.impl();

  size_t aSize = aI->size();
  size_t bSize = bI->size();

  // Run fast-paths when possible. Empty regions are handled too.
  if (aSize <= 1) {
    // Handle infinite A.
    if (aI->isInfinite()) {
      switch (op) {
        case kOpReplace  : return dst.setRegion(b); // DST <- B.
        case kOpIntersect: return dst.setRegion(b); // DST <- B.
        case kOpUnion    : return dst.setRegion(a); // DST <- A.
        case kOpXor      : break;
        case kOpSubtract : break;
        case kOpSubRev   : return dst.reset();      // DST <- CLEAR.

        default:
          return DebugUtils::errored(kErrorInvalidArgument);
      }
    }

    IntBox aBox { aI->boundingBox() };
    return combine(dst, aBox, b, op);
  }

  if (bSize <= 1) {
    // Handle infinite B.
    if (bI->isInfinite()) {
      switch (op) {
        case kOpReplace  : return dst.setRegion(b); // DST <- B.
        case kOpIntersect: return dst.setRegion(a); // DST <- A.
        case kOpUnion    : return dst.setRegion(b); // DST <- B.
        case kOpXor      : break;
        case kOpSubtract : return dst.reset();      // DST <- CLEAR.
        case kOpSubRev   : break;

        default:
          return DebugUtils::errored(kErrorInvalidArgument);
      }
    }

    IntBox bBox { bI->boundingBox() };
    return combine(dst, a, bBox, op);
  }

  B2D_ASSERT(aSize > 1 && bSize > 1);

  switch (op) {
    case kOpReplace:
      return dst.setRegion(b);

    case kOpIntersect:
      if (!aI->boundingBox().overlaps(bI->boundingBox()))
        return dst.reset();
      break;

    case kOpXor:
      // Check whether to use UNION instead of XOR.
      if (aI->boundingBox().overlaps(bI->boundingBox()))
        break;

      op = kOpUnion;
      B2D_FALLTHROUGH;

    case kOpUnion:
      // Check whether to use JOIN instead of UNION. This is a special case,
      // but happens often when the region is constructed from many boxes using
      // UNION operation.
      if (aI->boundingBox().y1() <= bI->boundingBox().y0() || _bRegionCanJoin(aI->data()[aSize - 1], bI->data()[0])) {
        if (dst.impl() == a.impl())
          return _bRegionJoinTo(dst, bI->data(), bSize, bI->boundingBox());

        if (dst.impl() == b.impl()) {
          Region bTmp(b);
          return _bRegionCombineJoinAB(dst, aI->data(), aSize, aI->boundingBox(), bI->data(), bSize, bI->boundingBox());
        }

        return _bRegionCombineJoinAB(dst, aI->data(), aSize, aI->boundingBox(), bI->data(), bSize, bI->boundingBox());
      }

      if (bI->boundingBox().y1() <= aI->boundingBox().y0() || _bRegionCanJoin(bI->data()[bSize - 1], aI->data()[0])) {
        if (dst.impl() == b.impl()) {
          return _bRegionJoinTo(dst, aI->data(), aSize, aI->boundingBox());
        }

        if (dst.impl() == a.impl()) {
          Region aTmp(a);
          return _bRegionCombineJoinAB(dst, bI->data(), bSize, bI->boundingBox(), aI->data(), aSize, aI->boundingBox());
        }

        return _bRegionCombineJoinAB(dst, bI->data(), bSize, bI->boundingBox(), aI->data(), aSize, aI->boundingBox());
      }
      break;

    case kOpSubtract:
      if (!aI->boundingBox().overlaps(bI->boundingBox()))
        return dst.setRegion(a);;
      break;

    case kOpSubRev:
      if (!aI->boundingBox().overlaps(bI->boundingBox()))
        return dst.setRegion(b);

      op = kOpSubtract;
      std::swap(aI, bI);
      std::swap(aSize, bSize);
      break;

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  return Region_combineAB(dst,
                          aI->_data, aSize, aI->boundingBox(),
                          bI->_data, bSize, bI->boundingBox(),
                          op,
                          dst.impl() == aI || dst.impl() == bI);
}

// ============================================================================
// [b2d::Region - Combine - Region + Box/Rect]
// ============================================================================

Error Region::combine(Region& dst, const Region& a, const IntBox& b, uint32_t op) noexcept {
  RegionImpl* aI = a.impl();

  // Handle the infinite region A.
  if (aI->isInfinite()) {
    switch (op) {
      case kOpReplace  : return dst.setBox(b); // DST <- B.
      case kOpIntersect: return dst.setBox(b); // DST <- B.
      case kOpUnion    : dst.setRegion(a);     // DST <- A.
      case kOpXor      : break;
      case kOpSubtract : break;
      case kOpSubRev   : return dst.reset();   // DST <- CLEAR.

      default:
        return DebugUtils::errored(kErrorInvalidArgument);
    }
  }

  // Run fast-paths when possible. Empty regions are handled too.
  if (aI->_size <= 1)
    return combine(dst, aI->boundingBox(), b, op);

  switch (op) {
    case kOpReplace:
      if (!b.isValid())
        return dst.reset();

      return dst.setBox(b); // DST <- B.

    case kOpIntersect:
      if (!b.isValid() || !b.overlaps(aI->boundingBox()))
        return dst.reset();

      if (b.subsumes(aI->boundingBox()))
        return dst.setRegion(a);

      return _bRegionCombineClip(dst, a, b);

    case kOpXor:
    case kOpUnion:
      if (!b.isValid())
        return dst.setRegion(a);

      if (aI->boundingBox().y1() <= b.y0() || _bRegionCanJoin(aI->_data[aI->_size - 1], b)) {
        if (dst == a)
          return _bRegionJoinTo(dst, &b, 1, b);
        else
          return _bRegionCombineJoinAB(dst, aI->_data, aI->_size, aI->boundingBox(), &b, 1, b);
      }
      break;

    case kOpSubtract:
      if (!b.isValid() || !b.overlaps(aI->boundingBox()))
        return dst.setRegion(a);
      break;

    case kOpSubRev:
      if (!b.isValid())
        return dst.reset();

      if (!b.overlaps(aI->boundingBox()))
        return dst.setBox(b);

      return Region_combineAB(dst, &b, 1, b, aI->_data, aI->_size, aI->boundingBox(), kOpSubtract, dst.impl() == aI);

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  return Region_combineAB(dst, aI->data(), aI->size(), aI->boundingBox(), &b, 1, b, op, dst.impl() == aI);
}

Error Region::combine(Region& dst, const Region& a, const IntRect& b, uint32_t op) noexcept {
  return combine(dst, a, IntBox(b), op);
}

// ============================================================================
// [b2d::Region - Combine - Box/Rect + Region]
// ============================================================================

Error Region::combine(Region& dst, const IntBox& a, const Region& b, uint32_t op) noexcept {
  RegionImpl* bI = b.impl();

  // Handle the infinite region B.
  if (bI->isInfinite()) {
    switch (op) {
      case kOpReplace  : return dst.setRegion(b); // DST <- B.
      case kOpIntersect: return dst.setBox(a);    // DST <- A.
      case kOpUnion    : return dst.setRegion(b); // DST <- B
      case kOpXor      : break;
      case kOpSubtract : return dst.reset();      // DST <- CLEAR.
      case kOpSubRev   : break;

      default:
        return DebugUtils::errored(kErrorInvalidArgument);
    }
  }

  // Run fast-paths when possible. Empty regions are handled too.
  if (bI->_size <= 1)
    return combine(dst, a, bI->boundingBox(), op);

  switch (op) {
    case kOpReplace:
      return dst.setRegion(b);

    case kOpIntersect:
      if (!a.isValid() || !a.overlaps(bI->boundingBox()))
        return dst.reset();

      if (a.subsumes(bI->boundingBox()))
        return dst.setRegion(b);

      return _bRegionCombineClip(dst, b, a);

    case kOpUnion:
      if (!a.isValid())
        return dst.setRegion(b);
      break;

    case kOpXor:
      if (!a.isValid())
        return dst.setRegion(b);

      if (!a.overlaps(bI->boundingBox()))
        op = kOpUnion;
      break;

    case kOpSubtract:
      if (!a.isValid())
        return dst.reset();

      if (!a.overlaps(bI->boundingBox()))
        return dst.setBox(a);
      break;

    case kOpSubRev:
      if (!a.isValid() || !a.overlaps(bI->boundingBox()))
        return dst.setRegion(b);

      return Region_combineAB(dst, bI->data(), bI->size(), bI->boundingBox(), &a, 1, a, op, dst.impl() == bI);

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  return Region_combineAB(dst, &a, 1, a, bI->data(), bI->size(), bI->boundingBox(), op, dst.impl() == bI);
}

Error Region::combine(Region& dst, const IntRect& a, const Region& b, uint32_t op) noexcept {
  return combine(dst, IntBox(a), b, op);
}

// ============================================================================
// [b2d::Region - Combine - Box (op) Box]
// ============================================================================

static B2D_INLINE Error _bRegionCombine(Region& dst, const IntBox* a, const IntBox* b, uint32_t op) noexcept {
  // There are several combinations of A and B.
  //
  // Special cases:
  //
  //   1) Both rectangles are invalid.
  //   2) Only one rectangle is valid (one invalid).
  //   3) Rectangles do not intersect, but they are continuous on the Y axis.
  //      (In some cases this can be extended that rectangles intersect, but
  //       their left and right coordinates are shared.)
  //   4) Rectangles do not intersect, but they are continuous on the X axis.
  //      (In some cases this can be extended that rectangles intersect, but
  //       their top and bottom coordinates are shared.)
  //
  // Common cases:
  //
  //   5) Rectangles do not intersect and do not share area on the Y axis.
  //   6) Rectangles do not intersect, but they share area on the Y axis.
  //   7) Rectangles intersect.
  //   8) One rectangle overlaps the other.
  //
  //   +--------------+--------------+--------------+--------------+
  //   |       1)     |       2)     |       3)     |       4)     |
  //   |              |              |              |              |
  //   |              |  +--------+  |  +--------+  |  +----+----+ |
  //   |              |  |        |  |  |        |  |  |    |    | |
  //   |              |  |        |  |  |        |  |  |    |    | |
  //   |              |  |        |  |  +--------+  |  |    |    | |
  //   |              |  |        |  |  |        |  |  |    |    | |
  //   |              |  |        |  |  |        |  |  |    |    | |
  //   |              |  |        |  |  |        |  |  |    |    | |
  //   |              |  +--------+  |  +--------+  |  +----+----+ |
  //   |              |              |              |              |
  //   +--------------+--------------+--------------+--------------+
  //   |       5)     |       6)     |       7)     |       8)     |
  //   |              |              |              |              |
  //   |  +----+      | +---+        |  +-----+     |  +--------+  |
  //   |  |    |      | |   |        |  |     |     |  |        |  |
  //   |  |    |      | |   | +----+ |  |  +--+--+  |  |  +--+  |  |
  //   |  +----+      | +---+ |    | |  |  |  |  |  |  |  |  |  |  |
  //   |              |       |    | |  +--+--+  |  |  |  +--+  |  |
  //   |      +----+  |       +----+ |     |     |  |  |        |  |
  //   |      |    |  |              |     +-----+  |  +--------+  |
  //   |      |    |  |              |              |              |
  //   |      +----+  |              |              |              |
  //   +--------------+--------------+--------------+--------------+

  // Maximum number of generated rectangles by any operator is 4.
  IntBox box[4];
  uint32_t boxCount = 0;

  switch (op) {
    // ------------------------------------------------------------------------
    // [Replace]
    // ------------------------------------------------------------------------

    case Region::kOpReplace:
OnReplace:
      if (!b->isValid())
        goto OnClear;
      goto OnSetB;

    // ------------------------------------------------------------------------
    // [Intersect]
    // ------------------------------------------------------------------------

    case Region::kOpIntersect:
      // Case 1, 2, 3, 4, 5, 6.
      if (!IntBox::intersect(box[0], *a, *b))
        goto OnClear;

      // Case 7, 8.
      goto OnSetBox;

    // ------------------------------------------------------------------------
    // [Union]
    // ------------------------------------------------------------------------

    case Region::kOpUnion:
      // Case 1, 2.
      if (!a->isValid())
        goto OnReplace;
      if (!b->isValid())
        goto OnSetA;

      // Make first the upper rectangle.
      if (a->y0() > b->y0())
        std::swap(a, b);

OnUnion:
      // Case 3, 5.
      if (a->y1() <= b->y0()) {
        box[0] = *a;
        box[1] = *b;
        boxCount = 2;

        // Coalesce (Case 3).
        if (box[0].y1() == box[1].y0() && box[0].x0() == box[1].x0() && box[0].x1() == box[1].x1()) {
          box[0]._y1 = box[1].y1();
          boxCount = 1;
        }

        goto OnSetArray;
      }

      // Case 4 - with addition that rectangles can intersect.
      //        - with addition that rectangles do not need to be
      //          continuous on the X axis.
      if (a->y0() == b->y0() && a->y1() == b->y1()) {
        box[0]._y0 = a->y0();
        box[0]._y1 = a->y1();

        if (a->x0() > b->x0())
          std::swap(a, b);

        box[0]._x0 = a->x0();
        box[0]._x1 = a->x1();

        // Intersects or continuous.
        if (b->x0() <= a->x1()) {
          if (b->x1() > a->x1())
            box[0]._x1 = b->x1();
          boxCount = 1;
          goto OnSetBox;
        }
        else {
          box[1].reset(b->x0(), box[0].y0(), b->x1(), box[0].y1());
          boxCount = 2;
          goto OnSetArray;
        }
      }

      // Case 6, 7, 8.
      B2D_ASSERT(b->y0() < a->y1());
      {
        // Top part.
        if (a->y0() < b->y0()) {
          box[0].reset(a->x0(), a->y0(), a->x1(), b->y0());
          boxCount = 1;
        }

        // Inner part.
        int iy0 = b->y0();
        int iy1 = Math::min(a->y1(), b->y1());

        if (a->x0() > b->x0())
          std::swap(a, b);

        int ix0 = Math::max(a->x0(), b->x0());
        int ix1 = Math::min(a->x1(), b->x1());

        if (ix0 > ix1) {
          box[boxCount + 0].reset(a->x0(), iy0, a->x1(), iy1);
          box[boxCount + 1].reset(b->x0(), iy0, b->x1(), iy1);
          boxCount += 2;
        }
        else {
          B2D_ASSERT(a->x1() >= ix0 && b->x0() <= ix1);

          // If the A or B subsumes the intersection area, extend also the iy1
          // and skip the bottom part (we join it).
          if (a->x0() <= ix0 && a->x1() >= ix1 && iy1 < a->y1()) iy1 = a->y1();
          if (b->x0() <= ix0 && b->x1() >= ix1 && iy1 < b->y1()) iy1 = b->y1();
          box[boxCount].reset(a->x0(), iy0, Math::max(a->x1(), b->x1()), iy1);

          // Coalesce.
          if (boxCount == 1 && box[0].x0() == box[1].x0() && box[0].x1() == box[1].x1())
            box[0]._y1 = box[1].y1();
          else
            boxCount++;
        }

        // Bottom part.
        if (a->y1() > iy1) {
          box[boxCount].reset(a->x0(), iy1, a->x1(), a->y1());
          goto OnUnionLastCoalesce;
        }
        else if (b->y1() > iy1) {
          box[boxCount].reset(b->x0(), iy1, b->x1(), b->y1());
OnUnionLastCoalesce:
          if (boxCount == 1 && box[0].x0() == box[1].x0() && box[0].x1() == box[1].x1())
            box[0]._y1 = box[1].y1();
          else
            boxCount++;
        }
      }
      goto OnSetArray;

    // ------------------------------------------------------------------------
    // [Xor]
    // ------------------------------------------------------------------------

    case Region::kOpXor:
      if (!a->isValid())
        goto OnReplace;
      if (!b->isValid())
        goto OnSetA;

      // Make first the upper rectangle.
      if (a->y0() > b->y0())
        std::swap(a, b);

      // Case 3, 4, 5, 6.
      //
      // If the input boxes A and B do not intersect then we can use UNION
      // operator instead.
      if (!IntBox::intersect(box[3], *a, *b))
        goto OnUnion;

      // Case 7, 8.
      //
      // Top part.
      if (a->y0() < b->y0()) {
        box[0].reset(a->x0(), a->y0(), a->x1(), b->y0());
        boxCount = 1;
      }

      // Inner part.
      if (a->x0() > b->x0())
        std::swap(a, b);

      if (a->x0() < box[3].x0()) box[boxCount++].reset(a->x0(), box[3].y0(), box[3].x0(), box[3].y1());
      if (b->x1() > box[3].x1()) box[boxCount++].reset(box[3].x1(), box[3].y0(), b->x1(), box[3].y1());

      // Bottom part.
      if (a->y1() > b->y1())
        std::swap(a, b);

      if (b->y1() > box[3].y1())
        box[boxCount++].reset(b->x0(), box[3].y1(), b->x1(), b->y1());

      if (boxCount == 0)
        goto OnClear;

      goto OnSetArray;

    // ------------------------------------------------------------------------
    // [Subtract]
    // ------------------------------------------------------------------------

    case Region::kOpSubtract:
OnSubtract:
      if (!a->isValid())
        goto OnClear;
      if (!b->isValid())
        goto OnSetA;

      // Case 3, 4, 5, 6.
      //
      // If the input boxes A and B do not intersect then the result is A.
      if (!IntBox::intersect(box[3], *a, *b))
        goto OnSetA;

      // Case 7, 8.
      //
      // Top part.
      if (a->y0() < b->y0()) {
        box[0].reset(a->x0(), a->y0(), a->x1(), box[3].y0());
        boxCount = 1;
      }

      // Inner part.
      if (a->x0() < box[3].x0())
        box[boxCount++].reset(a->x0(), box[3].y0(), box[3].x0(), box[3].y1());
      if (box[3].x1() < a->x0())
        box[boxCount++].reset(box[3].x1(), box[3].y0(), a->x1(), box[3].y1());

      // Bottom part.
      if (a->y1() > box[3].y1())
        box[boxCount++].reset(a->x0(), box[3].y1(), a->x1(), a->y1());

      if (boxCount == 0)
        goto OnClear;

      goto OnSetArray;

    // ------------------------------------------------------------------------
    // [SubRev]
    // ------------------------------------------------------------------------

    case Region::kOpSubRev:
      std::swap(a, b);
      goto OnSubtract;

    // ------------------------------------------------------------------------
    // [Invalid]
    // ------------------------------------------------------------------------

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

OnClear:
  return dst.reset();

OnSetA:
  box[0] = *a;
  goto OnSetBox;

OnSetB:
  box[0] = *b;

OnSetBox:
  {
    B2D_ASSERT(box[0].isValid());
    B2D_PROPAGATE(dst.prepare(1));

    RegionImpl* dstI = dst.impl();
    dstI->_size = 1;
    dstI->_boundingBox = box[0];
    dstI->_data[0] = box[0];

    B2D_ASSERT(bRegionIsDataValid(dstI));
    return kErrorOk;
  }

OnSetArray:
  {
    B2D_ASSERT(boxCount > 0);
    B2D_PROPAGATE(dst.prepare(boxCount));

    RegionImpl* dstI = dst.impl();
    dstI->_size = boxCount;
    bRegionCopyDataEx(dstI->_data, box, boxCount, &dstI->_boundingBox);

    B2D_ASSERT(bRegionIsDataValid(dstI));
    return kErrorOk;
  }
}

Error Region::combine(Region& dst, const IntBox& a, const IntBox& b, uint32_t op) noexcept {
  return _bRegionCombine(dst, &a, &b, op);
}

Error Region::combine(Region& dst, const IntRect& a, const IntRect& b, uint32_t op) noexcept {
  IntBox aBox(a);
  IntBox bBox(b);

  return _bRegionCombine(dst, &aBox, &bBox, op);
}

// ============================================================================
// [b2d::Region - Translate]
// ============================================================================

static Error B2D_CDECL Region_translate(Region& dst, const Region& src, const IntPoint& pt) noexcept {
  RegionImpl* dstI = dst.impl();
  RegionImpl* srcI = src.impl();

  int tx = pt.x();
  int ty = pt.y();

  if (srcI->isInfinite() || (tx | ty) == 0)
    return dst.setRegion(src);

  size_t size = srcI->_size;
  if (size == 0)
    return dst.reset();

  // If the translation cause arithmetic overflow or underflow then we first
  // clip the input region into safe boundary which might be translated.
  constexpr int kMin = std::numeric_limits<int>::min();
  constexpr int kMax = std::numeric_limits<int>::max();

  bool saturate = ((tx < 0) & (srcI->boundingBox().x0() < kMin - tx)) |
                  ((ty < 0) & (srcI->boundingBox().y0() < kMin - ty)) |
                  ((tx > 0) & (srcI->boundingBox().x1() > kMax - tx)) |
                  ((ty > 0) & (srcI->boundingBox().y1() > kMax - ty)) ;
  if (saturate)
    return Region::translateClip(dst, src, pt, IntBox(kMin, kMin, kMax, kMax));

  if (dstI->isShared() || size > dstI->capacity()) {
    dstI = bRegionAllocData(size);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }

  IntBox* dstData = dstI->_data;
  IntBox* srcData = srcI->_data;

  dstI->_size = size;
  dstI->_boundingBox.reset(srcI->boundingBox().x0() + tx, srcI->boundingBox().y0() + ty,
                           srcI->boundingBox().x1() + tx, srcI->boundingBox().y1() + ty);

  for (size_t i = 0; i < size; i++)
    dstData[i].reset(srcData->x0() + tx, srcData->y0() + ty, srcData->x1() + tx, srcData->y1() + ty);

  return dstI != dst.impl() ? AnyInternal::replaceImpl(&dst, dstI) : kErrorOk;
}

// ============================================================================
// [b2d::Region - TranslateClip]
// ============================================================================

Error Region::translateClip(Region& dst, const Region& src, const IntPoint& pt, const IntBox& clipBox) noexcept {
  RegionImpl* dstI = dst.impl();
  RegionImpl* srcI = src.impl();

  size_t size = srcI->_size;
  if (size == 0 || !clipBox.isValid())
    return dst.reset();

  if (srcI->isInfinite())
    return dst.setBox(clipBox);

  int tx = pt._x;
  int ty = pt._y;

  // Use faster _bRegionCombineClip() in case that there is no translation.
  if ((tx | ty) == 0)
    return _bRegionCombineClip(dst, src, clipBox);

  int cx0 = clipBox.x0();
  int cy0 = clipBox.y0();
  int cx1 = clipBox.x1();
  int cy1 = clipBox.y1();

  if (tx < 0) {
    cx0 = Math::min(cx0, std::numeric_limits<int>::max() + tx);
    cx1 = Math::min(cx1, std::numeric_limits<int>::max() + tx);
  }
  else if (tx > 0) {
    cx0 = Math::max(cx0, std::numeric_limits<int>::min() + tx);
    cx1 = Math::max(cx1, std::numeric_limits<int>::min() + tx);
  }

  if (ty < 0) {
    cy0 = Math::min(cy0, std::numeric_limits<int>::max() + ty);
    cy1 = Math::min(cy1, std::numeric_limits<int>::max() + ty);
  }
  else if (ty > 0) {
    cy0 = Math::max(cy0, std::numeric_limits<int>::min() + ty);
    cy1 = Math::max(cy1, std::numeric_limits<int>::min() + ty);
  }

  cx0 -= tx;
  cy0 -= ty;

  cx1 -= tx;
  cy1 -= ty;

  if (cx0 >= cx1 || cy0 >= cy1)
    return dst.reset();

  if (dstI->isShared() || size > dstI->capacity()) {
    dstI = bRegionAllocData(size);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }
  B2D_ASSERT(dstI->capacity() >= size);

  IntBox* dCur = dstI->_data;
  IntBox* dPrevBand = nullptr;
  IntBox* dCurBand;

  IntBox* sCur = srcI->_data;
  IntBox* sEnd = sCur + size;

  int dstBBoxX0 = std::numeric_limits<int>::max();
  int dstBBoxX1 = std::numeric_limits<int>::min();

  // Skip boxes which do not intersect with the clip-box.
  while (sCur->y1() <= cy0) {
    if (++sCur == sEnd)
      goto OnEnd;
  }

  // Do the intersection part.
  for (;;) {
    B2D_ASSERT(sCur != sEnd);
    if (sCur->y0() >= cy1)
      break;

    int y0;
    int y1 = 0; // Be quiet.
    int bandY0 = sCur->y0();

    dCurBand = dCur;

    // Skip leading boxes which do not intersect with the clip-box.
    while (sCur->x1() <= cx0) {
      if (++sCur == sEnd) goto OnEnd;
      if (sCur->y0() != bandY0) goto OnSkip;
    }

    // Do the inner part.
    if (sCur->x0() < cx1) {
      y0 = Math::max(sCur->y0(), cy0) + ty;
      y1 = Math::min(sCur->y1(), cy1) + ty;

      // First box.
      B2D_ASSERT(dCur != dstI->data() + dstI->capacity());
      dCur->reset(Math::max(sCur->x0(), cx0) + tx, y0, Math::min(sCur->x1(), cx1) + tx, y1);
      dCur++;

      sCur++;
      if (sCur == sEnd || sCur->y0() != bandY0)
        goto OnMerge;

      // Inner boxes.
      while (sCur->x1() <= cx1) {
        B2D_ASSERT(dCur != dstI->data() + dstI->capacity());
        B2D_ASSERT(sCur->x0() >= cx0 && sCur->x1() <= cx1);

        dCur->reset(sCur->x0() + tx, y0, sCur->x1() + tx, y1);
        dCur++;

        sCur++;
        if (sCur == sEnd || sCur->y0() != bandY0)
          goto OnMerge;
      }

      // Last box.
      if (sCur->x0() < cx1) {
        B2D_ASSERT(dCur != dstI->data() + dstI->capacity());
        B2D_ASSERT(sCur->x0() >= cx0);

        dCur->reset(sCur->x0() + tx, y0, Math::min(sCur->x1(), cx1) + tx, y1);
        dCur++;

        sCur++;
        if (sCur == sEnd || sCur->y0() != bandY0)
          goto OnMerge;
      }

      B2D_ASSERT(sCur->x0() >= cx1);
    }

    // Skip trailing boxes which do not intersect with the clip-box.
    while (sCur->x0() >= cx1) {
      if (++sCur == sEnd) goto OnEnd;
      if (sCur->y0() != bandY0) goto OnMerge;
    }

OnMerge:
    if (dCurBand != dCur) {
      if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
      if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

      if (dPrevBand)
        dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
      else
        dPrevBand = dCurBand;
    }

OnSkip:
    if (sCur == sEnd)
      break;
  }

OnEnd:
  size = (size_t)(dCur - dstI->data());
  dstI->_size = size;

  if (size == 0)
    dstI->_boundingBox.reset();
  else
    dstI->_boundingBox.reset(dstBBoxX0, dstI->_data[0].y0(), dstBBoxX1, dstI->_data[size - 1].y1());
  B2D_ASSERT(bRegionIsDataValid(dstI));

  return dstI != dst.impl() ? AnyInternal::replaceImpl(&dst, dstI) : kErrorOk;
}

// ============================================================================
// [b2d::Region - IntersectClip]
// ============================================================================

Error Region::intersectClip(Region& dst, const Region& a, const Region& b, const IntBox& clipBox) noexcept {
  RegionImpl* aI = a.impl();
  RegionImpl* bI = b.impl();

  size_t aSize = aI->_size;
  size_t bSize = bI->_size;

  if (aSize == 0 || bSize == 0 || !clipBox.isValid())
    return dst.reset();

  int cx0 = Math::max(clipBox.x0(), Math::max(aI->boundingBox().x0(), bI->boundingBox().x0()));
  int cy0 = Math::max(clipBox.y0(), Math::max(aI->boundingBox().y0(), bI->boundingBox().y0()));
  int cx1 = Math::min(clipBox.x1(), Math::min(aI->boundingBox().x1(), bI->boundingBox().x1()));
  int cy1 = Math::min(clipBox.y1(), Math::min(aI->boundingBox().y1(), bI->boundingBox().y1()));

  if (cx0 >= cx1 || cy0 >= cy1)
    return dst.reset();

  if (aSize == 1 || bSize == 1) {
    IntBox newClipBox(cx0, cy0, cx1, cy1);

    if (aSize != 1)
      return _bRegionCombineClip(dst, a, newClipBox);
    if (bSize != 1)
      return _bRegionCombineClip(dst, b, newClipBox);

    return dst.setBox(newClipBox);
  }

  // --------------------------------------------------------------------------
  // [Destination]
  // --------------------------------------------------------------------------

  RegionImpl* dstI = dst.impl();
  bool requireNewData = dst == a || dst == b || dstI->isShared();

  IntBox* dCur = nullptr;
  IntBox* dPrevBand = nullptr;
  IntBox* dCurBand = nullptr;

  int dstBBoxX0 = std::numeric_limits<int>::max();
  int dstBBoxX1 = std::numeric_limits<int>::min();

  // --------------------------------------------------------------------------
  // [Source - A & B]
  // --------------------------------------------------------------------------

  const IntBox* aCur = aI->data();
  const IntBox* bCur = bI->data();

  const IntBox* aEnd = aCur + aSize;
  const IntBox* bEnd = bCur + bSize;

  const IntBox* aBandEnd = nullptr;
  const IntBox* bBandEnd = nullptr;

  int y0, y1;
  size_t estimatedSize;

  // Skip all parts which do not intersect. If there is no intersection
  // detected then this loop can skip into the `OnClear` label.
  while (aCur->y0() <= cy0) { if (++aCur == aEnd) goto OnClear; }
  while (bCur->y0() <= cy0) { if (++bCur == bEnd) goto OnClear; }

  for (;;) {
    bool cont = false;

    while (aCur->y1() <= bCur->y0()) { if (++aCur == aEnd) goto OnClear; else cont = true; }
    while (bCur->y1() <= aCur->y0()) { if (++bCur == bEnd) goto OnClear; else cont = true; }

    if (cont)
      continue;

    while (aCur->x1() <= cx0 || aCur->x0() >= cx1) { if (++aCur == aEnd) goto OnClear; else cont = true; }
    while (bCur->x1() <= cx0 || bCur->x0() >= cx1) { if (++bCur == bEnd) goto OnClear; else cont = true; }

    if (!cont)
      break;
  }

  if (aCur->y0() >= cy1 || bCur->y0() >= cy1) {
OnClear:
    return dst.reset();
  }

  B2D_ASSERT(aCur != aEnd);
  B2D_ASSERT(bCur != bEnd);

  // Update aSize and bSize so the estimated size is closer to the result.
  aSize -= (size_t)(aCur - aI->data());
  bSize -= (size_t)(bCur - bI->data());

  // --------------------------------------------------------------------------
  // [Estimated Size]
  // --------------------------------------------------------------------------

  estimatedSize = (aSize + bSize) * 2;
  if (estimatedSize < aSize || estimatedSize < bSize)
    return DebugUtils::errored(kErrorNoMemory);

  // --------------------------------------------------------------------------
  // [Destination]
  // --------------------------------------------------------------------------

  requireNewData |= (estimatedSize > dstI->capacity());
  if (requireNewData) {
    dstI = bRegionAllocData(estimatedSize);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }

  dCur = dstI->data();

  // --------------------------------------------------------------------------
  // [Intersection]
  // --------------------------------------------------------------------------

  aBandEnd = bRegionGetEndBand(aCur, aEnd);
  bBandEnd = bRegionGetEndBand(bCur, bEnd);

  for (;;) {
    int ym = Math::min(aCur->y1(), bCur->y1());

    // Vertical intersection of current A and B bands.
    y0 = Math::max(aCur->y0(), Math::max(bCur->y0(), cy0));
    y1 = Math::min(cy1, ym);

    if (y0 < y1) {
      const IntBox* aBand = aCur;
      const IntBox* bBand = bCur;

      dCurBand = dCur;

      for (;;) {
        // Skip boxes which do not intersect.
        if (aBand->x1() <= bBand->x0()) { if (++aBand == aBandEnd) goto OnIntersectBandDone; else continue; }
        if (bBand->x1() <= aBand->x0()) { if (++bBand == bBandEnd) goto OnIntersectBandDone; else continue; }

        // Horizontal intersection of current A and B boxes.
        int x0 = Math::max(aBand->x0(),
                 Math::max(bBand->x0(), cx0));
        int xm = Math::min(aBand->x1(), bBand->x1());
        int x1 = Math::min(cx1, xm);

        if (x0 < x1) {
          B2D_ASSERT(dCur != dstI->data() + dstI->capacity());
          dCur->reset(x0, y0, x1, y1);
          dCur++;
        }

        // Advance.
        if (aBand->x1() == xm && (++aBand == aBandEnd || aBand->x0() >= cx1)) break;
        if (bBand->x1() == xm && (++bBand == bBandEnd || bBand->x0() >= cx1)) break;
      }

      // Update bounding box and coalesce.
OnIntersectBandDone:
      if (dCurBand != dCur) {
        if (dstBBoxX0 > dCurBand[0].x0()) dstBBoxX0 = dCurBand[0].x0();
        if (dstBBoxX1 < dCur   [-1].x1()) dstBBoxX1 = dCur   [-1].x1();

        if (dPrevBand)
          dCur = _bRegionCoalesce(dCur, &dPrevBand, &dCurBand, y1);
        else
          dPrevBand = dCurBand;
      }
    }

    // Advance A.
    if (aCur->y1() == ym) {
      if ((aCur = aBandEnd) == aEnd || aCur->y0() >= cy1)
        break;

      while (aCur->x1() <= cx0 || aCur->x0() >= cx1)
        if (++aCur == aEnd) goto OnEnd;

      aBandEnd = bRegionGetEndBand(aCur, aEnd);
    }

    // Advance B.
    if (bCur->y1() == ym) {
      if ((bCur = bBandEnd) == bEnd || bCur->y0() >= cy1)
        break;

      while (bCur->x1() <= cx0 || bCur->x0() >= cx1)
        if (++bCur == bEnd) goto OnEnd;

      bBandEnd = bRegionGetEndBand(bCur, bEnd);
    }
  }

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

OnEnd:
  dstI->_size = (size_t)(dCur - dstI->data());
  if (dstI->size() == 0)
    dstI->_boundingBox.reset();
  else
    dstI->_boundingBox.reset(dstBBoxX0, dstI->_data[0].y0(), dstBBoxX1, dCur[-1].y1());
  B2D_ASSERT(bRegionIsDataValid(dstI));

  if (requireNewData) {
    RegionImpl* oldI = dst.impl();
    dst._impl = dstI;
    oldI->release();
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::Region - Eq]
// ============================================================================

static bool B2D_CDECL Region_eq(const Region* a, const Region* b) noexcept {
  const RegionImpl* aI = a->impl();
  const RegionImpl* bI = b->impl();

  if (aI == bI)
    return true;

  size_t size = aI->size();
  if (size != bI->_size || aI->boundingBox() != bI->boundingBox())
    return false;

  const IntBox* aData = aI->data();
  const IntBox* bData = bI->data();

  for (size_t i = 0; i < size; i++)
    if (aData[i] != bData[i])
      return false;

  return true;
}

// ============================================================================
// [b2d::Region - Runtime Handlers]
// ============================================================================

#if B2D_MAYBE_SSE2
void RegionOnInitSSE2(RegionOps& ops) noexcept;
#endif

void RegionOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  Any::_initNone(Any::kTypeIdRegion            , const_cast<RegionImpl*>(&RegionImplNone));
  Any::_initNone(Any::kTypeIdRegionInfiniteSlot, const_cast<RegionImpl*>(&RegionImplInfinite));

  RegionOps& ops = _bRegionOps;

  #if !B2D_ARCH_SSE2
  ops.translate = Region_translate;
  ops.eq = Region_eq;
  #endif

  #if B2D_MAYBE_SSE2
  if (Runtime::hwInfo().hasSSE2()) RegionOnInitSSE2(ops);
  #endif
}

B2D_END_NAMESPACE
