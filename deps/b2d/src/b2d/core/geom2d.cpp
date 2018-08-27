// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/geom2d.h"
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/math_integrate.h"
#include "../core/math_roots.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Geom2D - Quad / Cubic]
// ============================================================================

Error Geom2D::quadBounds(const Point bez[3], Box& bBox) noexcept {
  // Get bounding box of start and end points.
  Box box(bez[0].x(), bez[0].y(), bez[0].x(), bez[0].y());
  box.bound(bez[2]);

  // Merge X extrema.
  if (bez[1].x() < box.x0() || bez[1].x() > box.x1()) {
    double t = (bez[0].x() - bez[1].x()) / (bez[0].x() - 2.0 * bez[1].x() + bez[2].x());
    double a, b, c;

    quadCoefficients(t, a, b, c);
    box.boundX(a * bez[0].x() + b * bez[1].x() + c * bez[2].x());
  }

  // Merge Y extrema.
  if (bez[1].y() < box.y0() || bez[1].y() > box.y1()) {
    double t = (bez[0].y() - bez[1].y()) / (bez[0].y() - 2.0 * bez[1].y() + bez[2].y());
    double a, b, c;

    quadCoefficients(t, a, b, c);
    box.boundY(a * bez[0].y() + b * bez[1].y() + c * bez[2].y());
  }

  bBox.reset(box);
  return kErrorOk;
}

Error Geom2D::closestToQuad(const Point bez[3], Point& p, double& t) noexcept {
  Point pClosest(bez[0]);

  Point a(bez[1] - bez[0]);
  Point b(bez[0] - bez[1] * 2.0 + bez[2]);
  Point q(bez[0] - p);

  double tClosest = 0.0;
  double dClosest = Math::lengthSq(bez[0], p);
  double distance = Math::lengthSq(bez[2], p);

  if (distance < dClosest) {
    pClosest = bez[2];
    tClosest = 1.0;
    dClosest = distance;
  }

  double poly[4];
  double root[3];

  poly[0] =       (b.x() * b.x() + b.y() * b.y());
  poly[1] = 3.0 * (a.x() * b.x() + a.y() * b.y());
  poly[2] = 2.0 * (a.x() * a.x() + a.y() * a.y()) + (b.x() * q.x() + b.y() * q.y());
  poly[3] =       (a.x() * q.x() + a.y() * q.y());

  size_t tCount = Math::cubicRoots(root, poly, 0.0, 1.0);
  for (size_t i = 0; i < tCount; i++) {
    q = evalQuad(bez, root[i]);
    distance = Math::lengthSq(q, p);

    if (distance < dClosest) {
      pClosest = q;
      tClosest = root[i];
      dClosest = distance;
    }
  }

  p = pClosest;
  t = tClosest;
  return kErrorOk;
}

double Geom2D::lengthOfQuad(const Point bez[3]) noexcept {
  // Implementation based on segfaultlabs.com 'Quadratic Bezier Curve Length':
  //   http://segfaultlabs.com/docs/quadratic-bezier-curve-length
  double ax = (bez[0].x() + bez[2].x() - bez[1].x() * 2.0);
  double ay = (bez[0].y() + bez[2].y() - bez[1].y() * 2.0);

  double bx = (bez[1].x() - bez[0].x()) * 2.0;
  double by = (bez[1].y() - bez[0].y()) * 2.0;

  double a = (ax * ax + ay * ay) * 4.0;
  double b = (ax * bx + ay * by) * 4.0;
  double c = (bx * bx + by * by);

  double Sabc = std::sqrt(a + b + c);
  double Sa   = std::sqrt(a);
  double Sc   = std::sqrt(c);

  double bDivSa = b / Sa;
  double logPart = std::log((2.0 * (Sabc + Sa) + bDivSa) / (2.0 * Sc + bDivSa));

  return (2.0 * Sa * (Sabc * (2.0 * a + b) - b * Sc) + (4.0 * a * c - b * b) * logPart) / (8.0 * Sa * a);
}

