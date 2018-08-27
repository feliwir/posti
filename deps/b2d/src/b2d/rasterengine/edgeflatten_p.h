// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_EDGEFLATTEN_P_H
#define _B2D_RASTERENGINE_EDGEFLATTEN_P_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/support.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterEngine::FlattenMonoData]
// ============================================================================

//! \internal
//!
//! Base data (mostly stack) used by `FlattenMonoQuad` and `FlattenMonoCubic`.
class FlattenMonoData {
public:
  enum : size_t {
    kRecursionLimit = 32,

    kStackSizeQuad  = kRecursionLimit * 3,
    kStackSizeCubic = kRecursionLimit * 4,
    kStackSizeTotal = kStackSizeCubic
  };

  B2D_INLINE FlattenMonoData(double toleranceSq) noexcept
    : _toleranceSq(toleranceSq) {}

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _stack[kStackSizeTotal];
  double _toleranceSq;
};

// ============================================================================
// [b2d::RasterEngine::FlattenMonoQuad]
// ============================================================================

//! \internal
//!
//! Helper to flatten a monotonic quad curve.
class FlattenMonoQuad {
public:
  struct SplitStep {
    B2D_INLINE bool isFinite() const noexcept { return std::isfinite(value); }
    B2D_INLINE const Point& midPoint() const noexcept { return p012; }

    double value;
    double limit;

    Point p01;
    Point p12;
    Point p012;
  };

  B2D_INLINE explicit FlattenMonoQuad(FlattenMonoData& flattenData) noexcept
    : _flattenData(flattenData) {}

  B2D_INLINE void begin(const Point* src, uint32_t signBit) noexcept {
    _stackPtr = _flattenData._stack;

    if (signBit == 0) {
      _p0 = src[0];
      _p1 = src[1];
      _p2 = src[2];
    }
    else {
      _p0 = src[2];
      _p1 = src[1];
      _p2 = src[0];
    }
  }

  B2D_INLINE const Point& first() const noexcept { return _p0; }
  B2D_INLINE const Point& last() const noexcept { return _p2; }

  B2D_INLINE bool canPop() const noexcept { return _stackPtr != _flattenData._stack; }
  B2D_INLINE bool canPush() const noexcept { return _stackPtr != _flattenData._stack + FlattenMonoData::kStackSizeQuad; }

  B2D_INLINE bool isLeftToRight() const noexcept { return first().x() < last().x(); }

  // Caused by floating point inaccuracy, we must bound the control
  // point as we really need monotonic curve that would never outbound
  // the boundary defined by its start/end points.
  B2D_INLINE void boundLeftToRight() noexcept {
    _p1._x = Math::bound(_p1.x(), _p0.x(), _p2.x());
    _p1._y = Math::bound(_p1.y(), _p0.y(), _p2.y());
  }

  B2D_INLINE void boundRightToLeft() noexcept {
    _p1._x = Math::bound(_p1.x(), _p2.x(), _p0.x());
    _p1._y = Math::bound(_p1.y(), _p0.y(), _p2.y());
  }

  B2D_INLINE bool isFlat(SplitStep& step) const noexcept {
    Point v1 = _p1 - _p0;
    Point v2 = _p2 - _p0;

    double d = Math::cross(v2, v1);
    double lenSq = Math::lengthSq(v2);

    step.value = d * d;
    step.limit = _flattenData._toleranceSq * lenSq;

    return step.value <= step.limit;
  }

  B2D_INLINE void split(SplitStep& step) const noexcept {
    step.p01 = (_p0 + _p1) * 0.5;
    step.p12 = (_p1 + _p2) * 0.5;
    step.p012 = (step.p01 + step.p12) * 0.5;
  }

  B2D_INLINE void push(const SplitStep& step) noexcept {
    // Must be checked before calling `push()`.
    B2D_ASSERT(canPush());

    _stackPtr[0].reset(step.p012);
    _stackPtr[1].reset(step.p12);
    _stackPtr[2].reset(_p2);
    _stackPtr += 3;

    _p1 = step.p01;
    _p2 = step.p012;
  }

  B2D_INLINE void discardAndAdvance(const SplitStep& step) noexcept {
    _p0 = step.p012;
    _p1 = step.p12;
  }

