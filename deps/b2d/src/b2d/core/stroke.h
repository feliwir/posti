// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_STROKE_H
#define _B2D_CORE_STROKE_H

// [Dependencies]
#include "../core/array.h"
#include "../core/geomtypes.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::StrokeJoin]
// ============================================================================

namespace StrokeJoin {
  //! Stroke join type.
  enum Type : uint32_t {
    kMiter             = 0,              //!< Miter join (default).
    kRound             = 1,              //!< Round join.
    kBevel             = 2,              //!< Bevel join.
    kMiterRev          = 3,              //!< Miter join (reversed).
    kMiterRound        = 4,              //!< Miter join (rounded).

    kCount             = 5,              //!< Count of join types.
    kDefault           = kMiter          //!< Default join type.
  };
}

// ============================================================================
// [b2d::StrokeCap]
// ============================================================================

namespace StrokeCap {
  //! Stroke cap.
  enum Type : uint32_t {
    kButt              = 0,              //!< Butt cap (default).
    kSquare            = 1,              //!< Square cap.
    kRound             = 2,              //!< Round cap.
    kRoundRev          = 3,              //!< Round cap (reversed).
    kTriangle          = 4,              //!< Triangle cap.
    kTriangleRev       = 5,              //!< Triangle cap (reversed).

    kCount             = 6,              //!< Used to catch invalid arguments.
    kDefault           = kButt           //!< Default line-cap type.
  };
}

// ============================================================================
// [b2d::StrokeTransformOrder]
// ============================================================================

namespace StrokeTransformOrder {
  //! Stroke transform order.
  enum Type : uint32_t {
    kAfter             = 0,              //!< Transform after stroke  => `Transform(Stroke(Input))` (default).
    kBefore            = 1,              //!< Transform before stroke => `Stroke(Transform(Input))`.

    kCount             = 2,              //!< Count of transform order options.
    kDefault           = kAfter
  };
}

// ============================================================================
// [b2d::StrokeParams]
// ============================================================================

class StrokeParams {
public:
  static const constexpr double kStrokeWidthDefault = 1.0;
  static const constexpr double kMiterLimitDefault = 4.0;
  static const constexpr double kDashOffsetDefault = 0.0;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API StrokeParams() noexcept;
  B2D_API StrokeParams(const StrokeParams& other) noexcept;
  B2D_INLINE StrokeParams(StrokeParams&& other) noexcept
    : _strokeWidth(other._strokeWidth),
      _miterLimit(other._miterLimit),
      _dashOffset(other._dashOffset),
      _dashArray(std::move(other._dashArray)),
      _hints(other._hints) {}
  B2D_API ~StrokeParams() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE double strokeWidth() const noexcept { return _strokeWidth; }
  B2D_INLINE double miterLimit() const noexcept { return _miterLimit; }
  B2D_INLINE double dashOffset() const noexcept { return _dashOffset; }
  B2D_INLINE const Array<double>& dashArray() const noexcept { return _dashArray; }

  B2D_INLINE uint32_t hints() const noexcept { return _hints.packed; }
  B2D_INLINE uint32_t startCap() const noexcept { return _hints.startCap; }
  B2D_INLINE uint32_t endCap() const noexcept { return _hints.endCap; }
  B2D_INLINE uint32_t strokeJoin() const noexcept { return _hints.strokeJoin; }
  B2D_INLINE uint32_t transformOrder() const noexcept { return _hints.transformOrder; }

  B2D_INLINE void setStrokeWidth(double strokeWidth) noexcept { _strokeWidth = strokeWidth; }
  B2D_INLINE void setMiterLimit(double miterLimit) noexcept { _miterLimit = miterLimit; }
  B2D_INLINE void setDashOffset(double dashOffset) noexcept { _dashOffset = dashOffset; }
  B2D_INLINE void setDashArray(const Array<double>& dashArray) noexcept { _dashArray = dashArray; }
  B2D_INLINE Error setDashArray(const double* dashArray, size_t size) noexcept { return _dashArray.copyFrom(dashArray, size); }