// TODO: Rework, use inlined Math::quadRoots instead, verify it's correct.
Error Geom2D::cubicBounds(const Point bez[4], Box& bBox) noexcept {
  // Get bounding box of start and end points.
  Box box(bez[0].x(), bez[0].y(), bez[0].x(), bez[0].y());
  box.bound(bez[3]);

  // X extrema.
  double a = 3.0 * (-bez[0].x() + 3.0 * (bez[1].x() - bez[2].x()) + bez[3].x());
  double b = 6.0 * ( bez[0].x() - 2.0 *  bez[1].x() + bez[2].x()              );
  double c = 3.0 * (-bez[0].x() +        bez[1].x()                           );
  double d;

  for (int i = 0;;) {
    double t0;
    double t1;

    // Catch the A and B near zero.
    if (Math::isNearZero(a)) {
      // A~=0 && B~=0.
      if (!Math::isNearZero(b)) {
        // Simple case (t = -c / b).
        t0 = -c / b;
      }
      else {
        goto _Skip;
      }
    }
    // Calculate roots (t = b^2 - 4ac).
    else if ((t0 = b * b - 4.0 * a * c) > 0.0) {
      d = std::sqrt(t0);
      d = -0.5 * (b + (b < 0.0 ? -d : d));

      t0 = d / a;
      t1 = c / d;

      if (t1 > 0.0 && t1 < 1.0) {
        cubicCoefficients(t1, a, b, c, d);

        box.bound(a * bez[0].x() + b * bez[1].x() + c * bez[2].x() + d * bez[3].x(),
                  a * bez[0].y() + b * bez[1].y() + c * bez[2].y() + d * bez[3].y());
      }
    }
    else {
      goto _Skip;
    }

    if (t0 > 0.0 && t0 < 1.0) {
      cubicCoefficients(t0, a, b, c, d);
      box.bound(a * bez[0].x() + b * bez[1].x() + c * bez[2].x() + d * bez[3].x(),
                a * bez[0].y() + b * bez[1].y() + c * bez[2].y() + d * bez[3].y());
    }

_Skip:
    if (++i == 2)
      break;

    // Y extrema.
    a = 3.0 * (-bez[0].y() + 3.0 * (bez[1].y() - bez[2].y()) + bez[3].y());
    b = 6.0 * ( bez[0].y() - 2.0 *  bez[1].y() + bez[2].y()              );
    c = 3.0 * (-bez[0].y() +        bez[1].y()                           );
  }

  bBox.reset(box);
  return kErrorOk;
}

Error Geom2D::closestToCubic(const Point bez[4], Point& p, double& t) noexcept {
  Point pClosest(bez[0]);

  Point b(3.0 * (bez[2].x() - bez[1].x()),
          3.0 * (bez[2].y() - bez[1].y()));
  Point c(3.0 * (bez[1].x() - bez[0].x()),
          3.0 * (bez[1].y() - bez[0].y()));
  Point a(bez[3].x() - bez[0].x() - b.x(),
          bez[3].y() - bez[0].y() - b.y());

  b._x -= c.x();
  b._y -= c.y();

  Point q = bez[0] - p;
  double tClosest = 0.0;
  double dClosest = Math::lengthSq(bez[0], p);
  double distance = Math::lengthSq(bez[3], p);

  if (distance < dClosest) {
    pClosest = bez[3];
    tClosest = 1.0;
    dClosest = distance;
  }

  double poly[6];
  double root[5];

  poly[0] = 3.0 * (a.x() * a.x() + a.y() * a.y());
  poly[1] = 5.0 * (a.x() * b.x() + a.y() * b.y());
  poly[2] = 4.0 * (a.x() * c.x() + a.y() * c.y()) + 2.0 * (b.x() * b.x() + b.y() * b.y());
  poly[3] = 3.0 * (b.x() * c.x() + b.y() * c.y()) + 3.0 * (a.x() * q.x() + a.y() * q.y());
  poly[4] =       (c.x() * c.x() + c.y() * c.y()) + 2.0 * (b.x() * q.x() + b.y() * q.y());
  poly[5] =       (c.x() * q.x() + c.y() * q.y());

  size_t tCount = Math::polyRoots(root, poly, 5, 0.0, 1.0);
  for (size_t i = 0 ; i < tCount; i++) {
    q = evalCubic(bez, root[i]);
    distance = Math::lengthSq(q, p);

    if (distance < dClosest) {
      pClosest = q;
      tClosest = root[i];
      dClosest = distance;
    }
  }

  p = pClosest;
  t = tClosest;
  return kErrorOk;
}

