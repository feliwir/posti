// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/geom2d.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/region.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Path2D - Null]
// ============================================================================

static Wrap<Path2DImpl> gPath2DImplNone;

// ============================================================================
// [b2d::Path2D - Data - UpdatePointers]
// ============================================================================

static B2D_INLINE void _bPathImplUpdate(Path2DImpl* d, size_t capacity) noexcept {
  d->_vertexData = reinterpret_cast<Point*>(Support::alignUp<uint8_t*>(d->commandData() + capacity, 16));
}

// ============================================================================
// [b2d::Path2D - Data - Copy / Move]
// ============================================================================

static B2D_INLINE void Path2D_copyCommandData(uint8_t* dst, const uint8_t* src, size_t size) noexcept {
  std::memcpy(dst, src, size);
}

static B2D_INLINE void Path2D_moveCommandData(uint8_t* dst, const uint8_t* src, size_t size) noexcept {
  std::memmove(dst, src, size);
}

static B2D_INLINE void Path2D_copyVertexData(Point* dst, const Point* src, size_t size) noexcept {
  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
}

static B2D_INLINE void Path2D_moveVertexData(Point* dst, const Point* src, size_t size) noexcept {
  std::memmove(dst, src, size * sizeof(Point));
}

// ============================================================================
// [b2d::Path2D - Data - Realloc]
// ============================================================================

static Path2DImpl* _bPathImplRealloc(Path2DImpl* impl, size_t capacity) noexcept {
  size_t size = impl->size();
  B2D_ASSERT(size <= capacity);

  Path2DImpl* newI = Path2DImpl::new_(capacity);
  if (B2D_UNLIKELY(!newI))
    return nullptr;

  newI->_size = size;
  newI->_pathInfo = impl->_pathInfo;

  if (size) {
    Path2D_copyCommandData(newI->commandData(), impl->commandData(), size);
    Path2D_copyVertexData(newI->vertexData(), impl->vertexData(), size);
  }

  impl->release();
  return newI;
}

// ============================================================================
// [b2d::Path2D - Data - Copy]
// ============================================================================

static Path2DImpl* _bPathImplCopy(Path2DImpl* impl) noexcept {
  size_t size = impl->size();
  if (size == 0)
    return &gPath2DImplNone;

  Path2DImpl* newI = Path2DImpl::new_(size);
  if (B2D_UNLIKELY(!newI))
    return nullptr;

  newI->_size = size;
  newI->_pathInfo = impl->pathInfo();

  if (size) {
    Path2D_copyCommandData(newI->commandData(), impl->commandData(), size);
    Path2D_copyVertexData(newI->vertexData(), impl->vertexData(), size);
  }

  return newI;
}

// ============================================================================
// [b2d::Path2D - Macros]
// ============================================================================

#define PATH_ADD_DATA_BEGIN(COUNT, DIRTY_FLAGS)                               \
  Path2DImpl* thisI = impl();                                                 \
                                                                              \
  size_t _size = thisI->size();                                               \
  size_t _remain = thisI->capacity() - _size;                                 \
                                                                              \
  if (B2D_UNLIKELY(_size == 0))                                               \
    return DebugUtils::errored(kErrorPathNoVertex);                           \
                                                                              \
  if (B2D_UNLIKELY(thisI->commandData()[_size - 1] == kCmdClose))             \
    return DebugUtils::errored(kErrorPathNoVertex);                           \
                                                                              \
  if (B2D_UNLIKELY(impl()->isShared() || COUNT > _remain)) {                  \
    if (B2D_UNLIKELY(_add(COUNT, DIRTY_FLAGS) == Globals::kInvalidIndex))     \
      return DebugUtils::errored(kErrorNoMemory);                             \
    thisI = impl();                                                           \
  }                                                                           \
  else {                                                                      \
    thisI->_size += COUNT;                                                    \
    thisI->pathInfo().makeDirty(DIRTY_FLAGS);                                 \
  }                                                                           \
                                                                              \
  uint8_t* cmdData = thisI->commandData() + _size;                            \
  Point* vtxData = thisI->vertexData() + _size;

#define PATH_ADD_DATA_END()                                                   \
  return kErrorOk;

// ============================================================================
// [b2d::Path2D - Utils]
// ============================================================================

static B2D_INLINE Error _bPathGetLastVertex(const Path2D* self, Point& dst) noexcept {
  Path2DImpl* thisI = self->impl();
  size_t last = thisI->size();

  if (!last)
    return DebugUtils::errored(kErrorPathNoRelative);

  const uint8_t* cmdData = thisI->commandData();
  uint32_t c = cmdData[--last];

  dst = thisI->vertexData()[last];
  if (c != Path2D::kCmdClose)
    return kErrorOk;

  while (last) {
    if (cmdData[--last] == Path2D::kCmdMoveTo) {
      dst = thisI->vertexData()[last];
      return kErrorOk;
    }
  }

  return DebugUtils::errored(kErrorPathNoRelative);
}

// ============================================================================
// [b2d::Path2D - Container]
// ============================================================================

Error Path2D::_detach() noexcept {
  if (!isShared())
    return kErrorOk;

  Path2DImpl* newI = _bPathImplCopy(impl());
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

Error Path2D::reserve(size_t capacity) noexcept {
  Path2DImpl* thisI = impl();
  if (!thisI->isShared() && thisI->capacity() >= capacity)
    return kErrorOk;

  size_t size = thisI->size();
  capacity = Math::max(capacity, size);

  Path2DImpl* newI = Path2DImpl::new_(capacity);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  newI->_size = size;
  newI->_pathInfo = thisI->pathInfo();

  if (size) {
    Path2D_copyCommandData(newI->commandData(), thisI->commandData(), size);
    Path2D_copyVertexData(newI->vertexData(), thisI->vertexData(), size);
  }

  return AnyInternal::replaceImpl(this, newI);
}

Error Path2D::compact() noexcept {
  Path2DImpl* thisI = impl();

  if (thisI->isTemporary() || thisI->size() == thisI->capacity())
    return kErrorOk;

  if (thisI->isShared()) {
    Path2DImpl* newI = _bPathImplCopy(thisI);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    Path2DImpl* newI = _bPathImplRealloc(thisI, thisI->size());
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    _impl = newI;
    return kErrorOk;
  }
}

size_t Path2D::_prepare(uint32_t op, size_t count, uint32_t dirtyFlags) noexcept {
  Path2DImpl* thisI = impl();

  size_t start = (op == kContainerOpReplace) ? size_t(0) : thisI->size();
  size_t remain = thisI->capacity() - start;

  if (thisI->isShared() || count > remain) {
    size_t optimalCapacity = bDataGrow(sizeof(Path2DImpl), start, start + count, sizeof(Point) + sizeof(uint8_t));
    Path2DImpl* newI = Path2DImpl::new_(optimalCapacity);

    if (B2D_UNLIKELY(!newI))
      return Globals::kInvalidIndex;

    newI->_size = start + count;
    newI->pathInfo().makeDirty(dirtyFlags);

    if (start) {
      Path2D_copyCommandData(newI->commandData(), thisI->commandData(), start);
      Path2D_copyVertexData(newI->vertexData(), thisI->vertexData(), start);
    }

    AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->_size = start + count;
    thisI->pathInfo().makeDirty(dirtyFlags);
  }

  return start;
}

size_t Path2D::_add(size_t count, uint32_t dirtyFlags) noexcept {
  Path2DImpl* thisI = impl();

  size_t tSize = thisI->size();
  size_t tFree = thisI->capacity() - tSize;

  if (thisI->isShared() || count > tFree) {
    Support::OverflowFlag of;
    size_t zSize = Support::safeAdd<size_t>(tSize, count, &of);

    if (B2D_UNLIKELY(of))
      return Globals::kInvalidIndex;

    size_t optimalCapacity = bDataGrow(sizeof(Path2DImpl), tSize, zSize, sizeof(Point) + sizeof(uint8_t));
    Path2DImpl* newI = Path2DImpl::new_(optimalCapacity);

    if (B2D_UNLIKELY(!newI))
      return Globals::kInvalidIndex;

    newI->_size = zSize;
    newI->pathInfo().makeDirty(dirtyFlags);

    if (tSize) {
      Path2D_copyCommandData(newI->commandData(), thisI->commandData(), tSize);
      Path2D_copyVertexData(newI->vertexData(), thisI->vertexData(), tSize);
    }

    AnyInternal::replaceImpl(this, newI);
    return tSize;
  }
  else {
    thisI->_size += count;
    thisI->pathInfo().makeDirty(dirtyFlags);
    return tSize;
  }
}

size_t Path2D::_updateAndGrow(uint8_t* cmdEnd, size_t n) noexcept {
  Path2DImpl* thisI = impl();

  B2D_ASSERT(!thisI->isShared());
  B2D_ASSERT(cmdEnd >= thisI->commandData() && cmdEnd <= thisI->commandData() + thisI->capacity());

  thisI->_size = (size_t)(cmdEnd - thisI->commandData());
  return _add(n, 0);
}

// ============================================================================
// [b2d::Path2D - Clear / Reset]
// ============================================================================

Error Path2D::reset(uint32_t resetPolicy) noexcept {
  Path2DImpl* thisI = impl();

  if (thisI->isShared() || resetPolicy == Globals::kResetHard)
    return AnyInternal::replaceImpl(this, none().impl());

  thisI->_size = 0;
  thisI->_pathInfo._flagsAtomic.store(Path2DInfo::kInitialFlags, std::memory_order_relaxed);

  return kErrorOk;
}

// ============================================================================
// [b2d::Path2D - Set]
// ============================================================================

Error Path2D::setPath(const Path2D& other) noexcept {
  return AnyInternal::replaceImpl(this, other.impl()->addRef());
}

Error Path2D::setDeep(const Path2D& other) noexcept {
  Path2DImpl* thisI = this->impl();
  Path2DImpl* otherI = other.impl();

  if (thisI == otherI)
    return detach();

  size_t size = otherI->_size;
  if (size == 0)
    return reset();

  B2D_PROPAGATE(reserve(size));

  thisI = this->impl();
  thisI->_size = size;
  thisI->_pathInfo = other.pathInfo();

  Path2D_copyCommandData(thisI->commandData(), otherI->commandData(), size);
  Path2D_copyVertexData(thisI->vertexData(), otherI->vertexData(), size);

  return kErrorOk;
}

Point Path2D::lastVertex() const noexcept {
  Point p;
  if (_bPathGetLastVertex(this, p) != kErrorOk)
    p.resetToNaN();
  return p;
}

Error Path2D::_setAt(size_t index, const Point& p) noexcept {
  Path2DImpl* thisI = impl();
  if (index >= thisI->size())
    return DebugUtils::errored(kErrorInvalidArgument);

  if (thisI->commandData()[index] == kCmdClose)
    return kErrorOk;

  B2D_PROPAGATE(detach());
  thisI = impl();

  thisI->vertexData()[index] = p;
  thisI->pathInfo().makeDirty(Path2DInfo::kCommandFlags);

  return kErrorOk;
}

// ============================================================================
// [b2d::Path2D - SubPath]
// ============================================================================

Error Path2D::subpathRange(size_t index, Range& dst) const noexcept {
  size_t size = impl()->size();

  if (index >= size) {
    dst.reset(Globals::kInvalidIndex, Globals::kInvalidIndex);
    return DebugUtils::errored(kErrorInvalidArgument);
  }

  size_t i = index + 1;
  const uint8_t* cmdData = impl()->commandData();

  while (i < size) {
    uint32_t cmd = cmdData[i];
    if (cmd == kCmdMoveTo)
      break;

    i++;
    if (cmd == kCmdClose)
      break;
  }

  dst.reset(index, i);
  return kErrorOk;
}

// ============================================================================
// [b2d::Path2D - MoveTo]
// ============================================================================

Error Path2D::moveTo(double x0, double y0) noexcept {
  size_t pos = _add(1, Path2DInfo::kGeometryFlags);
  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  Path2DImpl* thisI = impl();
  thisI->_commandData[pos] = kCmdMoveTo;
  thisI->_vertexData[pos].reset(x0, y0);

  return kErrorOk;
}

Error Path2D::moveToRel(double x0, double y0) noexcept {
  Point offset;
  B2D_PROPAGATE(_bPathGetLastVertex(this, offset));
  return moveTo(x0 + offset.x(), y0 + offset.y());
}

// ============================================================================
// [b2d::Path2D - LineTo]
// ============================================================================

Error Path2D::lineTo(double x1, double y1) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(x1, y1);
  PATH_ADD_DATA_END()
}

