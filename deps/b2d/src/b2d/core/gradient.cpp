// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/array.h"
#include "../core/gradient.h"
#include "../core/math.h"
#include "../core/pixelutils_p.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Gradient - Globals]
// ============================================================================

GradientOps _bGradientOps;
static Wrap<GradientImpl> GradientImplNone[Gradient::kTypeCount];

// ============================================================================
// [b2d::Gradient - Utilities]
// ============================================================================

static B2D_INLINE void Gradient_copyValues(GradientImpl* dstI, const GradientImpl* srcI, size_t size) noexcept {
  B2D_ASSERT(size <= dstI->capacity());

  dstI->_size = size;
  dstI->setExtend(srcI->extend());
  dstI->setMatrix(srcI->matrix());
  dstI->setMatrixType(srcI->matrixType());
  dstI->setVertex(0, srcI->vertex(0));
  dstI->setVertex(1, srcI->vertex(1));
  dstI->setVertex(2, srcI->vertex(2));
}

static B2D_INLINE GradientCache* Gradient_copyCache(GradientCache* cache) noexcept {
  return cache ? cache->addRef() : nullptr;
}

static B2D_INLINE void Gradient_copyStops(GradientStop* dst, const GradientStop* src, size_t size) noexcept {
  if (size)
    std::memcpy(dst, src, size * sizeof(GradientStop));
}

static void Gradient_sortStops(GradientStop* stops, size_t size) noexcept {
  // Insertion sort is used to keep the order of stops having the same offset.
  GradientStop* i = stops + 1;
  GradientStop* j = stops;
  GradientStop* end = stops + size;

  while (i < end) {
    while (j[0].offset() > j[1].offset()) {
      std::swap(j[0], j[1]);
      if (j == stops)
        break;
      j--;
    }

    j = i;
    i++;
  }
}

// ============================================================================
// [b2d::Gradient - Impl]
// ============================================================================

GradientImpl::GradientImpl(uint32_t objectInfo, size_t capacity) noexcept
  : ObjectImpl(objectInfo),
    _size(0),
    _capacity(capacity),
    _matrix(Matrix2D::identity()),
    _cache32(nullptr) {}

GradientImpl::~GradientImpl() noexcept {
  GradientCache* cache = _cache32.load(std::memory_order_relaxed);
  if (cache)
    cache->release();
}

// ============================================================================
// [b2d::Gradient - Cache]
// ============================================================================

GradientCache* GradientImpl::_ensureCache32() const noexcept {
  // Gradient should never need cache if doesn't have more than 1 stop.
  if (_size < 1)
    return nullptr;

  // TODO: Make the cache adaptive.
  uint32_t cacheSize = kGradientCacheSize;

  GradientCache* cache = GradientCache::alloc(cacheSize, 4);
  if (B2D_UNLIKELY(!cache))
    return nullptr;

  _bGradientOps.interpolate32(cache->data<uint32_t>(), cacheSize, _stops, _size);

  // We must drop this cache if some other thread created another one meanwhile.
  GradientCache* expected = nullptr;
  if (!_cache32.compare_exchange_strong(expected, cache)) {
    B2D_ASSERT(expected != nullptr);
    GradientCache::destroy(cache);
    cache = expected;
  }

  return cache;
}

// ============================================================================
// [b2d::Gradient - Construction / Destruction]
// ============================================================================

Gradient::Gradient(uint32_t typeId, double v0, double v1, double v2, double v3, double v4, double v5) noexcept
  : Object(GradientImpl::new_(typeId, Gradient::kInitSize)) {

  if (B2D_UNLIKELY(!impl())) {
    _impl = Any::noneByTypeId(typeId).impl();
  }
  else {
    GradientImpl* thisI = impl();
    thisI->setScalar(0, v0);
    thisI->setScalar(1, v1);
    thisI->setScalar(2, v2);
    thisI->setScalar(3, v3);
    thisI->setScalar(4, v4);
    thisI->setScalar(5, v5);
  }
}

// ============================================================================
// [b2d::Gradient - Reset]
// ============================================================================

Error Gradient::reset(uint32_t resetPolicy) noexcept {
  GradientImpl* thisI = impl();
  if (thisI->isShared() || resetPolicy == Globals::kResetHard)
    return AnyInternal::replaceImpl(this, Any::noneByTypeId(typeId()).impl());

  thisI->_destroyCache();
  thisI->_size = 0;
  thisI->_matrix.reset();

  thisI->setExtend(kExtendDefault);
  thisI->setMatrixType(Matrix2D::kTypeIdentity);
  thisI->setScalar(0, 0.0);
  thisI->setScalar(1, 0.0);
  thisI->setScalar(2, 0.0);
  thisI->setScalar(3, 0.0);
  thisI->setScalar(4, 0.0);
  thisI->setScalar(5, 0.0);

  return kErrorOk;
}