struct Geom2DCubicLengthFunction {
  static Error B2D_CDECL eval(void* _ctx, double* dst, const double* t, size_t n) noexcept {
    Geom2DCubicLengthFunction* ctx = static_cast<Geom2DCubicLengthFunction*>(_ctx);

    double ax = ctx->ax, ay = ctx->ay;
    double bx = ctx->bx, by = ctx->by;
    double cx = ctx->cx, cy = ctx->cy;

    for (size_t i = 0; i < n; i++) {
      double tVal = t[i];
      double tInv = 1.0 - tVal;
      double tPow = tVal * tVal;

      // A(1 - t)^2 + Bt(1 - t) + Ct^2
      double x = (ax * tInv + bx * tVal) * tInv + cx * tPow;
      double y = (ay * tInv + by * tVal) * tInv + cy * tPow;

      dst[i] = std::hypot(x, y);
    }

    return kErrorOk;
  }

  double ax, ay;
  double bx, by;
  double cx, cy;
};

double Geom2D::lengthOfCubic(const Point bez[4]) noexcept {
  Geom2DCubicLengthFunction func;

  func.ax = 3.0 * (bez[1].x() - bez[0].x());
  func.ay = 3.0 * (bez[1].y() - bez[0].y());
  func.bx = 6.0 * (bez[2].x() - bez[1].x());
  func.by = 6.0 * (bez[2].y() - bez[1].y());
  func.cx = 3.0 * (bez[3].x() - bez[2].x());
  func.cy = 3.0 * (bez[3].y() - bez[2].y());

  double result;
  if (Math::integrate(result, Geom2DCubicLengthFunction::eval, &func, 0.0, 1.0, 4) != kErrorOk)
    return std::numeric_limits<double>::quiet_NaN();
  else
    return result;
}

size_t Geom2D::cubicInflection(const Point bez[4], double* t) noexcept {
  double ax, ay, bx, by, cx, cy, dx, dy;
  double poly[3];

  extractCubicPolynomial(bez, ax, ay, bx, by, cx, cy, dx, dy);
  poly[0] = 6.0 * (ay * bx - ax * by);
  poly[1] = 6.0 * (ay * cx - ax * cy);
  poly[2] = 2.0 * (by * cx - bx * cy);
  return Math::quadRoots(t, poly, Math::kAfter0, Math::kBefore1);
}

// Based on paper Precise Flattening of Cubic Bézier Segments by T.Hain et al.
//
// This function is designed to pre-process one Bézier curve into a B-Spline
// that doesn't have inflection point. This function can be also used as a first
// step to accomplish requirements for flattening using parabolic approximation
// or before stroking.
size_t Geom2D::cubicSimplified(const Point bez[4], Point* dst) noexcept {
  double ax, ay, bx, by, cx, cy, dx, dy;
  double q[3];
  double t[3];

  extractCubicPolynomial(bez, ax, ay, bx, by, cx, cy, dx, dy);
  q[0] = (ay * bx - ax * by); // qa.
  q[1] = (ay * cx - ax * cy); // qb.
  q[2] = (by * cx - bx * cy); // qc.

  // tCusp = -0.5 * ((ay * cx - ax * cy) / (ay * bx - ax * by)).
  // tCusp = -0.5 * (qb / qa)
  double tCusp = -0.5 * (q[1] / q[0]);

  // Solve the quadratic function so we can find the inflection points.
  q[0] *= 6.0;
  q[1] *= 6.0;
  q[2] *= 2.0;

  size_t tCount = Math::quadRoots(t, q, Math::kAfter0, Math::kBefore1);
  if (tCusp > 0.0 && tCusp < 1.0) {
    t[tCount++] = tCusp;
    Support::iSort<double>(t, tCount);
  }

  if (tCount == 0) {
    dst[0] = bez[0];
    dst[1] = bez[1];
    dst[2] = bez[2];
    dst[3] = bez[3];
    return 1;
  }

  size_t segmentCount = 2;
  double t_ = t[0];
  double cut = t_;

  splitCubic(bez, dst, dst + 3, t_);
  dst += 3;

  for (int tIndex = 1; tIndex < tCount; tIndex++) {
    t_ = t[tIndex];
    if (Math::isNear(t_, cut))
      continue;

    splitCubic(dst, dst, dst + 3, (t_ - cut) / (1.0 - cut));
    cut = t_;

    dst += 3;
    segmentCount++;
  }

  return segmentCount;
}