Error Path2D::lineToRel(double x1, double y1) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    Point offset(vtxData[-1]);
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(x1 + offset.x(), y1 + offset.y());
  PATH_ADD_DATA_END()
}

Error Path2D::hlineTo(double x) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(x, vtxData[-1].y());
  PATH_ADD_DATA_END()
}

Error Path2D::hlineToRel(double x) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(x + vtxData[-1].x(), vtxData[-1].y());
  PATH_ADD_DATA_END()
}

Error Path2D::vlineTo(double y) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(vtxData[-1].x(), y);
  PATH_ADD_DATA_END()
}

Error Path2D::vlineToRel(double y) noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kGeometryFlags)
    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(vtxData[-1].x(), y + vtxData[-1].y());
  PATH_ADD_DATA_END()
}

// ============================================================================
// [b2d::Path2D - PolyTo]
// ============================================================================

Error Path2D::polyTo(const Point* poly, size_t count) noexcept {
  if (B2D_UNLIKELY(count == 0))
    return kErrorOk;

  B2D_ASSERT(poly != nullptr);
  PATH_ADD_DATA_BEGIN(count, Path2DInfo::kGeometryFlags)
    size_t i;

    for (i = 0; i < count; i++)
      cmdData[i] = kCmdLineTo;

    for (i = 0; i < count; i++)
      vtxData[i] = poly[i];
  PATH_ADD_DATA_END()
}

Error Path2D::polyToRel(const Point* poly, size_t count) noexcept {
  if (B2D_UNLIKELY(count == 0))
    return kErrorOk;

  B2D_ASSERT(poly != nullptr);
  PATH_ADD_DATA_BEGIN(count, Path2DInfo::kGeometryFlags)
    size_t i;

    double dx = vtxData[-1].x();
    double dy = vtxData[-1].y();

    for (i = 0; i < count; i++)
      cmdData[i] = kCmdLineTo;

    for (i = 0; i < count; i++)
      vtxData[i].reset(poly[i].x() + dx, poly[i].y() + dy);
  PATH_ADD_DATA_END()
}

// ============================================================================
// [b2d::Path2D - QuadTo]
// ============================================================================

Error Path2D::quadTo(double x1, double y1, double x2, double y2) noexcept {
  PATH_ADD_DATA_BEGIN(2, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandQuadTo)
    cmdData[0] = kCmdQuadTo;
    cmdData[1] = kCmdControl;

    vtxData[0].reset(x1, y1);
    vtxData[1].reset(x2, y2);
  PATH_ADD_DATA_END()
}

Error Path2D::quadToRel(double x1, double y1, double x2, double y2) noexcept {
  PATH_ADD_DATA_BEGIN(2, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandQuadTo)
    double dx = vtxData[-1].x();
    double dy = vtxData[-1].y();

    cmdData[0] = kCmdQuadTo;
    cmdData[1] = kCmdControl;

    vtxData[0].reset(x1 + dx, y1 + dy);
    vtxData[1].reset(x2 + dx, y2 + dy);
  PATH_ADD_DATA_END()
}

Error Path2D::smoothQuadTo(double x2, double y2) noexcept {
  PATH_ADD_DATA_BEGIN(2, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandQuadTo)
    double x1 = vtxData[-1].x();
    double y1 = vtxData[-1].y();

    cmdData[0] = kCmdQuadTo;
    cmdData[1] = kCmdControl;

    if (_size >= 2 && cmdData[-2] == kCmdQuadTo) {
      x1 += x1 - vtxData[-2].x();
      y1 += y1 - vtxData[-2].y();
    }

    vtxData[0].reset(x1, y1);
    vtxData[1].reset(x2, y2);
  PATH_ADD_DATA_END()
}

Error Path2D::smoothQuadToRel(double x2, double y2) noexcept {
  PATH_ADD_DATA_BEGIN(2, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandQuadTo)
    double x1 = vtxData[-1].x();
    double y1 = vtxData[-1].y();

    cmdData[0] = kCmdQuadTo;
    cmdData[1] = kCmdControl;
    vtxData[1].reset(x2 + x1, y2 + y1);

    if (_size >= 2 && cmdData[-2] == kCmdQuadTo) {
      x1 += x1 - vtxData[-2].x();
      y1 += y1 - vtxData[-2].y();
    }

    vtxData[0].reset(x1, y1);
  PATH_ADD_DATA_END()
}

// ============================================================================
// [b2d::Path2D - CubicTo]
// ============================================================================

Error Path2D::cubicTo(double x1, double y1, double x2, double y2, double x3, double y3) noexcept {
  PATH_ADD_DATA_BEGIN(3, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandCubicTo)
    cmdData[0] = kCmdCubicTo;
    cmdData[1] = kCmdControl;
    cmdData[2] = kCmdControl;

    vtxData[0].reset(x1, y1);
    vtxData[1].reset(x2, y2);
    vtxData[2].reset(x3, y3);
  PATH_ADD_DATA_END()
}

