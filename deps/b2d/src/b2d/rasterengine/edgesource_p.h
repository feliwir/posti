// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_EDGESOURCE_P_H
#define _B2D_RASTERENGINE_EDGESOURCE_P_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/matrix2d.h"
#include "../core/math.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterEngine::EdgeTransformNone]
// ============================================================================

//! \internal
class EdgeTransformNone {
public:
  B2D_INLINE EdgeTransformNone() noexcept {}
  B2D_INLINE void apply(Point& dst, const Point& src) noexcept { dst = src; }
};

// ============================================================================
// [b2d::RasterEngine::EdgeTransformScale]
// ============================================================================

//! \internal
class EdgeTransformScale {
public:
  B2D_INLINE EdgeTransformScale(const Matrix2D& matrix) noexcept
    : _sx(matrix.m00()),
      _sy(matrix.m11()),
      _tx(matrix.m20()),
      _ty(matrix.m21()) {}

  B2D_INLINE EdgeTransformScale(const EdgeTransformScale& other) noexcept = default;

  B2D_INLINE void apply(Point& dst, const Point& src) noexcept {
    dst.reset(src._x * _sx + _tx, src._y * _sy + _ty);
  }

  double _sx, _sy;
  double _tx, _ty;
};

// ============================================================================
// [b2d::RasterEngine::EdgeTransformAffine]
// ============================================================================

//! \internal
class EdgeTransformAffine {
public:
  B2D_INLINE EdgeTransformAffine(const Matrix2D& matrix) noexcept
    : _matrix(matrix) {}

  B2D_INLINE EdgeTransformAffine(const EdgeTransformAffine& other) noexcept = default;

  B2D_INLINE void apply(Point& dst, const Point& src) noexcept {
    _matrix.mapPoint(dst, src);
  }

  Matrix2D _matrix;
};

// ============================================================================
// [b2d::RasterEngine::EdgeSourcePoly2D]
// ============================================================================

//! \internal
template<class TransformT = EdgeTransformNone>
class EdgeSourcePoly2D {
public:
  B2D_INLINE EdgeSourcePoly2D(const TransformT& transform) noexcept
    : _transform(transform),
      _srcPtr(nullptr),
      _srcEnd(nullptr) {}

  B2D_INLINE EdgeSourcePoly2D(const TransformT& transform, const Point* srcPtr, size_t count) noexcept
    : _transform(transform),
      _srcPtr(srcPtr),
      _srcEnd(srcPtr + count) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const Point* srcPtr, size_t count) noexcept {
    _srcPtr = srcPtr;
    _srcEnd = srcPtr + count;
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE bool begin(Point& initial) noexcept {
    if (_srcPtr == _srcEnd)
      return false;

    _transform.apply(initial, _srcPtr[0]);
    _srcPtr++;
    return true;
  }

  B2D_INLINE void beforeNextBegin() noexcept {}

  B2D_INLINE bool isLineTo() const noexcept { return _srcPtr != _srcEnd; }
  constexpr bool isQuadTo() const noexcept { return false; }
  constexpr bool isCubicTo() const noexcept { return false; }

  B2D_INLINE void nextLineTo(Point& pt1) noexcept {
    _transform.apply(pt1, _srcPtr[0]);
    _srcPtr++;
  }

  B2D_INLINE bool maybeNextLineTo(Point& pt1) noexcept {
    if (_srcPtr == _srcEnd)
      return false;

    nextLineTo(pt1);
    return true;
  }

  inline void nextQuadTo(Point&, Point&) noexcept {}
  inline bool maybeNextQuadTo(Point&, Point&) noexcept { return false; }

  inline void nextCubicTo(Point&, Point&, Point&) noexcept {}
  inline bool maybeNextCubicTo(Point&, Point&, Point&) noexcept { return false; }

  TransformT _transform;
  const Point* _srcPtr;
  const Point* _srcEnd;
};

// ============================================================================
// [b2d::RasterEngine::EdgeSourcePath2D]
// ============================================================================

//! \internal
template<class TransformT = EdgeTransformNone>
class EdgeSourcePath2D {
public:
  B2D_INLINE EdgeSourcePath2D(const TransformT& transform) noexcept
    : _transform(transform),
      _vtxPtr(nullptr),
      _cmdPtr(nullptr),
      _cmdEnd(nullptr),
      _cmdEndMinus2(nullptr) {}

