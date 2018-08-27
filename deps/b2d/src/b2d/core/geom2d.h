// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_GEOM2D_H
#define _B2D_CORE_GEOM2D_H

// [Dependencies]
#include "../core/contextdefs.h"
#include "../core/fill.h"
#include "../core/geomtypes.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [Forward Declarations]
// ============================================================================

class Path2D;

// ============================================================================
// [b2d::Geom2D]
// ============================================================================

namespace Geom2D {

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Geom2D - Half]
// ============================================================================

static B2D_INLINE Point half(const Point& a, const Point& b) noexcept {
  double x = (a._x * 0.5) + (b._x * 0.5);
  double y = (a._y * 0.5) + (b._y * 0.5);

  return Point(x, y);
}

static B2D_INLINE Size half(const Size& a, const Size& b) noexcept {
  double w = (a._w * 0.5) + (b._w * 0.5);
  double h = (a._h * 0.5) + (b._h * 0.5);

  return Size(w, h);
}

// ============================================================================
// [b2d::Geom2D - CrossProduct]
// ============================================================================

static B2D_INLINE double crossProduct(const Point& p0, const Point& p1, const Point& p2) noexcept {
  return (p2._x - p1._x) * (p1._y - p0._y) - (p2._y - p1._y) * (p1._x - p0._x);
}

// ============================================================================
// [b2d::Geom2D - Line]
// ============================================================================

static B2D_INLINE bool intersectLines(const Point& a0, const Point& a1, const Point& b0, const Point& b1, Point& dst) noexcept {
  double n = (a0._y - b0._y) * (b1._x - b0._x) - (a0._x - b0._x) * (b1._y - b0._y);
  double d = (a1._x - a0._x) * (b1._y - b0._y) - (a1._y - a0._y) * (b1._x - b0._x);

  if (std::abs(d) < Math::kIntersectionEpsilon)
    return false;

  dst._x = a0._x + (a1._x - a0._x) * n / d;
  dst._y = a0._y + (a1._y - a0._y) * n / d;

  return true;
}

// ============================================================================
// [b2d::Geom2D - Quad / Cubic]
// ============================================================================

static B2D_INLINE void quadCoefficients(double t, double& a, double& b, double& c) noexcept {
  double ti = 1.0 - t;

  a = ti * ti;
  b = 2.0 * (t - ti);
  c = t * t;
}

B2D_API Error quadBounds(const Point bez[3], Box& bBox) noexcept;

static B2D_INLINE Error quadBounds(const Point bez[3], Rect& bRect) noexcept {
  Error e = quadBounds(bez, reinterpret_cast<Box&>(bRect));
  bRect._w -= bRect._x;
  bRect._h -= bRect._y;
  return e;
}

static B2D_INLINE void extractQuadPolynomial(const Point bez[3],
  double& ax, double& ay,
  double& bx, double& by,
  double& cx, double& cy) noexcept {

  cx = bez[0]._x;
  cy = bez[0]._y;

  ax = cx - 2.0 * bez[1]._x + bez[2]._x;
  ay = cy - 2.0 * bez[1]._y + bez[2]._y;

  bx = 2.0 * (bez[1]._x - bez[0]._x);
  by = 2.0 * (bez[1]._y - bez[0]._y);
}

static B2D_INLINE void extractQuadPolynomial(const Point bez[3], Point& a, Point& b, Point& c) noexcept {
  c = bez[0];
  a = bez[0] - bez[1] * 2.0 + bez[2];
  b = (bez[1] - bez[0]) * 2.0;
}

static B2D_INLINE Point evalQuad(const Point bez[3], double t) noexcept {
  double a, b, c;
  quadCoefficients(t, a, b, c);

  return Point(a * bez[0]._x + b * bez[1]._x + c * bez[2]._x,
               a * bez[0]._y + b * bez[1]._y + c * bez[2]._y);
}

static B2D_INLINE void subdivQuad(const Point bez[3], Point a[3], Point b[3]) noexcept {
  Point p01(half(bez[0], bez[1]));
  Point p12(half(bez[1], bez[2]));

  a[0] = bez[0];
  a[1] = p01;
  b[1] = p12;
  b[2] = bez[2];

  a[2] = half(p01, p12);
  b[0] = a[2];
}

static B2D_INLINE void splitQuad(const Point bez[3], Point a[3], Point b[3], double t) noexcept {
  Point p01(Math::lerp(bez[0], bez[1], t));
  Point p12(Math::lerp(bez[1], bez[2], t));

  a[0] = bez[0];
  a[1] = p01;
  b[1] = p12;
  b[2] = bez[2];

  a[2] = Math::lerp(p01, p12, t);
  b[0] = a[2];
}