Error Path2D::cubicToRel(double x1, double y1, double x2, double y2, double x3, double y3) noexcept {
  PATH_ADD_DATA_BEGIN(3, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandCubicTo)
    double dx = vtxData[-1].x();
    double dy = vtxData[-1].y();

    cmdData[0] = kCmdCubicTo;
    cmdData[1] = kCmdControl;
    cmdData[2] = kCmdControl;

    vtxData[0].reset(x1 + dx, y1 + dy);
    vtxData[1].reset(x2 + dx, y2 + dy);
    vtxData[2].reset(x3 + dx, y3 + dy);
  PATH_ADD_DATA_END()
}

Error Path2D::smoothCubicTo(double x2, double y2, double x3, double y3) noexcept {
  PATH_ADD_DATA_BEGIN(3, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandCubicTo)
    double x1 = vtxData[-1].x();
    double y1 = vtxData[-1].y();

    cmdData[0] = kCmdCubicTo;
    cmdData[1] = kCmdControl;
    cmdData[2] = kCmdControl;

    if (_size >= 3 && cmdData[-3] == kCmdCubicTo) {
      x1 += x1 - vtxData[-2].x();
      y1 += y1 - vtxData[-2].y();
    }

    vtxData[0].reset(x1, y1);
    vtxData[1].reset(x2, y2);
    vtxData[2].reset(x3, y3);
  PATH_ADD_DATA_END()
}

Error Path2D::smoothCubicToRel(double x2, double y2, double x3, double y3) noexcept {
  PATH_ADD_DATA_BEGIN(3, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandCubicTo)
    double x1 = vtxData[-1].x();
    double y1 = vtxData[-1].y();

    cmdData[0] = kCmdCubicTo;
    cmdData[1] = kCmdControl;
    cmdData[2] = kCmdControl;

    vtxData[1].reset(x2 + x1, y2 + y1);
    vtxData[2].reset(x3 + x1, y3 + y1);

    if (_size >= 3 && cmdData[-3] == kCmdCubicTo) {
      x1 += x1 - vtxData[-2].x();
      y1 += y1 - vtxData[-2].y();
    }
    vtxData[0].reset(x1, y1);
  PATH_ADD_DATA_END()
}

// ============================================================================
// [b2d::Path2D - ArcTo]
// ============================================================================

Error Path2D::arcTo(const Point& cp, const Point& rp, double start, double sweep, bool startPath) noexcept {
  uint8_t initialCmd = startPath ? kCmdMoveTo : kCmdLineTo;

  // Degenerated.
  if (Math::isNearZero(sweep)) {
    size_t pos = _add(2, Path2DInfo::kGeometryFlags);
    if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
      return DebugUtils::errored(kErrorNoMemory);

    Path2DImpl* thisI = impl();
    uint8_t* cmdData = thisI->commandData() + pos;
    Point* vtxData = thisI->vertexData() + pos;

    double as = std::sin(start);
    double ac = std::cos(start);

    cmdData[0] = initialCmd;
    vtxData[0].reset(cp.x() + rp.x() * ac, cp.y() + rp.y() * as);

    if (startPath || pos == 0 || !Math::isNear(vtxData[-1].x(), vtxData[0].x()) || !Math::isNear(vtxData[-1].y(), vtxData[0].y())) {
      cmdData++;
      vtxData++;
    }
    else {
      thisI->_size--;
    }

    as = std::sin(start + sweep);
    ac = std::cos(start + sweep);

    cmdData[0] = kCmdLineTo;
    vtxData[0].reset(cp.x() + rp.x() * ac, cp.y() + rp.y() * as);

    return kErrorOk;
  }
  else {
    size_t pos = _add(13, Path2DInfo::kGeometryFlags | Path2DInfo::kCommandCubicTo);
    if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
      return DebugUtils::errored(kErrorNoMemory);

    Path2DImpl* thisI = impl();
    uint8_t* cmdData = thisI->commandData() + pos;
    Point* vtxData = thisI->vertexData() + pos;

    uint32_t splineSize = Arc(cp, rp, start, sweep).toCSpline(vtxData);
    B2D_ASSERT(splineSize <= 13);

    cmdData[0] = initialCmd;

    for (uint32_t i = 1; i < splineSize; i += 3) {
      cmdData[i    ] = kCmdCubicTo;
      cmdData[i + 1] = kCmdControl;
      cmdData[i + 2] = kCmdControl;
    }

    thisI->_size = pos + splineSize;
    return kErrorOk;
  }
}

Error Path2D::arcToRel(const Point& cp, const Point& rp, double start, double sweep, bool startPath) noexcept {
  Point offset;
  B2D_PROPAGATE(_bPathGetLastVertex(this, offset));
  return arcTo(cp + offset, rp, start, sweep, startPath);
}

Error Path2D::svgArcTo(const Point& rp, double angle, bool largeArcFlag, bool sweepFlag, const Point& p2) noexcept {
  // Mark current size (will be position where the first bezier would start).
  size_t mark = impl()->size();

  Point p0;

  // Get initial point - p0.
  B2D_PROPAGATE(_bPathGetLastVertex(this, p0));

  // Normalize radius.
  Point r = Math::abs(rp);

  // Calculate the middle point between the current and the final points.
  double dx2 = (p0.x() - p2.x()) * 0.5;
  double dy2 = (p0.y() - p2.y()) * 0.5;

  double as = std::sin(angle);
  double ac = std::cos(angle);

  // Calculate middle point - p1.
  Point p1 { ac * dx2 + as * dy2, -as * dx2 + ac * dy2 };

  // Ensure radii are large enough.
  Point r2 = r * r;
  Point p1_2 = p1 * p1;

  // Check that radii are large enough.
  double radiiCheck = p1_2.x() / r2.x() + p1_2.y() / r2.y();
  if (radiiCheck > 1.0) {
    r *= std::sqrt(radiiCheck);
    r2 = r * r;
  }

  // Calculate cp.
  double sign = (largeArcFlag == sweepFlag) ? -1.0 : 1.0;
  double sq   = (r2.x() * r2.y()  - r2.x() * p1_2.y() - r2.y() * p1_2.x()) / (r2.x() * p1_2.y() + r2.y() * p1_2.x());
  double coef = sign * (sq <= 0.0 ? 0.0 : std::sqrt(sq));

  Point cp(coef *  ((r.x() * p1.y()) / r.y()), coef * -((r.y() * p1.x()) / r.x()));

  // Calculate [cx, cy] from [cp.x, cp.y].
  double sx2 = (p0.x() + p2.x()) * 0.5;
  double sy2 = (p0.y() + p2.y()) * 0.5;
  double cx = sx2 + (ac * cp.x() - as * cp.y());
  double cy = sy2 + (as * cp.x() + ac * cp.y());

  // Calculate the start_angle and the sweep_angle.
  double ux = ( p1.x() - cp.x()) / r.x();
  double uy = ( p1.y() - cp.y()) / r.y();
  double vx = (-p1.x() - cp.x()) / r.x();
  double vy = (-p1.y() - cp.y()) / r.y();

  // Calculate the angle start.
  double n = std::sqrt(ux * ux + uy * uy);
  double p = ux; // (1 * ux) + (0 * uy)

  sign = (uy < 0.0) ? -1.0 : 1.0;

  double v = Math::bound(p / n, -1.0, 1.0);
  double startAngle = sign * std::acos(v);

  // Calculate the sweep angle.
  n = std::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
  p = ux * vx + uy * vy;

  sign = (ux * vy - uy * vx < 0) ? -1.0 : 1.0;
  v = Math::bound(p / n, -1.0, 1.0);

  double sweepAngle = sign * std::acos(v);

  if (sweepFlag) {
    if (sweepAngle < 0)
      sweepAngle += Math::k2Pi;
  }
  else {
    if (sweepAngle > 0)
      sweepAngle -= Math::k2Pi;
  }

  B2D_PROPAGATE(arcTo(Point(0.0, 0.0), r, startAngle, sweepAngle, false));
  Path2DImpl* thisI = impl();

  size_t size = thisI->size();
  Point* vtxData = thisI->vertexData();

  // If no error has been reported then calling arcTo() should have added
  // at least two vertices. We need at least one to fix the matrix.
  B2D_ASSERT(size > 0);

  // We can now transform the resulting arc.
  Matrix2D matrix = Matrix2D::rotation(angle, cx, cy);
  matrix.mapPointsByType(Matrix2D::kTypeAffine, &vtxData[mark], &vtxData[mark], size - mark);

  // We must make sure that the starting and ending points exactly coincide
  // with the initial p0 and p2.
  vtxData[mark    ].reset(p0.x(), p0.y());
  vtxData[size - 1].reset(p2.x(), p2.y());

  return kErrorOk;
}

Error Path2D::svgArcToRel(const Point& rp, double angle, bool largeArcFlag, bool sweepFlag, const Point& p2) noexcept {
  Point offset;
  B2D_PROPAGATE(_bPathGetLastVertex(this, offset));
  return svgArcTo(rp, angle, largeArcFlag, sweepFlag, p2 + offset);
}

// ============================================================================
// [b2d::Path2D - Close]
// ============================================================================