// ===========================================================================
// [b2d::Geom2D - HitTest]
// ===========================================================================

template<typename BoxT>
static B2D_INLINE bool bHitTestBox(const Point& p, const BoxT& box) noexcept {
  double x0 = double(box.x0());
  double y0 = double(box.y0());
  double x1 = double(box.x1());
  double y1 = double(box.y1());

  return p.x() >= x0 && p.y() >= y0 && p.x() < x1 && p.y() < y1;
}

template<typename RectT>
static B2D_INLINE bool bHitTestRect(const Point& p, const RectT& rect) noexcept {
  double x0 = double(rect.x());
  double y0 = double(rect.y());
  double x1 = x0 + double(rect.w());
  double y1 = y0 + double(rect.h());

  return p.x() >= x0 && p.y() >= y0 && p.x() < x1 && p.y() < y1;
}

// Hit-test optimized so it doesn't need square root.
static B2D_INLINE bool bHitTestCircle(const Point& p, const Circle& circle) noexcept {
  double x = p.x() - circle._center.x();
  double y = p.y() - circle._center.y();
  double r = circle._radius;

  double maxDist = Math::pow2(r);
  double ptDist  = Math::pow2(x) + Math::pow2(y);

  return ptDist <= maxDist;
}

// Point-in-ellipse problem reduced to point-in-circle problem. It is always
// possible to scale an ellipse to get a circle. This trick is used to get
// the hit-test without using too much calculations.
static B2D_INLINE bool bHitTestEllipse(const Point& p, const Ellipse& ellipse) noexcept {
  double rx = std::abs(ellipse._radius.x());
  double ry = std::abs(ellipse._radius.y());

  double x = p.x() - ellipse._center.x();
  double y = p.y() - ellipse._center.y();

  // No radius or very close to zero (can't hit-test in such case).
  if (Math::isNearZero(rx) || Math::isNearZero(ry))
    return false;

  if (rx > ry) {
    y = y * rx / ry;
  }
  else if (rx < ry) {
    x = x * ry / rx;
    rx = ry;
  }

  double ptDist = Math::pow2(x) + Math::pow2(y);
  double maxDist = Math::pow2(rx);

  return ptDist <= maxDist;
}

static B2D_INLINE bool bHitTestRound(const Point& p, const Round& round) noexcept {
  double x = p.x() - round._rect.x();
  double y = p.y() - round._rect.y();

  double w = round._rect.w();
  double h = round._rect.h();

  double rx = std::abs(round._radius.x());
  double ry = std::abs(round._radius.y());

  // Hit-test the bounding box.
  if (x < 0.0 || y < 0.0 || x >= w || y >= h)
    return false;

  // Normalize rx/ry.
  rx = Math::min(rx, w * 2.0);
  ry = Math::min(ry, h * 2.0);

  // Hit-test the inner two boxes.
  if (x >= rx && x <= w - rx) return true;
  if (y >= ry && y <= h - ry) return true;

  // Hit-test the four symmetric rounded corners. There are used the same
  // trick as in ellipse hit-test (the elliptic corners are scaled to be
  // circular).
  x -= rx;
  y -= ry;

  if (x >= rx) x -= round._rect._w - rx - rx;
  if (y >= ry) y -= round._rect._h - ry - ry;

  // No radius or very close to zero (positive hit-test).
  if (Math::isNearZero(rx) || Math::isNearZero(ry))
    return true;

  if (rx > ry) {
    y = y * rx / ry;
  }
  else if (rx < ry) {
    x = x * ry / rx;
    rx = ry;
  }

  double ptDist = Math::pow2(x) + Math::pow2(y);
  double maxDist = Math::pow2(rx);

  return ptDist <= maxDist;
}