// ============================================================================
// [b2d::Gradient - Any Interface]
// ============================================================================

Error Gradient::_detach() noexcept {
  GradientImpl* thisI = impl();
  if (!thisI->isShared())
    return kErrorOk;

  size_t size = thisI->size();
  size_t capacity = Math::max<size_t>(size, Gradient::kInitSize);

  GradientImpl* newI = GradientImpl::new_(thisI->typeId(), capacity);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  newI->_capacity = capacity;
  newI->_cache32.store(Gradient_copyCache(thisI->cache32()), std::memory_order_relaxed);

  Gradient_copyValues(newI, thisI, size);
  Gradient_copyStops(newI->stops(), thisI->stops(), size);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Gradient - Container]
// ============================================================================

Error Gradient::reserve(size_t capacity) noexcept {
  if (B2D_UNLIKELY(capacity > bDataMaxSize(sizeof(GradientImpl), sizeof(GradientStop))))
    return DebugUtils::errored(kErrorNoMemory);

  GradientImpl* thisI = impl();
  size_t size = thisI->size();

  capacity = Math::max<size_t>(capacity, size);
  if (!isShared() && thisI->capacity() >= capacity)
    return kErrorOk;

  GradientImpl* newI = GradientImpl::new_(thisI->typeId(), capacity);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  newI->_capacity = capacity;
  newI->_cache32.store(Gradient_copyCache(thisI->cache32()), std::memory_order_relaxed);

  Gradient_copyValues(newI, thisI, size);
  Gradient_copyStops(newI->stops(), thisI->stops(), size);

  return AnyInternal::replaceImpl(this, newI);
}

Error Gradient::compact() noexcept {
  GradientImpl* thisI = impl();
  size_t size = thisI->size();

  if (thisI->isTemporary() || size == thisI->capacity())
    return kErrorOk;

  GradientImpl* newI = GradientImpl::new_(thisI->typeId(), size);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  newI->_capacity = size;
  newI->_cache32.store(Gradient_copyCache(thisI->cache32()), std::memory_order_relaxed);

  Gradient_copyValues(newI, thisI, size);
  Gradient_copyStops(newI->stops(), thisI->stops(), size);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Gradient - Setup]
// ============================================================================

Error Gradient::_setDeep(const Gradient& other) noexcept {
  GradientImpl* thisI = this->impl();
  GradientImpl* otherI = other.impl();

  if (thisI == otherI)
    return detach();

  size_t size = otherI->size();
  size_t capacity = Math::max<size_t>(size, Gradient::kInitSize);

  if (thisI->isShared() || size > thisI->capacity()) {
    GradientImpl* newI = GradientImpl::new_(otherI->typeId(), capacity);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    newI->_capacity = capacity;
    newI->_cache32.store(Gradient_copyCache(otherI->cache32()), std::memory_order_relaxed);

    Gradient_copyValues(newI, otherI, size);
    Gradient_copyStops(newI->stops(), otherI->stops(), size);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->_destroyCache();
    thisI->_cache32.store(Gradient_copyCache(otherI->cache32()), std::memory_order_relaxed);

    Gradient_copyValues(thisI, otherI, size);
    Gradient_copyStops(thisI->stops(), otherI->stops(), size);

    return kErrorOk;
  }
}

// ============================================================================
// [b2d::Gradient - Accessors]
// ============================================================================