Error Path2D::close() noexcept {
  PATH_ADD_DATA_BEGIN(1, Path2DInfo::kNoFlags)
    cmdData[0] = kCmdClose;
    vtxData[0].resetToNaN();
  PATH_ADD_DATA_END()
}

// ============================================================================
// [b2d::Path2D - AddBox / AddRect]
// ============================================================================

static B2D_INLINE Error _bPathImplAddBox(Path2DImpl* d, size_t pos, double x0, double y0, double x1, double y1, uint32_t dir) noexcept {
  uint8_t* cmdData = d->commandData() + pos;
  Point* vtxData = d->vertexData() + pos;

  cmdData[0] = Path2D::kCmdMoveTo;
  cmdData[1] = Path2D::kCmdLineTo;
  cmdData[2] = Path2D::kCmdLineTo;
  cmdData[3] = Path2D::kCmdLineTo;
  cmdData[4] = Path2D::kCmdClose;

  vtxData[0].reset(x0, y0);
  vtxData[1].reset(x1, y0);
  vtxData[2].reset(x1, y1);
  vtxData[3].reset(x0, y1);
  vtxData[4].resetToNaN();

  if (dir == Path2D::kDirCCW) {
    vtxData[1].reset(x0, y1);
    vtxData[3].reset(x1, y0);
  }

  return kErrorOk;
}

Error Path2D::addBox(const IntBox& box, uint32_t dir) noexcept {
  if (B2D_UNLIKELY(!box.isValid()))
    return kErrorOk;

  size_t pos = _add(5, Path2DInfo::kGeometryFlags);
  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  return _bPathImplAddBox(impl(), pos,
    double(box.x0()), double(box.y0()),
    double(box.x1()), double(box.y1()), dir);
}

Error Path2D::addBox(const Box& box, uint32_t dir) noexcept {
  if (B2D_UNLIKELY(!box.isValid()))
    return kErrorOk;

  size_t pos = _add(5, Path2DInfo::kGeometryFlags);
  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  return _bPathImplAddBox(impl(), pos, box.x0(), box.y0(), box.x1(), box.y1(), dir);
}

Error Path2D::addRect(const IntRect& rect, uint32_t dir) noexcept {
  if (B2D_UNLIKELY(!rect.isValid()))
    return kErrorOk;

  size_t pos = _add(5, Path2DInfo::kGeometryFlags);
  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  double x0 = double(rect.x());
  double y0 = double(rect.y());
  double x1 = double(rect.w()) + x0;
  double y1 = double(rect.h()) + y0;
  return _bPathImplAddBox(impl(), pos, x0, y0, x1, y1, dir);
}

Error Path2D::addRect(const Rect& rect, uint32_t dir) noexcept {
  if (B2D_UNLIKELY(!rect.isValid()))
    return kErrorOk;

  size_t pos = _add(5, Path2DInfo::kGeometryFlags);
  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  double x0 = rect.x();
  double y0 = rect.y();
  double x1 = rect.w() + x0;
  double y1 = rect.h() + y0;
  return _bPathImplAddBox(impl(), pos, x0, y0, x1, y1, dir);
}

// ============================================================================
// [b2d::Path2D - AddArg]
// ============================================================================