  B2D_INLINE void setHints(uint32_t hints) noexcept { _hints.packed = hints; }
  B2D_INLINE void setStartCap(uint32_t startCap) noexcept { _hints.startCap = uint8_t(startCap); }
  B2D_INLINE void setEndCap(uint32_t endCap) noexcept { _hints.endCap = uint8_t(endCap); }
  B2D_INLINE void setLineCaps(uint32_t bothCaps) noexcept { _hints.startCap = _hints.endCap = uint8_t(bothCaps); }
  B2D_INLINE void setStrokeJoin(uint32_t strokeJoin) noexcept { _hints.strokeJoin = uint8_t(strokeJoin); }
  B2D_INLINE void setTransformOrder(uint32_t order) noexcept { _hints.transformOrder = uint8_t(order); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _strokeWidth = kStrokeWidthDefault;
    _miterLimit = kMiterLimitDefault;
    _dashOffset = kDashOffsetDefault;
    _dashArray.reset();
    _hints.startCap = StrokeCap::kDefault;
    _hints.endCap = StrokeCap::kDefault;
    _hints.strokeJoin = StrokeJoin::kDefault;
    _hints.transformOrder = StrokeTransformOrder::kDefault;
  }

  // --------------------------------------------------------------------------
  // [Valid]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isValid() const noexcept {
    return std::isfinite(_strokeWidth) &&
           std::isfinite(_miterLimit) &&
           std::isfinite(_dashOffset) &&
          _hints.startCap < StrokeCap::kCount &&
          _hints.endCap < StrokeCap::kCount &&
          _hints.strokeJoin < StrokeJoin::kCount &&
          _hints.transformOrder < StrokeTransformOrder::kCount;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_API StrokeParams& operator=(const StrokeParams& other) noexcept;

  B2D_INLINE StrokeParams& operator=(StrokeParams&& other) noexcept {
    _strokeWidth = other._strokeWidth;
    _miterLimit = other._miterLimit;
    _dashOffset = other._dashOffset;
    _dashArray = std::move(other._dashArray);
    _hints = other._hints;

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _strokeWidth;
  double _miterLimit;
  double _dashOffset;
  Array<double> _dashArray;

  union Hints {
    struct {
      uint32_t startCap       : 8;
      uint32_t endCap         : 8;
      uint32_t strokeJoin     : 8;
      uint32_t transformOrder : 8;
    };

    uint32_t packed;
  } _hints;
};

// ============================================================================
// [b2d::Stroker]
// ============================================================================

class Stroker {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API Stroker() noexcept;
  B2D_API Stroker(const Stroker& other) noexcept;
  B2D_API explicit Stroker(const StrokeParams& params) noexcept;

  B2D_API ~Stroker() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const StrokeParams& params() const noexcept { return _params; }
  B2D_API Error setParams(const StrokeParams& params) noexcept;

  B2D_INLINE double flatness() const noexcept { return _flatness; }
  B2D_API Error setFlatness(double flatness) noexcept;

  // --------------------------------------------------------------------------
  // [Update]
  // --------------------------------------------------------------------------

  B2D_API void _update() noexcept;

  B2D_INLINE void update() const noexcept {
    if (!_dirty)
      return;
    const_cast<Stroker*>(this)->_update();
  }

  // --------------------------------------------------------------------------
  // [Stroke]
  // --------------------------------------------------------------------------

  B2D_API Error _strokeArg(Path2D& dst, uint32_t argId, const void* data) const noexcept;

  B2D_INLINE Error stroke(Path2D& dst, const Rect& rect) const noexcept { return _strokeArg(dst, kGeomArgRect, &rect); }
  B2D_INLINE Error stroke(Path2D& dst, const Path2D& path) const noexcept { return _strokeArg(dst, kGeomArgPath2D, &path); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_API Stroker& operator=(const Stroker& other) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  StrokeParams _params;                  //!< Stroke parameters.

  double _w;                             //!< `width / 2` after the simple transformation (if used).
  double _wAbs;                          //!< `width / 2`.
  double _wEps;
  double _da;
  double _flatness;

  int _wSign;
  uint8_t _dirty;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_STROKE_H