static B2D_INLINE bool bHitTestChord(const Point& p, const Arc& arc) noexcept {
  double cx = arc._center.x();
  double cy = arc._center.y();

  double rx = std::abs(arc._radius.x());
  double ry = std::abs(arc._radius.y());

  double x = p.x() - cx;
  double y = p.y() - cy;

  // No radius or very close to zero (can't hit-test in such case).
  if (Math::isNearZero(rx) || Math::isNearZero(ry))
    return false;

  // Transform an ellipse to a circle, transforming also the hit-tested point.
  if (rx > ry) {
    y = y * rx / ry;
  }
  else if (rx < ry) {
    x = x * ry / rx;
    rx = ry;
  }

  // Hit-test the ellipse.
  double ptDist = Math::pow2(x) + Math::pow2(y);
  double maxDist = Math::pow2(rx);

  if (ptDist > maxDist)
    return false;

  // Hit-test the rest.
  Point p0, p1;

  double start = arc._start;
  double sweep = arc._sweep;

  // Hit-test is always positive if `sweep` is larger than a `2*PI`.
  if (sweep < -Math::k2Pi || sweep > Math::k2Pi)
    return true;

  p0._y = std::sin(start) * rx;
  p0._x = std::cos(start) * rx;

  p1._y = std::sin(start + sweep) * rx;
  p1._x = std::cos(start + sweep) * rx;

  double dx = p1.x() - p0.x();
  double dy = p1.y() - p0.y();

  int onRight = sweep < 0.0;

  if (Math::isNearZero(dy)) {
    if (std::abs(sweep) < Math::kPiDiv2)
      return false;
    return onRight ? y <= p0.y() : y >= p0.y();
  }

  double lx = p0.x() + (y - p0.y()) * dx / dy;
  onRight ^= (dy > 0.0);

  if (onRight)
    return x >= lx;
  else
    return x < lx;
}

static B2D_INLINE bool bHitTestPie(const Point& p, const Arc& arc) noexcept {
  double cx = arc._center.x();
  double cy = arc._center.y();

  double rx = std::abs(arc._radius.x());
  double ry = std::abs(arc._radius.y());

  double x = p.x() - cx;
  double y = p.y() - cy;

  // No radius or very close to zero (can't hit-test in such case).
  if (Math::isNearZero(rx) || Math::isNearZero(ry))
    return false;

  // Transform an ellipse to a circle, transforming also the hit-tested point.
  if (rx > ry) {
    y = y * rx / ry;
  }
  else if (rx < ry) {
    x = x * ry / rx;
    rx = ry;
  }

  // Hit-test the ellipse.
  double ptDist = Math::pow2(x) + Math::pow2(y);
  double maxDist = Math::pow2(rx);

  if (ptDist > maxDist)
    return false;

  // Hit-test the rest.
  double start = arc._start;
  double sweep = arc._sweep;

  // Hit-test is always positive if `sweep` is larger than a `2PI`.
  if (sweep < -Math::k2Pi || sweep > Math::k2Pi)
    return true;

  if (sweep < 0.0) {
    start += sweep;
    sweep = -sweep;
  }

  if (start <= -Math::k2Pi || start >= Math::k2Pi) {
    start = std::fmod(start, Math::k2Pi);
  }

  double angle = Math::repeat(std::atan2(y, x) - start, Math::k2Pi);
  return angle <= sweep;
}