Error Path2D::_addArg(uint32_t id, const void* data, const Matrix2D* m, uint32_t dir) noexcept {
  if (B2D_UNLIKELY(id >= kGeomArgCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  size_t count = 0;
  uint32_t mType = m ? m->type() : Matrix2D::kTypeIdentity;
  uint32_t dirtyFlags = Path2DInfo::kGeometryFlags;

  switch (id) {
    case kGeomArgNone:
      return DebugUtils::errored(kErrorNoGeometry);

    case kGeomArgLine:
      count = 2;
      break;

    case kGeomArgArc:
      count = 13;
      dirtyFlags |= Path2DInfo::kCommandCubicTo;
      break;

    case kGeomArgBox: {
      const Box* box = static_cast<const Box*>(data);
      if (B2D_UNLIKELY(!box->isValid()))
        return DebugUtils::errored(kErrorInvalidGeometry);

      count = 5;
      break;
    }

    case kGeomArgIntBox: {
      const IntBox* box = static_cast<const IntBox*>(data);
      if (B2D_UNLIKELY(!box->isValid()))
        return DebugUtils::errored(kErrorInvalidGeometry);

      count = 5;
      break;
    }

    case kGeomArgRect: {
      const Rect* rect = static_cast<const Rect*>(data);
      if (B2D_UNLIKELY(!rect->isValid()))
        return DebugUtils::errored(kErrorInvalidGeometry);

      count = 5;
      break;
    }

    case kGeomArgIntRect: {
      const IntRect* rect = static_cast<const IntRect*>(data);
      if (B2D_UNLIKELY(!rect->isValid()))
        return DebugUtils::errored(kErrorInvalidGeometry);

      count = 5;
      break;
    }

    case kGeomArgRound: {
      const Round* round = static_cast<const Round*>(data);
      if (B2D_UNLIKELY(!round->_rect.isValid()))
        return DebugUtils::errored(kErrorInvalidGeometry);

      count = 18;
      dirtyFlags |= Path2DInfo::kCommandCubicTo;
      break;
    }

    case kGeomArgCircle:
    case kGeomArgEllipse:
      count = 14;
      dirtyFlags |= Path2DInfo::kCommandCubicTo;
      break;

    case kGeomArgChord:
    case kGeomArgPie:
      count = 20;
      dirtyFlags |= Path2DInfo::kCommandCubicTo;
      break;

    case kGeomArgTriangle:
      count = 4;
      break;

    case kGeomArgPolyline:
    case kGeomArgIntPolyline: {
      const CArray<void>* array = static_cast<const CArray<void>*>(data);
      count = array->size();

      if (B2D_UNLIKELY(count == 0))
        return kErrorOk;
      break;
    }

    case kGeomArgPolygon:
    case kGeomArgIntPolygon: {
      const CArray<void>* array = static_cast<const CArray<void>*>(data);
      count = array->size();

      if (B2D_UNLIKELY(count == 0))
        return kErrorOk;
      count++;
      break;
    }

    case kGeomArgArrayBox:
    case kGeomArgArrayIntBox:
    case kGeomArgArrayRect:
    case kGeomArgArrayIntRect: {
      const CArray<void>* array = static_cast<const CArray<void>*>(data);
      count = array->size();

      if (B2D_UNLIKELY(count == 0))
        return kErrorOk;

      Support::OverflowFlag of;
      count = Support::safeMul<size_t>(count, 5, &of);

      if (B2D_UNLIKELY(of))
        return DebugUtils::errored(kErrorNoMemory);
      break;
    }

    case kGeomArgRegion: {
      const Region* region = static_cast<const Region*>(data);
      count = region->size();

      if (count == 0)
        return kErrorOk;

      Support::OverflowFlag of;
      count = Support::safeMul<size_t>(count, 5, &of);

      if (B2D_UNLIKELY(of))
        return DebugUtils::errored(kErrorNoMemory);

      if (region->isInfinite())
        return DebugUtils::errored(kErrorInvalidGeometry);
      break;
    }

    case kGeomArgPath2D: {
      const Path2D* path = static_cast<const Path2D*>(data);

      // TODO: Direction!
      if (mType != Matrix2D::kTypeIdentity)
        return addPath(*path, *m);
      else
        return addPath(*path);
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  B2D_ASSERT(count > 0);
  size_t pos = _add(count, dirtyFlags);
  size_t nAdded;

  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  // Destination buffers.
  Path2DImpl* thisI = impl();
  uint8_t* cmdData = thisI->commandData() + pos;
  Point* vtxData = thisI->vertexData() + pos;

  // For adding 'Box', 'IntBox', 'Rect', and 'IntRect'.
  double x0, y0;
  double x1, y1;

  switch (id) {
    case kGeomArgLine: {
      const Line* line = static_cast<const Line*>(data);

      cmdData[0] = kCmdMoveTo;
      cmdData[1] = kCmdLineTo;

      vtxData[0] = line->p0();
      vtxData[1] = line->p1();

      if (dir != kDirCW) {
        vtxData[0] = line->p1();
        vtxData[1] = line->p0();
      }
      break;
    }

    case kGeomArgArc: {
      const Arc* arc = static_cast<const Arc*>(data);
      Arc rev;

      if (dir != kDirCW) {
        rev.reset(arc->_center, arc->_radius, arc->_start, -arc->_sweep);
        arc = &rev;
      }

      uint32_t size = arc->toCSpline(vtxData);
      cmdData[0] = kCmdMoveTo;

      for (uint32_t i = 1; i < size; i += 3) {
        cmdData[i + 0] = kCmdCubicTo;
        cmdData[i + 1] = kCmdControl;
        cmdData[i + 2] = kCmdControl;
      }

      thisI->_size = pos + size;
      break;
    }

    case kGeomArgBox: {
      x0 = static_cast<const Box*>(data)->x0();
      y0 = static_cast<const Box*>(data)->y0();
      x1 = static_cast<const Box*>(data)->x1();
      y1 = static_cast<const Box*>(data)->y1();

AddBox:
      cmdData[0] = kCmdMoveTo;
      cmdData[1] = kCmdLineTo;
      cmdData[2] = kCmdLineTo;
      cmdData[3] = kCmdLineTo;
      cmdData[4] = kCmdClose;

      vtxData[0].reset(x0, y0);
      vtxData[1].reset(x1, y0);
      vtxData[2].reset(x1, y1);
      vtxData[3].reset(x0, y1);
      vtxData[4].resetToNaN();

      if (dir != kDirCW) {
        vtxData[1].reset(x0, y1);
        vtxData[3].reset(x1, y0);
      }
      break;
    }

    case kGeomArgIntBox: {
      x0 = double(static_cast<const IntBox*>(data)->x0());
      y0 = double(static_cast<const IntBox*>(data)->y0());
      x1 = double(static_cast<const IntBox*>(data)->x1());
      y1 = double(static_cast<const IntBox*>(data)->y1());
      goto AddBox;
    }

    case kGeomArgRect: {
      x0 = static_cast<const Rect*>(data)->x();
      y0 = static_cast<const Rect*>(data)->y();
      x1 = static_cast<const Rect*>(data)->w() + x0;
      y1 = static_cast<const Rect*>(data)->h() + y0;
      goto AddBox;
    }

    case kGeomArgIntRect: {
      x0 = double(static_cast<const IntRect*>(data)->x());
      y0 = double(static_cast<const IntRect*>(data)->y());
      x1 = double(static_cast<const IntRect*>(data)->w()) + x0;
      y1 = double(static_cast<const IntRect*>(data)->h()) + y0;
      goto AddBox;
    }

    case kGeomArgRound: {
      const Round* round = static_cast<const Round*>(data);
      const Rect& rect = round->_rect;

      double wHalf = rect.w() * 0.5;
      double hHalf = rect.h() * 0.5;

      double rx = Math::min(Math::abs(round->_radius.x()), wHalf);
      double ry = Math::min(Math::abs(round->_radius.y()), hHalf);

      x0 = rect.x();
      y0 = rect.y();
      x1 = rect.x() + rect.w();
      y1 = rect.y() + rect.h();

      if (Math::isNearZero(rx) || Math::isNearZero(ry)) {
        // 18 - 5 == 13 (difference between vertices needed by kGeomArgRound and kGeomArgRect).
        thisI->_size -= 18 - 5;
        goto AddBox;
      }

      double kx = rx * Math::k1MinusKappa;
      double ky = ry * Math::k1MinusKappa;

      if (dir == kDirCW) {
        cmdData[ 0] = kCmdMoveTo;

        cmdData[ 1] = kCmdLineTo;
        cmdData[ 2] = kCmdCubicTo;
        cmdData[ 3] = kCmdControl;
        cmdData[ 4] = kCmdControl;

        cmdData[ 5] = kCmdLineTo;
        cmdData[ 6] = kCmdCubicTo;
        cmdData[ 7] = kCmdControl;
        cmdData[ 8] = kCmdControl;

        cmdData[ 9] = kCmdLineTo;
        cmdData[10] = kCmdCubicTo;
        cmdData[11] = kCmdControl;
        cmdData[12] = kCmdControl;

        cmdData[13] = kCmdLineTo;
        cmdData[14] = kCmdCubicTo;
        cmdData[15] = kCmdControl;
        cmdData[16] = kCmdControl;

        cmdData[17] = kCmdClose;

        vtxData[ 0].reset(x0 + rx, y0     );

        vtxData[ 1].reset(x1 - rx, y0     );
        vtxData[ 2].reset(x1 - kx, y0     );
        vtxData[ 3].reset(x1     , y0 + ky);
        vtxData[ 4].reset(x1     , y0 + ry);

        vtxData[ 5].reset(x1     , y1 - ry);
        vtxData[ 6].reset(x1     , y1 - ky);
        vtxData[ 7].reset(x1 - kx, y1     );
        vtxData[ 8].reset(x1 - rx, y1     );

        vtxData[ 9].reset(x0 + rx, y1     );
        vtxData[10].reset(x0 + kx, y1     );
        vtxData[11].reset(x0     , y1 - ky);
        vtxData[12].reset(x0     , y1 - ry);

        vtxData[13].reset(x0     , y0 + ry);
        vtxData[14].reset(x0     , y0 + ky);
        vtxData[15].reset(x0 + kx, y0     );
        vtxData[16].reset(x0 + rx, y0     );

        vtxData[17].resetToNaN();
      }
      else {
        cmdData[ 0] = kCmdMoveTo;
        cmdData[ 1] = kCmdCubicTo;
        cmdData[ 2] = kCmdControl;
        cmdData[ 3] = kCmdControl;

        cmdData[ 4] = kCmdLineTo;
        cmdData[ 5] = kCmdCubicTo;
        cmdData[ 6] = kCmdControl;
        cmdData[ 7] = kCmdControl;

        cmdData[ 8] = kCmdLineTo;
        cmdData[ 9] = kCmdCubicTo;
        cmdData[10] = kCmdControl;
        cmdData[11] = kCmdControl;

        cmdData[12] = kCmdLineTo;
        cmdData[13] = kCmdCubicTo;
        cmdData[14] = kCmdControl;
        cmdData[15] = kCmdControl;

        cmdData[16] = kCmdClose;

        vtxData[ 0].reset(x0 + rx, y0     );
        vtxData[ 1].reset(x0 + kx, y0     );
        vtxData[ 2].reset(x0     , y0 + ky);
        vtxData[ 3].reset(x0     , y0 + ry);

        vtxData[ 4].reset(x0     , y1 - ry);
        vtxData[ 5].reset(x0     , y1 - ky);
        vtxData[ 6].reset(x0 + kx, y1     );
        vtxData[ 7].reset(x0 + rx, y1     );

        vtxData[ 8].reset(x1 - rx, y1     );
        vtxData[ 9].reset(x1 - kx, y1     );
        vtxData[10].reset(x1     , y1 - ky);
        vtxData[11].reset(x1     , y1 - ry);

        vtxData[12].reset(x1     , y0 + ry);
        vtxData[13].reset(x1     , y0 + ky);
        vtxData[14].reset(x1 - kx, y0     );
        vtxData[15].reset(x1 - rx, y0     );

        vtxData[16].resetToNaN();
        thisI->_size--;
      }
      break;
    }

    case kGeomArgCircle:
    case kGeomArgEllipse: {
      double cx, rx, rxKappa;
      double cy, ry, ryKappa;

      if (id == kGeomArgCircle) {
        const Circle* circle = static_cast<const Circle*>(data);

        cx = circle->_center.x();
        cy = circle->_center.y();

        rx = circle->_radius;
        ry = std::abs(rx);
      }
      else {
        const Ellipse* ellipse = static_cast<const Ellipse*>(data);

        cx = ellipse->_center.x();
        cy = ellipse->_center.y();

        rx = ellipse->_radius.x();
        ry = ellipse->_radius.y();
      }

      if (dir == kDirCCW)
        ry = -ry;

      rxKappa = rx * Math::kKappa;
      ryKappa = ry * Math::kKappa;

      cmdData[ 0] = kCmdMoveTo;
      cmdData[ 1] = kCmdCubicTo;
      cmdData[ 2] = kCmdControl;
      cmdData[ 3] = kCmdControl;
      cmdData[ 4] = kCmdCubicTo;
      cmdData[ 5] = kCmdControl;
      cmdData[ 6] = kCmdControl;
      cmdData[ 7] = kCmdCubicTo;
      cmdData[ 8] = kCmdControl;
      cmdData[ 9] = kCmdControl;
      cmdData[10] = kCmdCubicTo;
      cmdData[11] = kCmdControl;
      cmdData[12] = kCmdControl;
      cmdData[13] = kCmdClose;

      vtxData[ 0].reset(cx + rx     , cy          );
      vtxData[ 1].reset(cx + rx     , cy + ryKappa);
      vtxData[ 2].reset(cx + rxKappa, cy + ry     );
      vtxData[ 3].reset(cx          , cy + ry     );
      vtxData[ 4].reset(cx - rxKappa, cy + ry     );
      vtxData[ 5].reset(cx - rx     , cy + ryKappa);
      vtxData[ 6].reset(cx - rx     , cy          );
      vtxData[ 7].reset(cx - rx     , cy - ryKappa);
      vtxData[ 8].reset(cx - rxKappa, cy - ry     );
      vtxData[ 9].reset(cx          , cy - ry     );
      vtxData[10].reset(cx + rxKappa, cy - ry     );
      vtxData[11].reset(cx + rx     , cy - ryKappa);
      vtxData[12].reset(cx + rx     , cy          );
      vtxData[13].resetToNaN();
      break;
    }

    case kGeomArgChord:
    case kGeomArgPie: {
      const Arc* arc = static_cast<const Arc*>(data);

      cmdData[0] = kCmdMoveTo;
      cmdData[1] = kCmdLineTo;
      vtxData[0].reset(arc->_center);

      if (id == kGeomArgPie) {
        cmdData++;
        vtxData++;
      }

      Arc rev;
      if (dir != kDirCW) {
        rev.reset(arc->_center, arc->_radius, arc->_start, -arc->_sweep);
        arc = &rev;
      }

      uint32_t size = arc->toCSpline(vtxData);
      for (uint32_t i = 1; i < size; i += 3) {
        B2D_ASSERT(i + 2 <= size);
        cmdData[i + 0] = kCmdCubicTo;
        cmdData[i + 1] = kCmdControl;
        cmdData[i + 2] = kCmdControl;
      }

      cmdData[size] = kCmdClose;
      vtxData[size].resetToNaN();

      cmdData += size + 1;
      vtxData += size + 1;

      thisI->_size = (size_t)(cmdData - thisI->commandData());
      break;
    }

    case kGeomArgTriangle: {
      const Triangle* triangle = static_cast<const Triangle*>(data);

      cmdData[0] = kCmdMoveTo;
      cmdData[1] = kCmdLineTo;
      cmdData[2] = kCmdLineTo;
      cmdData[3] = kCmdClose;

      vtxData[0] = triangle->_p[0];
      vtxData[1] = triangle->_p[1];
      vtxData[2] = triangle->_p[2];
      vtxData[3].resetToNaN();

      if (dir != kDirCW) {
        vtxData[1] = triangle->_p[2];
        vtxData[2] = triangle->_p[1];
      }
      break;
    }

    case kGeomArgPolyline: {
      const CArray<Point>* array = static_cast<const CArray<Point>*>(data);
      const Point* srcData = array->data();

      size_t i;
      cmdData[0] = kCmdMoveTo;

      for (i = 1; i < count; i++)
        cmdData[i] = kCmdLineTo;

      if (dir == kDirCW) {
        for (i = count; i; i--, vtxData++, srcData++)
          *vtxData = *srcData;
      }
      else {
        srcData += count - 1;
        for (i = count; i; i--, vtxData++, srcData--)
          *vtxData = *srcData;
      }
      break;
    }

    case kGeomArgIntPolyline: {
      const CArray<IntPoint>* array = static_cast<const CArray<IntPoint>*>(data);
      const IntPoint* srcData = array->data();

      size_t i;
      cmdData[0] = kCmdMoveTo;

      for (i = 1; i < count; i++)
        cmdData[i] = kCmdLineTo;

      if (dir == kDirCW) {
        for (i = count; i; i--, vtxData++, srcData++)
          *vtxData = *srcData;
      }
      else {
        srcData += count - 1;
        for (i = count; i; i--, vtxData++, srcData--)
          *vtxData = *srcData;
      }

      break;
    }

    case kGeomArgPolygon: {
      const CArray<Point>* array = static_cast<const CArray<Point>*>(data);
      const Point* srcData = array->data();

      size_t i;
      count--;

      cmdData[0] = kCmdMoveTo;
      for (i = 1; i < count; i++)
        cmdData[i] = kCmdLineTo;
      cmdData[count] = kCmdClose;

      vtxData[0] = srcData[0];
      vtxData[count].resetToNaN();

      if (dir == kDirCW) {
        for (i = count - 1; i; i--)
          *++vtxData = *++srcData;
      }
      else {
        srcData += count;
        for (i = count - 1; i; i--)
          *++vtxData = *--srcData;
      }
      break;
    }

    case kGeomArgIntPolygon: {
      const CArray<IntPoint>* array = static_cast<const CArray<IntPoint>*>(data);
      const IntPoint* srcData = array->data();

      size_t i;
      count--;

      cmdData[0] = kCmdMoveTo;
      for (i = 1; i < count; i++)
        cmdData[i] = kCmdLineTo;
      cmdData[count] = kCmdClose;

      vtxData[0] = srcData[0];
      vtxData[count].resetToNaN();

      if (dir == kDirCW) {
        for (i = count - 1; i; i--)
          *++vtxData = *++srcData;
      }
      else {
        srcData += count - 1;
        for (i = count - 1; i; i--)
          *++vtxData = *--srcData;
      }
      break;
    }

    case kGeomArgArrayBox: {
      const CArray<Box>* array = static_cast<const CArray<Box>*>(data);
      const Box* srcData = array->data();

      nAdded = 0;

      if (dir == kDirCW) {
        for (size_t i = count; i != 0; i--, srcData++) {
          if (!srcData->isValid())
            continue;

          x0 = srcData->x0();
          y0 = srcData->y0();
          x1 = srcData->x1();
          y1 = srcData->y1();

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x1, y0);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x0, y1);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }
      else {
        srcData += count - 1;
        for (size_t i = count; i != 0; i--, srcData--) {
          if (!srcData->isValid())
            continue;

          x0 = srcData->x0();
          y0 = srcData->y0();
          x1 = srcData->x1();
          y1 = srcData->y1();

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x0, y1);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x1, y0);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }
    }

AddArrayBoxCmds:
    {
      for (size_t i = nAdded; i; i--) {
        cmdData[0] = kCmdMoveTo;
        cmdData[1] = kCmdLineTo;
        cmdData[2] = kCmdLineTo;
        cmdData[3] = kCmdLineTo;
        cmdData[4] = kCmdClose;
        cmdData += 5;
      }

      thisI->_size = (size_t)(cmdData - thisI->commandData());
      break;
    }

    case kGeomArgArrayIntBox: {
      const CArray<IntBox>* array = static_cast<const CArray<IntBox>*>(data);
      const IntBox* srcData = array->data();

      nAdded = 0;

      if (dir == kDirCW) {
        for (size_t i = count; i != 0; i--, srcData++) {
          if (!srcData->isValid())
            continue;

          x0 = double(srcData->x0());
          y0 = double(srcData->y0());
          x1 = double(srcData->x1());
          y1 = double(srcData->y1());

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x1, y0);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x0, y1);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }
      else {
        srcData += count - 1;
        for (size_t i = count; i != 0; i--, srcData--) {
          if (!srcData->isValid())
            continue;

          x0 = double(srcData->x0());
          y0 = double(srcData->y0());
          x1 = double(srcData->x1());
          y1 = double(srcData->y1());

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x0, y1);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x1, y0);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }

      goto AddArrayBoxCmds;
    }

    case kGeomArgArrayRect: {
      const CArray<Rect>* array = static_cast<const CArray<Rect>*>(data);
      const Rect* srcData = array->data();

      nAdded = 0;

      if (dir == kDirCW) {
        for (size_t i = count; i != 0; i--, srcData++) {
          if (!srcData->isValid())
            continue;

          x0 = srcData->x();
          y0 = srcData->y();
          x1 = srcData->w() + x0;
          y1 = srcData->h() + y0;

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x1, y0);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x0, y1);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }
      else {
        srcData += count - 1;
        for (size_t i = count; i != 0; i--, srcData--) {
          if (!srcData->isValid())
            continue;

          x0 = srcData->x();
          y0 = srcData->y();
          x1 = srcData->w() + x0;
          y1 = srcData->h() + y0;

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x0, y1);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x1, y0);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }

      goto AddArrayBoxCmds;
    }

    case kGeomArgArrayIntRect: {
      const CArray<IntRect>* array = static_cast<const CArray<IntRect>*>(data);
      const IntRect* srcData = array->data();

      nAdded = 0;

      if (dir == kDirCW) {
        for (size_t i = count; i != 0; i--, srcData++) {
          if (!srcData->isValid())
            continue;

          x0 = double(srcData->x());
          y0 = double(srcData->y());
          x1 = double(srcData->w()) + x0;
          y1 = double(srcData->h()) + y0;

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x1, y0);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x0, y1);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }
      else {
        srcData += count - 1;
        for (size_t i = count; i != 0; i--, srcData--) {
          if (!srcData->isValid())
            continue;

          x0 = double(srcData->x());
          y0 = double(srcData->y());
          x1 = double(srcData->w()) + x0;
          y1 = double(srcData->h()) + y0;

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x0, y1);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x1, y0);
          vtxData[4].resetToNaN();

          vtxData += 5;
          nAdded++;
        }
      }

      goto AddArrayBoxCmds;
    }

    case kGeomArgRegion: {
      const Region* region = static_cast<const Region*>(data);
      const IntBox* src = region->data();

      size_t i;

      if (dir == kDirCW) {
        for (i = count; i != 0; i--, src++) {
          x0 = double(src->x0());
          y0 = double(src->y0());
          x1 = double(src->x1());
          y1 = double(src->y1());

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x1, y0);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x0, y1);
          vtxData[4].resetToNaN();

          vtxData += 5;
        }
      }
      else {
        src += count - 1;
        for (i = count; i != 0; i--, src--) {
          x0 = double(src->x0());
          y0 = double(src->y0());
          x1 = double(src->x1());
          y1 = double(src->y1());

          vtxData[0].reset(x0, y0);
          vtxData[1].reset(x0, y1);
          vtxData[2].reset(x1, y1);
          vtxData[3].reset(x1, y0);
          vtxData[4].resetToNaN();

          vtxData += 5;
        }
      }

      for (i = count; i; i--) {
        cmdData[0] = kCmdMoveTo;
        cmdData[1] = kCmdLineTo;
        cmdData[2] = kCmdLineTo;
        cmdData[3] = kCmdLineTo;
        cmdData[4] = kCmdClose;
        cmdData += 5;
      }
      break;
    }

    default:
      B2D_NOT_REACHED();
  }

  if (mType != Matrix2D::kTypeIdentity)
    m->mapPointsByType(mType, vtxData, vtxData, thisI->size() - pos);

  return kErrorOk;
}