Error Gradient::setExtend(uint32_t extend) noexcept {
  if (B2D_UNLIKELY(extend >= kExtendCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(detach());
  impl()->setExtend(extend);
  return kErrorOk;
}

Error Gradient::setScalar(uint32_t scalarId, double v) noexcept {
  if (B2D_UNLIKELY(scalarId >= kScalarIdCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(detach());
  impl()->setScalar(scalarId, v);
  return kErrorOk;
}

Error Gradient::setScalars(double v0, double v1, double v2, double v3, double v4, double v5) noexcept {
  B2D_PROPAGATE(detach());

  GradientImpl* thisI = impl();
  thisI->setScalar(0, v0);
  thisI->setScalar(1, v1);
  thisI->setScalar(2, v2);
  thisI->setScalar(3, v3);
  thisI->setScalar(4, v4);
  thisI->setScalar(5, v5);
  return kErrorOk;
}

Error Gradient::setVertex(uint32_t vertexId, double x, double y) noexcept {
  if (B2D_UNLIKELY(vertexId >= kVertexIdCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(detach());
  impl()->_vertex[vertexId].reset(x, y);
  return kErrorOk;
}

Error Gradient::resetStops() noexcept {
  if (impl()->_size == 0)
    return kErrorOk;

  B2D_PROPAGATE(detach());
  impl()->_destroyCache();
  impl()->_size = 0;
  return kErrorOk;
}

Error Gradient::setStops(const Array<GradientStop>& stops) noexcept {
  return setStops(stops.data(), stops.size());
}

Error Gradient::setStops(const GradientStop* stops, size_t size) noexcept {
  if (B2D_UNLIKELY(size == 0))
    return resetStops();

  GradientImpl* thisI = impl();
  if (thisI->isShared() || size > thisI->capacity()) {
    if (B2D_UNLIKELY(size >= Gradient::kMaxSize))
      return DebugUtils::errored(kErrorNoMemory);

    GradientImpl* newI = GradientImpl::new_(
      thisI->typeId(), Math::max<size_t>(size, Gradient::kInitSize));

    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    Gradient_copyValues(newI, thisI, size);
    Gradient_copyStops(newI->stops(), stops, size);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->_destroyCache();
    thisI->_size = size;

    Gradient_copyStops(thisI->stops(), stops, size);
    return kErrorOk;
  }
}

Error Gradient::setStop(double offset, Argb32 argb32, bool replace) noexcept {
  return setStop(GradientStop(offset, argb32), replace);
}

Error Gradient::setStop(double offset, Argb64 argb64, bool replace) noexcept {
  return setStop(GradientStop(offset, argb64), replace);
}

Error Gradient::setStop(const GradientStop& stop, bool replace) noexcept {
  if (!stop.isValid())
    return DebugUtils::errored(kErrorInvalidArgument);

  GradientImpl* thisI = impl();
  size_t i;
  size_t size = thisI->size();
  const GradientStop* stops = thisI->stops();

  double offset = stop.offset();
  Argb64 argb64 = stop.argb64();

  for (i = 0; i < size; i++)
    if (stops[i].offset() > offset)
      break;

  // Replace the existing stop.
  if (i > 0 && replace && stops[i - 1].offset() == offset) {
    B2D_PROPAGATE(detach());
    thisI = impl();
    thisI->_destroyCache();
    thisI->_stops[i - 1].setArgb64(argb64);
    return kErrorOk;
  }

  // Insert a new stop.
  if (thisI->isShared() || thisI->capacity() == size) {
    size_t capacity = bDataGrow(sizeof(GradientImpl), size, size + 1, sizeof(GradientStop));
    // TODO: Overflow...

    GradientImpl* newI = GradientImpl::new_(typeId(), capacity);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    Gradient_copyValues(newI, thisI, size + 1);
    Gradient_copyStops(newI->stops(), stops, i);

    newI->_stops[i].reset(offset, argb64);
    Gradient_copyStops(newI->stops() + i + 1, stops + i, size - i);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->_destroyCache();
    std::memmove(thisI->stops() + i + 1, thisI->stops() + i, (size - i) * sizeof(GradientStop));

    thisI->_stops[i].reset(offset, argb64);
    thisI->_size++;

    return kErrorOk;
  }
}

Error Gradient::removeStop(double offset, bool all) noexcept {
  if (offset < 0.0 || offset > 1.0)
    return DebugUtils::errored(kErrorInvalidArgument);

  GradientImpl* thisI = impl();
  size_t size = thisI->size();
  const GradientStop* stops = thisI->stops();

  for (size_t a = 0; a < size; a++) {
    if (stops[a].offset() > offset)
      break;

    if (stops[a].offset() == offset) {
      size_t b = a + 1;

      if (all) {
        while (b < size) {
          if (stops[b].offset() != offset)
            break;
          b++;
        }
      }
      return removeRange(Range(a, b));
    }
  }

  return DebugUtils::errored(kErrorNotFound);
}

Error Gradient::removeStop(const GradientStop& stop, bool all) noexcept {
  double offset = stop.offset();
  if (!(offset >= 0.0 && offset <= 1.0))
    return DebugUtils::errored(kErrorInvalidArgument);

  GradientImpl* thisI = impl();
  const GradientStop* stops = thisI->_stops;

  size_t a = 0;
  size_t size = thisI->size();

  for (;;) {
    if (a >= size || stops[a].offset() > offset)
      return DebugUtils::errored(kErrorNotFound);

    if (stops[a].offset() == offset)
      break;

    a++;
  }

  for (;;) {
    if (stops[a]._argb != stop._argb)
      break;

    if (++a >= size || stops[a].offset() != offset)
      return DebugUtils::errored(kErrorNotFound);
  }

  size_t b = a + 1;
  if (all) {
    while (b < size) {
      if (stops[b].offset() != offset || stops[b]._argb != stop._argb)
        break;
      b++;
    }
  }

  return removeRange(Range(a, b));
}

Error Gradient::removeIndex(size_t index) noexcept {
  return index < impl()->size() ? removeRange(Range(index, index + 1)) : kErrorOk;
}

Error Gradient::removeRange(const Range& range) noexcept {
  GradientImpl* thisI = impl();
  size_t size = thisI->size();

  size_t rEnd = Math::min(range.end(), size);
  size_t rIdx = range.start();

  if (rIdx >= rEnd)
    return kErrorOk;

  uint32_t rSize = uint32_t(rEnd - rIdx);
  if (thisI->isShared()) {
    size_t capacity = Math::max<size_t>(size - uint32_t(rSize), Gradient::kInitSize);
    GradientImpl* newI = GradientImpl::new_(type(), capacity);

    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    Gradient_copyValues(newI, thisI, size - rSize);
    Gradient_copyStops(newI->_stops, thisI->_stops, rIdx);
    Gradient_copyStops(newI->_stops + rIdx, thisI->_stops + rEnd, size - rEnd);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->_destroyCache();
    size_t itemsToMove = size - rEnd;

    if (itemsToMove)
      std::memmove(thisI->_stops + rIdx, thisI->_stops + rEnd, itemsToMove * sizeof(GradientStop));

    thisI->_size -= rSize;
    return kErrorOk;
  }
}

Error Gradient::removeRange(double tMin, double tMax) noexcept {
  if (tMax < tMin)
    return DebugUtils::errored(kErrorInvalidArgument);

  GradientImpl* thisI = impl();
  const GradientStop* stops = thisI->stops();

  size_t size = thisI->size();
  if (size == 0)
    return kErrorOk;

  size_t a, b;
  for (a = 0; a < size && stops[a].offset() <  tMin; a++) continue;
  for (b = a; b < size && stops[b].offset() <= tMax; b++) continue;

  return (a < b) ? removeRange(Range(a, b)) : Error(kErrorOk);
}

size_t Gradient::indexOf(double offset) const noexcept {
  if (offset < 0.0 || offset > 1.0)
    return Globals::kInvalidIndex;

  const GradientImpl* thisI = impl();
  const GradientStop* stops = thisI->stops();

  size_t size = thisI->size();
  for (size_t i = 0; i < size; i++) {
    if (stops[i].offset() > offset)
      break;

    if (stops[i].offset() == offset)
      return i;
  }

  return Globals::kInvalidIndex;
}

size_t Gradient::indexOf(const GradientStop& stop) const noexcept {
  double offset = stop.offset();

  if (!(offset >= 0.0 && offset <= 1.0))
    return Globals::kInvalidIndex;

  const GradientImpl* thisI = impl();
  const GradientStop* stops = thisI->stops();

  size_t size = thisI->size();
  for (size_t i = 0; i < size; i++) {
    if (stops[i].offset() < offset)
      continue;

    if (stops[i].offset() != offset)
      break;

    if (stops[i]._argb == stop._argb)
      return i;
  }

  return Globals::kInvalidIndex;
}

// ============================================================================
// [b2d::Gradient - Matrix]
// ============================================================================

Error Gradient::setMatrix(const Matrix2D& m) noexcept {
  B2D_PROPAGATE(detach());

  GradientImpl* thisI = impl();
  thisI->setMatrix(m);
  thisI->setMatrixType(m.type());
  return kErrorOk;
}

Error Gradient::resetMatrix() noexcept {
  if (impl()->matrixType() == Matrix2D::kTypeIdentity)
    return kErrorOk;

  B2D_PROPAGATE(detach());

  GradientImpl* thisI = impl();
  thisI->_matrix.reset();
  thisI->setMatrixType(Matrix2D::kTypeIdentity);
  return kErrorOk;
}

Error Gradient::_matrixOp(uint32_t op, const void* params) noexcept {
  B2D_PROPAGATE(detach());

  GradientImpl* thisI = impl();
  thisI->_matrix._matrixOp(op, params);
  thisI->setMatrixType(thisI->matrix().type());
  return kErrorOk;
}

// ============================================================================
// [b2d::Gradient - Equality]
// ============================================================================

bool B2D_CDECL Gradient::_eq(const Gradient* a, const Gradient* b) noexcept {
  GradientImpl* aImpl = a->impl();
  GradientImpl* bImpl = b->impl();

  if (aImpl == bImpl)
    return true;

  return aImpl->_commonData._extra32 == bImpl->_commonData._extra32 &&
         aImpl->size() == bImpl->size() &&
         aImpl->matrix() == bImpl->matrix() &&
         aImpl->scalar(0) == bImpl->scalar(0) &&
         aImpl->scalar(1) == bImpl->scalar(1) &&
         aImpl->scalar(2) == bImpl->scalar(2) &&
         aImpl->scalar(3) == bImpl->scalar(3) &&
         aImpl->scalar(4) == bImpl->scalar(4) &&
         aImpl->scalar(5) == bImpl->scalar(5) &&
         Support::inlinedEq(aImpl->_stops, bImpl->_stops, aImpl->size() * sizeof(GradientStop));
}

// ============================================================================
// [b2d::Gradient - Interpolate]
// ============================================================================

static void B2D_CDECL Gradient_interpolate32(uint32_t* dPtr, uint32_t dSize, const GradientStop* sPtr, size_t sSize) noexcept {
  B2D_ASSERT(dPtr != nullptr);
  B2D_ASSERT(dSize > 0);

  B2D_ASSERT(sPtr != nullptr);
  B2D_ASSERT(sSize > 0);

  uint32_t* dSpanPtr = dPtr;
  uint32_t i = dSize;

  uint32_t c0 = sPtr[0].argb32().value();
  uint32_t c1 = c0;

  uint32_t p0 = 0;
  uint32_t p1;

  size_t sIndex = 0;
  double fWidth = double(int32_t(--dSize) << 8);

  uint32_t cp = PixelUtils::prgb32_8888_from_argb32_8888(c0);
  uint32_t cpFirst = cp;

  if (sSize == 1)
    goto SolidLoop;

  do {
    c1 = sPtr[sIndex].argb32().value();
    p1 = uint32_t(Math::iround(sPtr[sIndex].offset() * fWidth));

    dSpanPtr = dPtr + (p0 >> 8);
    i = ((p1 >> 8) - (p0 >> 8));

    if (i == 0)
      c0 = c1;

    p0 = p1;
    i++;

SolidInit:
    cp = PixelUtils::prgb32_8888_from_argb32_8888(c0);
    if (c0 == c1) {
SolidLoop:
      do {
        dSpanPtr[0] = cp;
        dSpanPtr++;
      } while (--i);
    }
    else {
      dSpanPtr[0] = cp;
      dSpanPtr++;

      if (--i) {
        const uint32_t kShift = 23;
        const uint32_t kMask = 0xFFu << kShift;

        uint32_t rPos = (c0 <<  7) & kMask;
        uint32_t gPos = (c0 << 15) & kMask;
        uint32_t bPos = (c0 << 23) & kMask;

        uint32_t rInc = (c1 <<  7) & kMask;
        uint32_t gInc = (c1 << 15) & kMask;
        uint32_t bInc = (c1 << 23) & kMask;

        rInc = uint32_t(int32_t(rInc - rPos) / int32_t(i));
        gInc = uint32_t(int32_t(gInc - gPos) / int32_t(i));
        bInc = uint32_t(int32_t(bInc - bPos) / int32_t(i));

        rPos += 1u << (kShift - 1);
        gPos += 1u << (kShift - 1);
        bPos += 1u << (kShift - 1);

        if (ArgbUtil::isOpaque32(c0 & c1)) {
          // Both fully opaque, no need to premultiply.
          do {
            rPos += rInc;
            gPos += gInc;
            bPos += bInc;

            cp = 0xFF000000u + ((rPos & kMask) >>  7) +
                               ((gPos & kMask) >> 15) +
                               ((bPos & kMask) >> 23) ;

            dSpanPtr[0] = cp;
            dSpanPtr++;
          } while (--i);
        }
        else {
          // One or both having alpha, have to be premultiplied.
          uint32_t aPos = (c0 >> 1) & kMask;
          uint32_t aInc = (c1 >> 1) & kMask;

          aInc = uint32_t(int32_t(aInc - aPos) / int32_t(i));
          aPos += 1u << (kShift - 1);

          do {
            uint32_t _a, _g;

            aPos += aInc;
            rPos += rInc;
            gPos += gInc;
            bPos += bInc;

            cp = ((bPos & kMask) >> 23) +
                 ((rPos & kMask) >>  7);
            _a = ((aPos & kMask) >> 23);
            _g = ((gPos & kMask) >> 15);

            cp *= _a;
            _g *= _a;
            _a <<= 24;

            cp += 0x00800080u;
            _g += 0x00008000u;

            cp = (cp + ((cp >> 8) & 0x00FF00FFu));
            _g = (_g + ((_g >> 8) & 0x0000FF00u));

            cp &= 0xFF00FF00u;
            _g &= 0x00FF0000u;

            cp += _g;
            cp >>= 8;
            cp += _a;

            dSpanPtr[0] = cp;
            dSpanPtr++;
          } while (--i);
        }
      }

      c0 = c1;
    }
  } while (++sIndex < sSize);

  // The last stop doesn't have to end at 1.0, in such case the remaining space
  // is filled by the last color stop (premultiplied). We jump to the main loop
  // instead of filling the buffer here.
  i = uint32_t((size_t)((dPtr + dSize + 1) - dSpanPtr));
  if (i != 0)
    goto SolidInit;

  // The first pixel has to be always set to the first stop's color. The main
  // loop always honors the last color value of the stop colliding with the
  // previous offset index - for example if multiple stops have the same offset
  // [0.0] the first pixel will be the last stop's color. This is easier to fix
  // here as we don't need extra conditions in the main loop.
  dPtr[0] = cpFirst;
}

// ============================================================================
// [b2d::Gradient - Runtime Handlers]
// ============================================================================

#if B2D_MAYBE_SSE2
void GradientOnInitSSE2(GradientOps& ops) noexcept;
#endif

#if B2D_MAYBE_AVX2
void GradientOnInitAVX2(GradientOps& ops) noexcept;
#endif

void GradientOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  for (uint32_t i = 0; i < Gradient::kTypeCount; i++) {
    uint32_t typeId = Gradient::typeIdFromGradientType(i);

    GradientImpl* gradientI = new (&GradientImplNone[i]) GradientImpl(typeId, 0);
    gradientI->_commonData._setToBuiltInNone();
    Any::_initNone(typeId, gradientI);
  }

  GradientOps& ops = _bGradientOps;

  #if !B2D_ARCH_SSE2
  ops.interpolate32 = Gradient_interpolate32;
  #endif

  #if B2D_MAYBE_SSE2
  if (Runtime::hwInfo().hasSSE2()) GradientOnInitSSE2(ops);
  #endif

  #if B2D_MAYBE_AVX2
  if (Runtime::hwInfo().hasAVX2()) GradientOnInitAVX2(ops);
  #endif
}

// ============================================================================
// [b2d::Gradient - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_gradient_linear) {
  LinearGradient g(0.0, 0.5, 1.0, 1.5);

  EXPECT(g.type() == Gradient::kTypeLinear);
  EXPECT(g.typeId() == Any::kTypeIdLinearGradient);

  EXPECT(g.x0() == 0.0);
  EXPECT(g.y0() == 0.5);
  EXPECT(g.x1() == 1.0);
  EXPECT(g.y1() == 1.5);

  g.setStart(0.15, 0.85);
  g.setEnd(0.75, 0.25);

  EXPECT(g.x0() == 0.15);
  EXPECT(g.y0() == 0.85);
  EXPECT(g.x1() == 0.75);
  EXPECT(g.y1() == 0.25);
}

UNIT(b2d_core_gradient_radial) {
  RadialGradient g(1.0, 1.5, 0.0, 0.5, 0.25);

  EXPECT(g.type() == Gradient::kTypeRadial);
  EXPECT(g.typeId() == Any::kTypeIdRadialGradient);

  EXPECT(g.cx() == 1.0);
  EXPECT(g.cy() == 1.5);
  EXPECT(g.fx() == 0.0);
  EXPECT(g.fy() == 0.5);
  EXPECT(g.cr() == 0.25);
}

UNIT(b2d_core_gradient_conical) {
  ConicalGradient g(1.0, 1.5, 0.1);

  EXPECT(g.type() == Gradient::kTypeConical);
  EXPECT(g.typeId() == Any::kTypeIdConicalGradient);

  EXPECT(g.cx() == 1.0);
  EXPECT(g.cy() == 1.5);
  EXPECT(g.angle() == 0.1);
}

UNIT(b2d_core_gradient_stops) {
  // Gradient type doesn't matter here.
  LinearGradient g;

  g.addStop(0.0, Argb32(0x00000000));
  EXPECT(g.stopCount() == 1);
  EXPECT(g.stopAt(0).argb32().value() == 0x00000000);

  g.addStop(1.0, Argb32(0xFF000000));
  EXPECT(g.stopCount() == 2);
  EXPECT(g.stopAt(1).argb32().value() == 0xFF000000);

  g.addStop(0.5, Argb32(0xFFFF0000));
  EXPECT(g.stopCount() == 3);
  EXPECT(g.stopAt(1).argb32().value() == 0xFFFF0000);

  g.addStop(0.5, Argb32(0xFFFFFF00));
  EXPECT(g.stopCount() == 4);
  EXPECT(g.stopAt(2).argb32().value() == 0xFFFFFF00);

  g.removeStop(0.5, true);
  EXPECT(g.stopCount() == 2);
  EXPECT(g.stopAt(0).argb32().value() == 0x00000000);
  EXPECT(g.stopAt(1).argb32().value() == 0xFF000000);

  g.addStop(0.5, Argb32(0x80000000));
  EXPECT(g.stopCount() == 3);
  EXPECT(g.stopAt(1).argb32().value() == 0x80000000);

  // Check whether the detach functionality works as expected.
  LinearGradient copy(g);
  EXPECT(copy.stopCount() == 3);

  g.addStop(0.5, Argb32(0xCC000000));
  EXPECT(copy.stopCount() == 3);
  EXPECT(g.stopCount() == 4);
  EXPECT(g.stopAt(0).argb32().value() == 0x00000000);
  EXPECT(g.stopAt(1).argb32().value() == 0x80000000);
  EXPECT(g.stopAt(2).argb32().value() == 0xCC000000);
  EXPECT(g.stopAt(3).argb32().value() == 0xFF000000);

  g.resetStops();
  EXPECT(g.stopCount() == 0);
}

UNIT(b2d_core_gradient_interpolate32) {
  uint32_t buffer[260];
  uint32_t i;
  GradientStop stops[4];

  // Buffer with 4 extra pixels extra for detecting possible overflow. SSE2
  // implementation can write 4 pixels at time so check if the code is safe.
  uint32_t kBoMark = 0xDEADBEEF;
  buffer[256] = kBoMark;
  buffer[257] = kBoMark;
  buffer[258] = kBoMark;
  buffer[259] = kBoMark;

  // Solid.
  stops[0] = GradientStop(0.0, Argb32(0xAABBCCDD));

  std::memset(buffer, 0, 256 * sizeof(uint32_t));
  _bGradientOps.interpolate32(buffer, 256, stops, 1);

  EXPECT(buffer[259] == kBoMark, "Buffer overrun");
  EXPECT(buffer[258] == kBoMark, "Buffer overrun");
  EXPECT(buffer[257] == kBoMark, "Buffer overrun");
  EXPECT(buffer[256] == kBoMark, "Buffer overrun");

  for (i = 0; i < 256; i++) {
    uint32_t expected = PixelUtils::prgb32_8888_from_argb32_8888(0xAABBCCDD);
    EXPECT(buffer[i] == expected,
      "Solid fill doesn't match [%08X != %08X]", buffer[i], expected);
  }

  // Small - 2/3 pixels, 2 stops.
  stops[0] = GradientStop(0.0, Argb32(0x00000000));
  stops[1] = GradientStop(1.0, Argb32(0xFFFFFFFF));

  std::memset(buffer, 0, 256 * sizeof(uint32_t));
  _bGradientOps.interpolate32(buffer, 2, stops, 2);

  EXPECT(buffer[0] == PixelUtils::prgb32_8888_from_argb32_8888(0x00000000));
  EXPECT(buffer[1] == PixelUtils::prgb32_8888_from_argb32_8888(0xFFFFFFFF));
  EXPECT(buffer[2] == 0, "Buffer overrun");

  std::memset(buffer, 0, 256 * sizeof(uint32_t));
  _bGradientOps.interpolate32(buffer, 3, stops, 2);

  EXPECT(buffer[0] == PixelUtils::prgb32_8888_from_argb32_8888(0x00000000));
  EXPECT(buffer[1] == PixelUtils::prgb32_8888_from_argb32_8888(0x80808080));
  EXPECT(buffer[2] == PixelUtils::prgb32_8888_from_argb32_8888(0xFFFFFFFF));
  EXPECT(buffer[3] == 0, "Buffer overrun");

  // Small - 2/3 pixels, 3 stops.
  stops[0] = GradientStop(0.0, Argb32(0x00000000));
  stops[1] = GradientStop(0.5, Argb32(0xFFFF0000));
  stops[2] = GradientStop(1.0, Argb32(0xFFFFFFFF));

  std::memset(buffer, 0, 256 * sizeof(uint32_t));
  _bGradientOps.interpolate32(buffer, 2, stops, 3);

  EXPECT(buffer[0] == PixelUtils::prgb32_8888_from_argb32_8888(0x00000000));
  EXPECT(buffer[1] == PixelUtils::prgb32_8888_from_argb32_8888(0xFFFFFFFF));
  EXPECT(buffer[2] == 0, "Buffer overrun");

  std::memset(buffer, 0, 256 * sizeof(uint32_t));
  _bGradientOps.interpolate32(buffer, 3, stops, 3);

  EXPECT(buffer[0] == PixelUtils::prgb32_8888_from_argb32_8888(0x00000000));
  EXPECT(buffer[1] == PixelUtils::prgb32_8888_from_argb32_8888(0xFFFF0000));
  EXPECT(buffer[2] == PixelUtils::prgb32_8888_from_argb32_8888(0xFFFFFFFF));
  EXPECT(buffer[3] == 0, "Buffer overrun");

  // Interpolate.
  uint32_t a = 1;
  uint32_t r = 0;
  uint32_t g = 0;
  uint32_t b = 0;

  for (uint32_t a = 0; a < 4; a++) {
    for (uint32_t r = 0; r < 4; r++) {
      for (uint32_t g = 0; g < 4; g++) {
        for (uint32_t b = 0; b < 4; b++) {
          Argb32 c0((a & 0x1) ? 0xFF : 0, (r & 0x1) ? 0xFF : 0, (g & 0x1) ? 0xFF : 0, (b & 0x1) ? 0xFF : 0);
          Argb32 c1((a & 0x2) ? 0xFF : 0, (r & 0x2) ? 0xFF : 0, (g & 0x2) ? 0xFF : 0, (b & 0x2) ? 0xFF : 0);
          stops[0] = GradientStop(0.0, c0);
          stops[1] = GradientStop(1.0, c1);

          std::memset(buffer, 0, 256 * sizeof(uint32_t));
          _bGradientOps.interpolate32(buffer, 256, stops, 2);

          EXPECT(buffer[259] == kBoMark, "Buffer overrun");
          EXPECT(buffer[258] == kBoMark, "Buffer overrun");
          EXPECT(buffer[257] == kBoMark, "Buffer overrun");
          EXPECT(buffer[256] == kBoMark, "Buffer overrun");

          int32_t aVal = (int32_t(stops[0].argb32().a()));
          int32_t rVal = (int32_t(stops[0].argb32().r()));
          int32_t gVal = (int32_t(stops[0].argb32().g()));
          int32_t bVal = (int32_t(stops[0].argb32().b()));

          int32_t aInc = (int32_t(stops[1].argb32().a()) - aVal) / 255;
          int32_t rInc = (int32_t(stops[1].argb32().r()) - rVal) / 255;
          int32_t gInc = (int32_t(stops[1].argb32().g()) - gVal) / 255;
          int32_t bInc = (int32_t(stops[1].argb32().b()) - bVal) / 255;

          for (i = 0; i < 256; i++) {
            uint32_t result = buffer[i];
            uint32_t expected = PixelUtils::prgb32_8888_from_argb32_8888(
              Argb32(uint32_t(aVal), uint32_t(rVal),
                     uint32_t(gVal), uint32_t(bVal)).value());

            EXPECT(result == expected,
              "Invalid Pixel at [#%08X -> #%08X] at [%u] {#%08X (out) != #%08X (exp)}",
              c0.value(), c1.value(), i, result, expected);

            aVal += aInc;
            rVal += rInc;
            gVal += gInc;
            bVal += bInc;
          }
        }
      }
    }
  }
}
#endif

B2D_END_NAMESPACE