B2D_API Error closestToQuad(const Point bez[3], Point& p, double& t) noexcept;
B2D_API double lengthOfQuad(const Point bez[3]) noexcept;

// Cubic Parameters
// ----------------
//
// A =   -p0 + 3*p1 - 3*p2 + p3 == -p0 + 3*(p1 -   p2) + p3
// B =  3*p0 - 6*p1 + 3*p2      ==       3*(p0 - 2*p2  + p3)
// C = -3*p0 + 3*p1             ==       3*(p1 -   p0)
// D =    p0                    ==  p0
//
// Evaluation at `t`
// -----------------
//
// Value = At^3 + Bt^2 + Ct + D
// Value = t(t(At + B) + C) + D

static B2D_INLINE void cubicCoefficients(double t, double& a, double& b, double& c, double& d) noexcept {
  double ti = 1.0 - t;
  double t2 = t * t;
  double ti2 = ti * ti;

  a = ti * ti2;
  b = 3.0 * t * ti2;
  c = 3.0 * ti * t2;
  d = t * t2;
}

B2D_API Error cubicBounds(const Point bez[4], Box& bBox) noexcept;

static B2D_INLINE Error cubicBounds(const Point bez[4], Rect& bRect) noexcept {
  Error e = cubicBounds(bez, reinterpret_cast<Box&>(bRect));
  bRect._w -= bRect._x;
  bRect._h -= bRect._y;
  return e;
}

static B2D_INLINE void extractCubicPolynomial(const Point bez[4],
  double& ax, double& ay,
  double& bx, double& by,
  double& cx, double& cy,
  double& dx, double& dy) noexcept {

  dx = bez[0]._x;
  dy = bez[0]._y;

  ax = -dx + 3.0 * (bez[1]._x - bez[2]._x) + bez[3]._x;
  ay = -dy + 3.0 * (bez[1]._y - bez[2]._y) + bez[3]._y;

  bx = 3.0 * (dx - 2.0 * bez[1]._x + bez[2]._x);
  by = 3.0 * (dy - 2.0 * bez[1]._y + bez[2]._y);

  cx = 3.0 * (bez[1]._x - bez[0]._x);
  cy = 3.0 * (bez[1]._y - bez[0]._y);
}

static B2D_INLINE void extractCubicPolynomial(const Point bez[4], Point& a, Point& b, Point& c, Point& d) noexcept {
  d = bez[0];
  a = (bez[1] - bez[2]) * 3.0 + bez[3] - d;
  b = (bez[0] - bez[1]  * 2.0 + bez[2]) * 3.0;
  c = (bez[1] - bez[0]) * 3.0;
}

static B2D_INLINE void extractCubicDerivative(const Point bez[4], Point& a, Point& b, Point& c) noexcept {
  a = ((bez[1] - bez[2]) * 3.0 + bez[3] - bez[0]) * 3.0;
  b = (bez[2] - bez[1] * 2.0 + bez[0]) * 6.0;
  c = (bez[1] - bez[0]) * 3.0;
}

static B2D_INLINE Point evalCubic(const Point bez[4], double t) noexcept {
  double tInv = 1.0 - t;

  double ax = bez[0]._x * tInv + bez[1]._x * t;
  double ay = bez[0]._y * tInv + bez[1]._y * t;

  double bx = bez[1]._x * tInv + bez[2]._x * t;
  double by = bez[1]._y * tInv + bez[2]._y * t;

  double cx = bez[2]._x * tInv + bez[3]._x * t;
  double cy = bez[2]._y * tInv + bez[3]._y * t;

  return Point((ax * tInv + bx * t) * tInv + (bx * tInv + cx * t) * t,
               (ay * tInv + by * t) * tInv + (by * tInv + cy * t) * t);
}

static B2D_INLINE void subdivCubic(const Point bez[4], Point a[4], Point b[4]) noexcept {
  Point p01(half(bez[0], bez[1]));
  Point p12(half(bez[1], bez[2]));
  Point p23(half(bez[2], bez[3]));

  a[0] = bez[0];
  a[1] = p01;

  b[2] = p23;
  b[3] = bez[3];

  a[2] = half(p01, p12);
  b[1] = half(p12, p23);

  a[3] = half(a[2], b[1]);
  b[0] = a[3];
}