// ============================================================================
// [b2d::Path2D - AddPath]
// ============================================================================

static Error bPathAddPath(Path2D& dst, const Path2D& src, size_t rIdx, size_t rEnd, const Matrix2D& m, uint32_t mType) noexcept {
  if (mType >= Matrix2D::kTypeInvalid)
    return DebugUtils::errored(kErrorInvalidMatrix);

  uint32_t dirtyFlags = (Path2DInfo::kGeometryFlags) |
                        (src.impl()->pathInfo().flags() & Path2DInfo::kCommandFlags);

  size_t rSize = rEnd - rIdx;
  size_t pos = dst._add(rSize, dirtyFlags);

  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  Path2DImpl* dstI = dst.impl();
  Path2DImpl* srcI = src.impl();

  Path2D_copyCommandData(dstI->commandData() + pos, srcI->commandData() + rIdx, rSize);
  m.mapPointsByType(mType, dstI->vertexData() + pos, srcI->vertexData() + rIdx, rSize);

  return kErrorOk;
}

Error Path2D::addPath(const Path2D& other) noexcept {
  return addPath(other, Range::everything());
}

Error Path2D::addPath(const Path2D& other, const Range& range) noexcept {
  size_t pathSize = other.size();
  size_t rIdx, rEnd;

  if (!range.fit(pathSize, rIdx, rEnd))
    return kErrorOk;

  size_t rSize = rEnd - rIdx;
  size_t pos = _add(rSize, Path2DInfo::kAllFlags);

  if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
    return DebugUtils::errored(kErrorNoMemory);

  Path2DImpl* thisI = this->impl();
  Path2DImpl* otherI = other.impl();

  Path2D_copyCommandData(thisI->commandData() + pos, otherI->commandData() + rIdx, rSize);
  Path2D_copyVertexData(thisI->vertexData() + pos, otherI->vertexData() + rIdx, rSize);

  return kErrorOk;
}