  B2D_INLINE void pop() noexcept {
    _stackPtr -= 3;
    _p0 = _stackPtr[0];
    _p1 = _stackPtr[1];
    _p2 = _stackPtr[2];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FlattenMonoData& _flattenData;
  Point* _stackPtr;
  Point _p0, _p1, _p2;
};

// ============================================================================
// [b2d::RasterEngine::FlattenMonoCubic]
// ============================================================================

//! \internal
//!
//! Helper to flatten a monotonic cubic curve.
class FlattenMonoCubic {
public:
  struct SplitStep {
    B2D_INLINE bool isFinite() const noexcept { return std::isfinite(value); }
    B2D_INLINE const Point& midPoint() const noexcept { return p0123; }

    double value;
    double limit;

    Point p01;
    Point p12;
    Point p23;
    Point p012;
    Point p123;
    Point p0123;
  };

  B2D_INLINE explicit FlattenMonoCubic(FlattenMonoData& flattenData) noexcept
    : _flattenData(flattenData) {}

  B2D_INLINE void begin(const Point* src, uint32_t signBit) noexcept {
    _stackPtr = _flattenData._stack;

    if (signBit == 0) {
      _p0 = src[0];
      _p1 = src[1];
      _p2 = src[2];
      _p3 = src[3];
    }
    else {
      _p0 = src[3];
      _p1 = src[2];
      _p2 = src[1];
      _p3 = src[0];
    }
  }

  B2D_INLINE const Point& first() const noexcept { return _p0; }
  B2D_INLINE const Point& last() const noexcept { return _p3; }

  B2D_INLINE bool canPop() const noexcept { return _stackPtr != _flattenData._stack; }
  B2D_INLINE bool canPush() const noexcept { return _stackPtr != _flattenData._stack + FlattenMonoData::kStackSizeCubic; }

  B2D_INLINE bool isLeftToRight() const noexcept { return first().x() < last().x(); }

  // Caused by floating point inaccuracy, we must bound the control
  // point as we really need monotonic curve that would never outbound
  // the boundary defined by its start/end points.
  B2D_INLINE void boundLeftToRight() noexcept {
    _p1._x = Math::bound(_p1.x(), _p0.x(), _p3.x());
    _p1._y = Math::bound(_p1.y(), _p0.y(), _p3.y());
    _p2._x = Math::bound(_p2.x(), _p0.x(), _p3.x());
    _p2._y = Math::bound(_p2.y(), _p0.y(), _p3.y());
  }

  B2D_INLINE void boundRightToLeft() noexcept {
    _p1._x = Math::bound(_p1.x(), _p3.x(), _p0.x());
    _p1._y = Math::bound(_p1.y(), _p0.y(), _p3.y());
    _p2._x = Math::bound(_p2.x(), _p3.x(), _p0.x());
    _p2._y = Math::bound(_p2.y(), _p0.y(), _p3.y());
  }

  B2D_INLINE bool isFlat(SplitStep& step) const noexcept {
    Point v = _p3 - _p0;

    double d1Sq = Math::pow2(Math::cross(v, _p1 - _p0));
    double d2Sq = Math::pow2(Math::cross(v, _p2 - _p0));
    double lenSq = Math::lengthSq(v);

    step.value = Math::max(d1Sq, d2Sq);
    step.limit = _flattenData._toleranceSq * lenSq;

    return step.value <= step.limit;
  }

  B2D_INLINE void split(SplitStep& step) const noexcept {
    step.p01   = (_p0 + _p1) * 0.5;
    step.p12   = (_p1 + _p2) * 0.5;
    step.p23   = (_p2 + _p3) * 0.5;
    step.p012  = (step.p01  + step.p12 ) * 0.5;
    step.p123  = (step.p12  + step.p23 ) * 0.5;
    step.p0123 = (step.p012 + step.p123) * 0.5;
  }

  B2D_INLINE void push(const SplitStep& step) noexcept {
    // Must be checked before calling `push()`.
    B2D_ASSERT(canPush());

    _stackPtr[0].reset(step.p0123);
    _stackPtr[1].reset(step.p123);
    _stackPtr[2].reset(step.p23);
    _stackPtr[3].reset(_p3);
    _stackPtr += 4;

    _p1 = step.p01;
    _p2 = step.p012;
    _p3 = step.p0123;
  }

  B2D_INLINE void discardAndAdvance(const SplitStep& step) noexcept {
    _p0 = step.p0123;
    _p1 = step.p123;
    _p2 = step.p23;
  }

  B2D_INLINE void pop() noexcept {
    _stackPtr -= 4;
    _p0 = _stackPtr[0];
    _p1 = _stackPtr[1];
    _p2 = _stackPtr[2];
    _p3 = _stackPtr[3];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FlattenMonoData& _flattenData;
  Point* _stackPtr;
  Point _p0, _p1, _p2, _p3;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_EDGEFLATTEN_P_H