// Hit-test using barycentric technique.
static B2D_INLINE bool bHitTestTriangle(const Point& p, const Point* tri) noexcept {
  double v0x = tri[2].x() - tri[0].x();
  double v0y = tri[2].y() - tri[0].y();

  double v1x = tri[1].x() - tri[0].x();
  double v1y = tri[1].y() - tri[0].y();

  double v2x = p.x() - tri[0].x();
  double v2y = p.y() - tri[0].y();

  // Dot product.
  double d00 = v0x * v0x + v0y * v0y;
  double d01 = v0x * v1x + v0y * v1y;
  double d02 = v0x * v2x + v0y * v2y;

  double d11 = v1x * v1x + v1y * v1y;
  double d12 = v1x * v2x + v1y * v2y;

  // Barycentric coordinates.
  double dRcp = (d00 * d11 - d01 * d01);
  if (Math::isNearZero(dRcp))
    return false;

  double u = (d11 * d02 - d01 * d12) / dRcp;
  double v = (d00 * d12 - d01 * d02) / dRcp;

  return (u >= 0.0) & (v >= 0.0) & (u + v <= 1.0);
}

static B2D_INLINE bool bHitTestPath(const Point& p, const Path2D& path, uint32_t fillRule) noexcept {
  Path2DImpl* pathI = path.impl();
  size_t i = pathI->size();

  if (B2D_UNLIKELY(i == 0))
    return false;

  const uint8_t* cmdData = pathI->commandData();
  const Point* vtxData = pathI->vertexData();

  Point start;
  bool hasMoveTo = false;

  double px = p.x();
  double py = p.y();

  double x0, y0;
  double x1, y1;

  int windingNumber = 0;
  Wrap<Point> ptBuffer[8];

  do {
    switch (cmdData[0]) {
      case Path2D::kCmdMoveTo: {
        if (hasMoveTo) {
          x0 = vtxData[-1].x();
          y0 = vtxData[-1].y();
          x1 = start.x();
          y1 = start.y();

          hasMoveTo = false;
          goto _DoLine;
        }

        start = vtxData[0];

        cmdData++;
        vtxData++;
        i--;

        hasMoveTo = true;
        break;
      }

      case Path2D::kCmdLineTo: {
        if (B2D_UNLIKELY(!hasMoveTo))
          goto _Invalid;

        x0 = vtxData[-1].x();
        y0 = vtxData[-1].y();
        x1 = vtxData[0].x();
        y1 = vtxData[0].y();

        cmdData++;
        vtxData++;
        i--;

_DoLine:
        {
          double dx = x1 - x0;
          double dy = y1 - y0;

          if (dy > 0.0) {
            if (py >= y0 && py < y1) {
              double ix = x0 + (py - y0) * dx / dy;
              windingNumber += (px >= ix);
            }
          }
          else if (dy < 0.0) {
            if (py >= y1 && py < y0) {
              double ix = x0 + (py - y0) * dx / dy;
              windingNumber -= (px >= ix);
            }
          }
        }
        break;
      }

      case Path2D::kCmdQuadTo: {
        B2D_ASSERT(hasMoveTo);
        B2D_ASSERT(i >= 2);

        const Point* p = vtxData - 1;
        if (B2D_UNLIKELY(!hasMoveTo))
          goto _Invalid;

        double minY = Math::min(p[0].y(), Math::min(p[1].y(), p[2].y()));
        double maxY = Math::max(p[0].y(), Math::max(p[1].y(), p[2].y()));

        cmdData += 2;
        vtxData += 2;
        i -= 2;

        if (py >= minY && py <= maxY) {
          bool degenerated = Math::isNear(p[0].y(), p[1].y()) &&
                             Math::isNear(p[1].y(), p[2].y()) ;

          if (degenerated) {
            x0 = p[0].x();
            y0 = p[0].y();
            x1 = p[2].x();
            y1 = p[2].y();
            goto _DoLine;
          }

          // Subdivide curve to curve-spline separated at Y-extrama.
          Point* left = (Point*)ptBuffer;
          Point* rght = (Point*)ptBuffer + 3;

          double tExtrema[2];
          double tCut = 0.0;

          tExtrema[0] = (p[0].y() - p[1].y()) / (p[0].y() - 2.0 * p[1].y() + p[2].y());

          int tIndex;
          int tLength = tExtrema[0] > 0.0 && tExtrema[0] < 1.0;

          tExtrema[tLength++] = 1.0;

          rght[0] = p[0];
          rght[1] = p[1];
          rght[2] = p[2];

          for (tIndex = 0; tIndex < tLength; tIndex++) {
            double tVal = tExtrema[tIndex];
            if (tVal == tCut) continue;

            if (tVal == 1.0) {
              left[0] = rght[0];
              left[1] = rght[1];
              left[2] = rght[2];
            }
            else {
              Geom2D::splitQuad(rght, left, rght, tCut == 0.0 ? tVal : (tVal - tCut) / (1.0 - tCut));
            }

            minY = Math::min(left[0].y(), left[2].y());
            maxY = Math::max(left[0].y(), left[2].y());

            if (py >= minY && py < maxY) {
              double ax, ay;
              double bx, by;
              double cx, cy;
              Geom2D::extractQuadPolynomial(left, ax, ay, bx, by, cx, cy);

              double poly[3] = { ay, by, cy - py };
              int dir = 0;

              if (left[0].y() < left[2].y())
                dir = 1;
              else if (left[0].y() > left[2].y())
                dir = -1;

              // It should be only possible to have none or one solution.
              double ti[2];
              double ix;

              // At2 + Bt + C t(At + B) + C
              if (Math::quadRoots(ti, poly, Math::kAfter0, Math::kBefore1) >= 1)
                ix = ti[0] * (ax * ti[0] + bx) + cx;
              else if (py - minY < maxY - py)
                ix = p[0].x();
              else
                ix = p[2].x();

              if (px >= ix)
                windingNumber += dir;
            }

            tCut = tVal;
          }
        }
        break;
      }

      case Path2D::kCmdCubicTo: {
        B2D_ASSERT(hasMoveTo);
        B2D_ASSERT(i >= 3);

        const Point* p = vtxData - 1;
        if (B2D_UNLIKELY(!hasMoveTo))
          goto _Invalid;

        double minY = Math::min(p[0].y(), Math::min(p[1].y(), Math::min(p[2].y(), p[3].y())));
        double maxY = Math::max(p[0].y(), Math::max(p[1].y(), Math::max(p[2].y(), p[3].y())));

        cmdData += 3;
        vtxData += 3;
        i -= 3;

        if (py >= minY && py <= maxY) {
          bool degenerated = Math::isNear(p[0].y(), p[1].y()) &&
                             Math::isNear(p[1].y(), p[2].y()) &&
                             Math::isNear(p[2].y(), p[3].y()) ;

          if (degenerated) {
            x0 = p[0].x();
            y0 = p[0].y();
            x1 = p[3].x();
            y1 = p[3].y();
            goto _DoLine;
          }

          // Subdivide curve to curve-spline separated at Y-extrama.
          Point* left = (Point*)ptBuffer;
          Point* rght = (Point*)ptBuffer + 4;

          double poly[4];
          poly[0] = 3.0 * (-p[0].y() + 3.0 * (p[1].y() - p[2].y()) + p[3].y());
          poly[1] = 6.0 * ( p[0].y() - 2.0 * (p[1].y() + p[2].y())           );
          poly[2] = 3.0 * (-p[0].y() +       (p[1].y()           )           );

          double tExtrema[3];
          double tCut = 0.0;

          size_t tIndex;
          size_t tLength;

          tLength = Math::quadRoots(tExtrema, poly, Math::kAfter0, Math::kBefore1);
          tExtrema[tLength++] = 1.0;

          rght[0] = p[0];
          rght[1] = p[1];
          rght[2] = p[2];
          rght[3] = p[3];

          for (tIndex = 0; tIndex < tLength; tIndex++) {
            double tVal = tExtrema[tIndex];

            if (tVal == tCut)
              continue;

            if (tVal == 1.0) {
              left[0] = rght[0];
              left[1] = rght[1];
              left[2] = rght[2];
              left[3] = rght[3];
            }
            else {
              Geom2D::splitCubic(rght, rght, left, tCut == 0.0 ? tVal : (tVal - tCut) / (1.0 - tCut));
            }

            minY = Math::min(left[0].y(), left[3].y());
            maxY = Math::max(left[0].y(), left[3].y());

            if (py >= minY && py < maxY) {
              double ax, ay, bx, by;
              double cx, cy, dx, dy;
              Geom2D::extractCubicPolynomial(left, ax, ay, bx, by, cx, cy, dx, dy);

              poly[0] = ay;
              poly[1] = by;
              poly[2] = cy;
              poly[3] = dy - py;

              int dir = 0;
              if (left[0].y() < left[3].y())
                dir = 1;
              else if (left[0].y() > left[3].y())
                dir = -1;

              // It should be only possible to have zero/one solution.
              double ti[3];
              double ix;

              // Always use the Horner's method to evaluate a polynomial.
              //   At^3 + Bt^2 + Ct + D
              //   (At^2 + Bt + C)t + D
              //   ((At + B)t + C)t + D
              if (Math::cubicRoots(ti, poly, Math::kAfter0, Math::kBefore1) >= 1)
                ix = ((ax * ti[0] + bx) * ti[0] + cx) * ti[0] + dx;
              else if (py - minY < maxY - py)
                ix = p[0].x();
              else
                ix = p[3].x();

              if (px >= ix)
                windingNumber += dir;
            }

            tCut = tVal;
          }
        }
        break;
      }

      // ----------------------------------------------------------------------
      // [Close-To]
      // ----------------------------------------------------------------------

      case Path2D::kCmdClose: {
        if (hasMoveTo) {
          x0 = vtxData[-1].x();
          y0 = vtxData[-1].y();
          x1 = start.x();
          y1 = start.y();

          hasMoveTo = false;
          goto _DoLine;
        }

        cmdData++;
        vtxData++;

        i--;
        break;
      }

      default:
        B2D_NOT_REACHED();
    }
  } while (i);

  // Close the path.
  if (hasMoveTo) {
    x0 = vtxData[-1].x();
    y0 = vtxData[-1].y();
    x1 = start.x();
    y1 = start.y();

    hasMoveTo = false;
    goto _DoLine;
  }

  if (fillRule == FillRule::kEvenOdd)
    windingNumber &= 1;
  return windingNumber != 0;

_Invalid:
  return false;
}