Error Path2D::addPath(const Path2D& path, const Point& p) noexcept {
  size_t pathSize = path.size();

  Matrix2D m(1.0, 0.0, 0.0, 1.0, p.x(), p.y());
  return bPathAddPath(*this, path, 0, pathSize, m, Matrix2D::kTypeTranslation);
}

Error Path2D::addPath(const Path2D& path, const Range& range, const Point& p) noexcept {
  size_t pathSize = path.size();
  size_t rIdx, rEnd;

  if (B2D_UNLIKELY(!range.fit(pathSize, rIdx, rEnd)))
    return kErrorOk;

  Matrix2D m(1.0, 0.0, 0.0, 1.0, p.x(), p.y());
  return bPathAddPath(*this, path, rIdx, rEnd, m, Matrix2D::kTypeTranslation);
}

Error Path2D::addPath(const Path2D& path, const Matrix2D& m) noexcept {
  size_t pathSize = path.size();
  return bPathAddPath(*this, path, 0, pathSize, m, m.type());
}

Error Path2D::addPath(const Path2D& path, const Range& range, const Matrix2D& m) noexcept {
  size_t pathSize = path.size();
  size_t rIdx, rEnd;

  if (B2D_UNLIKELY(!range.fit(pathSize, rIdx, rEnd)))
    return kErrorOk;

  return bPathAddPath(*this, path, rIdx, rEnd, m, m.type());
}

// ============================================================================
// [b2d::Path2D - ClosestVertex]
// ============================================================================

size_t Path2D::closestVertex(const Point& p, double maxDistance, double* distance) const noexcept {
  Path2DImpl* thisI = impl();
  size_t size = thisI->size();

  if (B2D_UNLIKELY(size == 0))
    return Globals::kInvalidIndex;

  const uint8_t* cmdData = thisI->commandData();
  const Point* vtxData = thisI->vertexData();

  size_t bestIndex = Globals::kInvalidIndex;
  double bestDistance = std::numeric_limits<double>::infinity();
  double bestDistanceSq = std::numeric_limits<double>::infinity();

  double px = p.x();
  double py = p.y();

  if (maxDistance > 0.0 && maxDistance < std::numeric_limits<double>::infinity()) {
    bestDistance = maxDistance;
    bestDistanceSq = Math::pow2(bestDistance);

    // This code-path can be used to skip the whole path if the given point `p`
    // is too far. We need 'maxDistance' to be specified and also Path2DInfo to
    // be valid.
    //
    // NOTE: We can't use it if the path contains Bezier curves. In that case
    // the bounding box can be inside the box needed to cover all the control
    // points.
    uint32_t infoFlags = _queryPathInfo(Path2DInfo::kCommandInfo | Path2DInfo::kGeometryInfo);
    uint32_t testFlags = Path2DInfo::kCommandQuadTo  |
                         Path2DInfo::kCommandCubicTo |
                         Path2DInfo::kGeometryValid  ;

    if ((infoFlags & testFlags) == Path2DInfo::kGeometryValid) {
      // If the given point is outside of the path bounding-box extended
      // by `maxDistance` then there is no vertex which can be returned.
      const Box& bBox = thisI->pathInfo().boundingBox();
      if (px < bBox.x0() - bestDistance || py < bBox.y0() - bestDistance ||
          px > bBox.x1() + bestDistance || py > bBox.y1() + bestDistance) {
        return Globals::kInvalidIndex;
      }
    }
  }

  for (size_t i = 0; i < size; i++) {
    if (cmdData[i] != kCmdClose) {
      double d = Math::pow2(vtxData[i].x() - px) +
                 Math::pow2(vtxData[i].y() - py);

      if (d < bestDistanceSq) {
        bestIndex = i;
        bestDistanceSq = d;
      }
    }
  }

  bestDistance = std::sqrt(bestDistanceSq);
  if (bestIndex == Globals::kInvalidIndex)
    bestDistance = std::numeric_limits<double>::quiet_NaN();

  if (distance)
    *distance = bestDistance;

  return bestIndex;
}

// ============================================================================
// [b2d::Path2D - Transform]
// ============================================================================

static Error Path2D_transformInternal(Path2D* self, size_t rIdx, size_t rEnd, const Matrix2D& m, uint32_t mType) noexcept {
  if (mType == Matrix2D::kTypeIdentity)
    return kErrorOk;

  if (B2D_UNLIKELY(mType >= Matrix2D::kTypeInvalid))
    return DebugUtils::errored(kErrorInvalidMatrix);

  B2D_PROPAGATE(self->detach());
  Path2DImpl* thisI = self->impl();

  size_t rSize = rEnd - rIdx;
  m.mapPointsByType(mType, thisI->vertexData() + rIdx, thisI->vertexData() + rIdx, rSize);

  self->makeDirty(Path2DInfo::kGeometryFlags);
  return kErrorOk;
}

Error Path2D::translate(const Point& p) noexcept {
  size_t pathSize = size();
  if (B2D_UNLIKELY(!pathSize))
    return kErrorOk;
  return Path2D_transformInternal(this, 0, pathSize, Matrix2D::translation(p), Matrix2D::kTypeTranslation);
}

Error Path2D::translate(const Point& p, const Range& range) noexcept {
  size_t pathSize = size();
  size_t rIdx, rEnd;

  if (B2D_UNLIKELY(!range.fit(pathSize, rIdx, rEnd)))
    return kErrorOk;

  return Path2D_transformInternal(this, rIdx, rEnd, Matrix2D::translation(p), Matrix2D::kTypeTranslation);
}

Error Path2D::transform(const Matrix2D& m) noexcept {
  size_t pathSize = size();
  if (B2D_UNLIKELY(!pathSize))
    return kErrorOk;
  return Path2D_transformInternal(this, 0, pathSize, m, m.type());
}

Error Path2D::transform(const Matrix2D& m, const Range& range) noexcept {
  size_t pathSize = size();
  size_t rIdx, rEnd;

  if (B2D_UNLIKELY(!range.fit(pathSize, rIdx, rEnd)))
    return kErrorOk;

  return Path2D_transformInternal(this, rIdx, rEnd, m, m.type());
}