static B2D_INLINE void splitCubic(const Point bez[4], Point a[4], Point b[4], double t) noexcept {
  Point p01(Math::lerp(bez[0], bez[1], t));
  Point p12(Math::lerp(bez[1], bez[2], t));
  Point p23(Math::lerp(bez[2], bez[3], t));

  a[0] = bez[0];
  a[1] = p01;

  b[2] = p23;
  b[3] = bez[3];

  a[2] = Math::lerp(p01, p12, t);
  b[1] = Math::lerp(p12, p23, t);

  a[3] = Math::lerp(a[2], b[1], t);
  b[0] = a[3];
}

static B2D_INLINE void leftCubic(const Point bez[4], Point a[4], double t) noexcept {
  Point p01(Math::lerp(bez[0], bez[1], t));
  Point p12(Math::lerp(bez[1], bez[2], t));
  Point p23(Math::lerp(bez[2], bez[3], t));

  a[0] = bez[0];
  a[1] = p01;
  a[2] = Math::lerp(p01, p12, t);
  a[3] = Math::lerp(a[2], Math::lerp(p12, p23, t), t);
}

static B2D_INLINE void rightCubic(const Point bez[4], Point b[4], double t) noexcept {
  Point p01(Math::lerp(bez[0], bez[1], t));
  Point p12(Math::lerp(bez[1], bez[2], t));
  Point p23(Math::lerp(bez[2], bez[3], t));

  b[3] = bez[3];
  b[2] = p23;
  b[1] = Math::lerp(p12, p23, t);
  b[0] = Math::lerp(Math::lerp(p01, p12, t), b[1], t);
}

//! Covert quad to cubic.
//!
//! \code
//! c0 = q0
//! c1 = q0 + 2/3 * (q1 - q0)
//! c2 = q2 + 2/3 * (q1 - q2)
//! c3 = q2
//! \endcode
static B2D_INLINE void cubicFromQuad(Point bez[4], const Point q[3]) noexcept {
  double q1x23 = q[1]._x * Math::k2Div3;
  double q1y23 = q[1]._y * Math::k2Div3;

  bez[0] = q[0];
  bez[3] = q[2];
  bez[1].reset(q[0]._x * Math::k1Div3 + q1x23, q[0]._y * Math::k1Div3 + q1y23);
  bez[2].reset(q[2]._x * Math::k1Div3 + q1x23, q[2]._y * Math::k1Div3 + q1y23);
}

static B2D_INLINE Point cubicMidPoint(const Point bez[4]) noexcept {
  return Point(
    (1.0 / 8.0) * (bez[0]._x + bez[3]._x + 3.0 * (bez[1]._x + bez[2]._x)),
    (1.0 / 8.0) * (bez[0]._y + bez[3]._y + 3.0 * (bez[1]._y + bez[2]._y)));
}

B2D_API size_t cubicInflection(const Point bez[4], double* t) noexcept;
B2D_API size_t cubicSimplified(const Point bez[4], Point* dst) noexcept;

B2D_API Error closestToCubic(const Point bez[4], Point& p, double& t) noexcept;
B2D_API double lengthOfCubic(const Point bez[4]) noexcept;

// ============================================================================
// [b2d::Geom2D - HitTest]
// ============================================================================

//! \internal
B2D_API bool _hitTest(const Point& p, uint32_t id, const void* data, uint32_t fillRule) noexcept;

static B2D_INLINE bool hitTest(const Point& p, const Box& box) noexcept {
  return _hitTest(p, kGeomArgBox, &box, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const IntBox& box) noexcept {
  return _hitTest(p, kGeomArgIntBox, &box, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Rect& rect) noexcept {
  return _hitTest(p, kGeomArgRect, &rect, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const IntRect& rect) noexcept {
  return _hitTest(p, kGeomArgIntRect, &rect, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Round& round) noexcept {
  return _hitTest(p, kGeomArgRound, &round, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Circle& circle) noexcept {
  return _hitTest(p, kGeomArgCircle, &circle, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Ellipse& ellipse) noexcept {
  return _hitTest(p, kGeomArgEllipse, &ellipse, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Chord& chord) noexcept {
  return _hitTest(p, kGeomArgChord, &chord, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Pie& pie) noexcept {
  return _hitTest(p, kGeomArgPie, &pie, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Triangle& triangle) noexcept {
  return _hitTest(p, kGeomArgTriangle, &triangle, FillRule::kDefault);
}

static B2D_INLINE bool hitTest(const Point& p, const Path2D& path, uint32_t fillRule = FillRule::kDefault) noexcept {
  return _hitTest(p, kGeomArgPath2D, &path, fillRule);
}

//! \}

} // Geom2D namespace

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_GEOM2D_H