  B2D_INLINE EdgeSourcePath2D(const TransformT& transform, const Point* vtxData, const uint8_t* cmdData, size_t count) noexcept
    : _transform(transform),
      _vtxPtr(vtxData),
      _cmdPtr(cmdData),
      _cmdEnd(cmdData + count),
      _cmdEndMinus2(_cmdEnd - 2) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const Point* vtxData, const uint8_t* cmdData, size_t count) noexcept {
    _vtxPtr = vtxData;
    _cmdPtr = cmdData;
    _cmdEnd = cmdData + count;
    _cmdEndMinus2 = _cmdEnd - 2;
  }

  B2D_INLINE void reset(const Path2D& path) noexcept {
    const Path2DImpl* pathI = path.impl();
    reset(pathI->vertexData(), pathI->commandData(), pathI->size());
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE bool begin(Point& initial) noexcept {
    for (;;) {
      if (_cmdPtr == _cmdEnd)
        return false;

      uint32_t cmd = _cmdPtr[0];
      _cmdPtr++;
      _vtxPtr++;

      if (cmd != Path2D::kCmdMoveTo)
        continue;

      _transform.apply(initial, _vtxPtr[-1]);
      return true;
    }
  }

  B2D_INLINE void beforeNextBegin() noexcept {}

  B2D_INLINE bool isLineTo() const noexcept { return _cmdPtr != _cmdEnd && _cmdPtr[0] == Path2D::kCmdLineTo; }
  B2D_INLINE bool isQuadTo() const noexcept { return _cmdPtr <= _cmdEndMinus2 && _cmdPtr[0] == Path2D::kCmdQuadTo; }
  B2D_INLINE bool isCubicTo() const noexcept { return _cmdPtr < _cmdEndMinus2 && _cmdPtr[0] == Path2D::kCmdCubicTo; }

  B2D_INLINE void nextLineTo(Point& pt1) noexcept {
    _transform.apply(pt1, _vtxPtr[0]);
    _cmdPtr++;
    _vtxPtr++;
  }

  B2D_INLINE bool maybeNextLineTo(Point& pt1) noexcept {
    if (!isLineTo())
      return false;

    nextLineTo(pt1);
    return true;
  }

  B2D_INLINE void nextQuadTo(Point& pt1, Point& pt2) noexcept {
    _transform.apply(pt1, _vtxPtr[0]);
    _transform.apply(pt2, _vtxPtr[1]);
    _cmdPtr += 2;
    _vtxPtr += 2;
  }

  B2D_INLINE bool maybeNextQuadTo(Point& pt1, Point& pt2) noexcept {
    if (!isQuadTo())
      return false;

    nextQuadTo(pt1, pt2);
    return true;
  }

  B2D_INLINE void nextCubicTo(Point& pt1, Point& pt2, Point& pt3) noexcept {
    _transform.apply(pt1, _vtxPtr[0]);
    _transform.apply(pt2, _vtxPtr[1]);
    _transform.apply(pt3, _vtxPtr[2]);
    _cmdPtr += 3;
    _vtxPtr += 3;
  }

  B2D_INLINE bool maybeNextCubicTo(Point& pt1, Point& pt2, Point& pt3) noexcept {
    if (!isCubicTo())
      return false;

    nextCubicTo(pt1, pt2, pt3);
    return true;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  TransformT _transform;
  const Point* _vtxPtr;
  const uint8_t* _cmdPtr;
  const uint8_t* _cmdEnd;
  const uint8_t* _cmdEndMinus2;
};

// ============================================================================
// [b2d::RasterEngine::EdgeSource...]
// ============================================================================

typedef EdgeSourcePoly2D<EdgeTransformScale> EdgeSourcePoly2DScale;
typedef EdgeSourcePoly2D<EdgeTransformAffine> EdgeSourcePoly2DAffine;

typedef EdgeSourcePath2D<EdgeTransformScale> EdgeSourcePath2DScale;
typedef EdgeSourcePath2D<EdgeTransformAffine> EdgeSourcePath2DAffine;

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_EDGESOURCE_P_H