Error Path2D::fitTo(const Rect& rect) noexcept {
  if (!rect.isValid())
    return DebugUtils::errored(kErrorInvalidArgument);

  size_t pathSize = impl()->size();
  if (B2D_UNLIKELY(!pathSize))
    return kErrorOk;

  Box bBox;
  if (!getBoundingBox(bBox))
    return kErrorOk;

  double cx = bBox.x();
  double cy = bBox.y();

  double tx = rect.x();
  double ty = rect.y();

  double sx = rect.w() / bBox.w();
  double sy = rect.h() / bBox.h();

  tx -= cx * sx;
  ty -= cy * sy;

  Matrix2D m { sx, 0.0, 0.0, sy, tx, ty };
  return Path2D_transformInternal(this, 0, pathSize, m, Matrix2D::kTypeScaling);
}

Error Path2D::scale(double x, double y, bool keepStartPos) noexcept {
  Path2DImpl* thisI = impl();
  size_t pathSize = thisI->size();

  if (B2D_UNLIKELY(!pathSize))
    return kErrorOk;

  Matrix2D m { Matrix2D::scaling(x, y) };
  if (keepStartPos) {
    const uint8_t* cmdData = thisI->commandData();
    const Point* vtxData = thisI->vertexData();

    Point first = { 0.0, 0.0 };
    for (size_t i = 0; i < pathSize; i++) {
      if (cmdData[i] != kCmdClose) {
        first = vtxData[i];
        break;
      }
    }

    m.setM20(first.x() - first.x() * x);
    m.setM21(first.y() - first.y() * y);
  }

  return Path2D_transformInternal(this, 0, pathSize, m, Matrix2D::kTypeScaling);
}

Error Path2D::flipX(double x0, double x1) noexcept {
  Path2DImpl* thisI = impl();
  size_t pathSize = thisI->size();

  double tx = x0 + x1;
  Matrix2D m { -1.0, 0.0, 0.0, 1.0, tx, 0.0 };
  return Path2D_transformInternal(this, 0, pathSize, m, Matrix2D::kTypeScaling);
}

Error Path2D::flipY(double y0, double y1) noexcept {
  Path2DImpl* thisI = impl();
  size_t pathSize = thisI->size();

  double ty = y0 + y1;
  Matrix2D m { 1.0, 0.0, 0.0, -1.0, 0.0, ty };
  return Path2D_transformInternal(this, 0, pathSize, m, Matrix2D::kTypeScaling);
}

// ============================================================================
// [b2d::Path2D - Equality]
// ============================================================================

bool B2D_CDECL Path2D::_eq(const Path2D* a, const Path2D* b) noexcept {
  const Path2DImpl* aI = a->impl();
  const Path2DImpl* bI = b->impl();

  if (a == b)
    return true;

  size_t pathSize = aI->size();
  if (pathSize != bI->size())
    return false;

  if (std::memcmp(aI->commandData(), bI->commandData(), pathSize) != 0)
    return false;

  const Point* aVertexData = aI->vertexData();
  const Point* bVertexData = bI->vertexData();

  for (size_t i = 0; i < pathSize; i++)
    if (aVertexData[i] != bVertexData[i])
      return false;

  return true;
}

// ============================================================================
// [b2d::Path2D - Path Info]
// ============================================================================

static B2D_INLINE uint32_t Path2D_updateCommandInfo(Path2DImpl* thisI) noexcept {
  size_t i = thisI->size();
  const uint8_t* cmdData = thisI->commandData();

  uint32_t mask = 0u;

  while (i >= 4) {
    mask |= uint32_t(1u) << Math::min<uint32_t>(cmdData[0], Path2D::kCmdCount);
    mask |= uint32_t(1u) << Math::min<uint32_t>(cmdData[1], Path2D::kCmdCount);
    mask |= uint32_t(1u) << Math::min<uint32_t>(cmdData[2], Path2D::kCmdCount);
    mask |= uint32_t(1u) << Math::min<uint32_t>(cmdData[3], Path2D::kCmdCount);

    cmdData += 4;
    i -= 4;
  }

  while (i) {
    mask |= uint32_t(1u) << Math::min<uint32_t>(cmdData[0], Path2D::kCmdCount);

    cmdData++;
    i--;
  }

  return (Support::bitTest(mask, Path2D::kCmdQuadTo ) ? Path2DInfo::kCommandQuadTo   : uint32_t(0)) |
         (Support::bitTest(mask, Path2D::kCmdCubicTo) ? Path2DInfo::kCommandCubicTo  : uint32_t(0)) ;
}

static B2D_INLINE uint32_t Path2D_updateGeometryInfo(Path2DImpl* thisI) noexcept {
  size_t i = thisI->size();

  const uint8_t* cmdData = thisI->commandData();
  const Point* vtxData = thisI->vertexData();

  Box bBox {
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(),
    std::numeric_limits<double>::lowest()
  };

  bool isPrevVertex = false;

  // Iterate over the whole path.
  while (i) {
    uint32_t c = cmdData[0];

    switch (c) {
      case Path2D::kCmdMoveTo:
      case Path2D::kCmdLineTo:
        isPrevVertex = true;
        bBox.bound(vtxData[0]);

        cmdData++;
        vtxData++;
        i--;
        break;

      case Path2D::kCmdQuadTo:
        if (i < 2 || !isPrevVertex)
          goto Invalid;

        isPrevVertex = true;
        bBox.bound(vtxData[1]);

        // Perform expensive calculations only when necessary.
        if (!(vtxData[0].x() > bBox.x0() && vtxData[0].y() > bBox.y0() && vtxData[0].x() < bBox.x1() && vtxData[0].y() < bBox.y1())) {
          Box bnd;
          if (Geom2D::quadBounds(vtxData - 1, bnd) != kErrorOk)
            goto Invalid;
          bBox.bound(bnd);
        }

        cmdData += 2;
        vtxData += 2;
        i -= 2;
        break;

      case Path2D::kCmdCubicTo:
        if (i < 3 || !isPrevVertex)
          goto Invalid;

        isPrevVertex = true;
        bBox.bound(vtxData[2]);

        // Perform expensive calculations only when necessary.
        if (!(vtxData[0].x() > bBox.x0() && vtxData[1].x() > bBox.x0() &&
              vtxData[0].y() > bBox.y0() && vtxData[1].y() > bBox.y0() &&
              vtxData[0].x() < bBox.x1() && vtxData[1].x() < bBox.x1() &&
              vtxData[0].y() < bBox.y1() && vtxData[1].y() < bBox.y1() )) {
          Box bnd;
          if (Geom2D::cubicBounds(vtxData - 1, bnd) != kErrorOk)
            goto Invalid;
          bBox.bound(bnd);
        }

        cmdData += 3;
        vtxData += 3;
        i -= 3;
        break;

      case Path2D::kCmdClose:
        isPrevVertex = false;

        cmdData++;
        vtxData++;
        i--;
        break;

      default:
        goto Invalid;
    }
  }

  if (!std::isfinite(bBox.x0()) || !std::isfinite(bBox.y0()) ||
      !std::isfinite(bBox.x1()) || !std::isfinite(bBox.y1())) {
Invalid:
    thisI->_pathInfo._boundingBox.reset();
    return 0;
  }

  if (bBox.x0() > bBox.x1() || bBox.y0() > bBox.y1()) {
    // Empty bounding-box.
    thisI->_pathInfo._boundingBox.reset();
    return Path2DInfo::kGeometryEmpty;
  }
  else {
    thisI->_pathInfo._boundingBox.reset(bBox);
    return Path2DInfo::kGeometryValid;
  }
}

B2D_API uint32_t Path2D::_updatePathInfo(uint32_t requiredFlags) const noexcept {
  Path2DImpl* thisI = impl();

  uint32_t currentFlags = thisI->pathInfo().flags();
  uint32_t queryFlags = requiredFlags & currentFlags;

  uint32_t computedFlags = 0;
  uint32_t affectedFlags = 0;

  if (queryFlags & Path2DInfo::kCommandDirty) {
    computedFlags |= Path2D_updateCommandInfo(thisI);
    affectedFlags |= Path2DInfo::kCommandFlags;
  }

  if (queryFlags & Path2DInfo::kGeometryDirty) {
    computedFlags |= Path2D_updateGeometryInfo(thisI);
    affectedFlags |= Path2DInfo::kGeometryFlags;
  }

  uint32_t andMask = ~affectedFlags | computedFlags;
  return thisI->_pathInfo._flagsAtomic.fetch_and(andMask, std::memory_order_acq_rel) & andMask;
}

// ============================================================================
// [b2d::Path2D - Runtime Handlers]
// ============================================================================

void Path2DOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  Path2DImpl* impl = &gPath2DImplNone;
  impl->_commonData._objectInfo = Any::kTypeIdPath2D | Any::kFlagIsNone;
  impl->_pathInfo._flagsNonAtomic = Path2DInfo::kInitialFlags;
  Any::_initNone(Path2D::kTypeId, impl);
}

B2D_END_NAMESPACE