bool Geom2D::_hitTest(const Point& p, uint32_t id, const void* data, uint32_t fillRule) noexcept {
  switch (id) {
    case kGeomArgBox     : return bHitTestBox<Box     >(p, *static_cast<const Box    *>(data));
    case kGeomArgIntBox  : return bHitTestBox<IntBox  >(p, *static_cast<const IntBox *>(data));
    case kGeomArgRect    : return bHitTestRect<Rect   >(p, *static_cast<const Rect   *>(data));
    case kGeomArgIntRect : return bHitTestRect<IntRect>(p, *static_cast<const IntRect*>(data));
    case kGeomArgCircle  : return bHitTestCircle       (p, *static_cast<const Circle *>(data));
    case kGeomArgEllipse : return bHitTestEllipse      (p, *static_cast<const Ellipse*>(data));
    case kGeomArgRound   : return bHitTestRound        (p, *static_cast<const Round  *>(data));
    case kGeomArgChord   : return bHitTestChord        (p, *static_cast<const Chord  *>(data));
    case kGeomArgPie     : return bHitTestPie          (p, *static_cast<const Pie    *>(data));
    case kGeomArgTriangle: return bHitTestTriangle     (p,  static_cast<const Point  *>(data));
    case kGeomArgPath2D  : return bHitTestPath         (p, *static_cast<const Path2D *>(data), fillRule);
    default              : return false;
  }
}

B2D_END_NAMESPACE
