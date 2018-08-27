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
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Line - Intersect]
// ============================================================================

uint32_t Line::intersect(Point& dst, const Line& a, const Line& b) noexcept {
  double ax = a.x1() - a.x0();
  double ay = a.y1() - a.y0();
  double bx = b.x1() - b.x0();
  double by = b.y1() - b.y0();

  double d = ay * bx - ax * by;
  if (Math::isNearZero(d) || !std::isfinite(d))
    return kLineIntersectionNone;

  double ox = a.x0() - b.x0();
  double oy = a.y0() - b.y0();

  double t = (by * ox - bx * oy) / d;
  dst.reset(a.x0() + ax * t, a.y0() + ay * t);

  if (t < 0.0 && t > 1.0)
    return kLineIntersectionUnbounded;
  t = (ax * oy - ay * ox) / d;

  if (t < 0.0 && t > 1.0)
    return kLineIntersectionUnbounded;
  else
    return kLineIntersectionBounded;
}

// ============================================================================
// [b2d::Circle - ToCSpline]
// ============================================================================

uint32_t Circle::toCSpline(Point* dst) const noexcept {
  double cx = _center._x;
  double cy = _center._y;

  double r = _radius;
  double rKappa = r * Math::kKappa;

  dst[ 0].reset(cx + r     , cy         );
  dst[ 1].reset(cx + r     , cy + rKappa);
  dst[ 2].reset(cx + rKappa, cy + r     );
  dst[ 3].reset(cx         , cy + r     );
  dst[ 4].reset(cx - rKappa, cy + r     );
  dst[ 5].reset(cx - r     , cy + rKappa);
  dst[ 6].reset(cx - r     , cy         );
  dst[ 7].reset(cx - r     , cy - rKappa);
  dst[ 8].reset(cx - rKappa, cy - r     );
  dst[ 9].reset(cx         , cy - r     );
  dst[10].reset(cx + rKappa, cy - r     );
  dst[11].reset(cx + r     , cy - rKappa);
  dst[12].reset(cx + r     , cy         );

  return 13;
}

// ============================================================================
// [b2d::Ellipse - ToCSpline]
// ============================================================================

uint32_t Ellipse::toCSpline(Point* dst) const noexcept {
  double cx = _center._x;
  double cy = _center._y;

  double rx = _radius._x;
  double ry = _radius._y;

  double rxKappa = rx * Math::kKappa;
  double ryKappa = ry * Math::kKappa;

  dst[ 0].reset(cx + rx     , cy          );
  dst[ 1].reset(cx + rx     , cy + ryKappa);
  dst[ 2].reset(cx + rxKappa, cy + ry     );
  dst[ 3].reset(cx          , cy + ry     );
  dst[ 4].reset(cx - rxKappa, cy + ry     );
  dst[ 5].reset(cx - rx     , cy + ryKappa);
  dst[ 6].reset(cx - rx     , cy          );
  dst[ 7].reset(cx - rx     , cy - ryKappa);
  dst[ 8].reset(cx - rxKappa, cy - ry     );
  dst[ 9].reset(cx          , cy - ry     );
  dst[10].reset(cx + rxKappa, cy - ry     );
  dst[11].reset(cx + rx     , cy - ryKappa);
  dst[12].reset(cx + rx     , cy          );

  return 13;
}

// ============================================================================
// [b2d::Arc - ToCSpline]
// ============================================================================

uint32_t Arc::toCSpline(Point* dst) const noexcept {
  double cx = _center._x;
  double cy = _center._y;

  double rx = _radius._x;
  double ry = _radius._y;

  double start = _start;
  double sweep = _sweep;

  if (start >= Math::k2Pi || start <= -Math::k2Pi)
    start = std::fmod(start, Math::k2Pi);

  double z       = 0.0;
  double rxOne   = 1.0;
  double ryOne   = 1.0;
  double rxKappa = Math::kKappa;
  double ryKappa = Math::kKappa;

  double as = 0.0;
  double ac = 1.0;

  if (start != 0.0) {
    as = std::sin(start);
    ac = std::cos(start);
  }

  double xx = ac * rx, yx =-as * rx;
  double xy = as * ry, yy = ac * ry;

  if (sweep < 0.0) {
    ryOne   = -ryOne;
    ryKappa = -ryKappa;
    sweep   = -sweep;
  }

  uint32_t nSegments;
  double lastSegment;

  if (sweep >= Math::k2Pi) {
    nSegments = 13;
    lastSegment = Math::kPiDiv2;
  }
  else if (sweep >= Math::k1_5Pi) {
    nSegments = 13;
    lastSegment = sweep - Math::k1_5Pi;
  }
  else if (sweep >= Math::kPi) {
    nSegments = 10;
    lastSegment = sweep - Math::kPi;
  }
  else if (sweep >= Math::kPiDiv2) {
    nSegments = 7;
    lastSegment = sweep - Math::kPiDiv2;
  }
  else {
    nSegments = 4;
    lastSegment = sweep;
  }

  if (Math::isNearZeroPositive(lastSegment) && nSegments > 4) {
    lastSegment = Math::kPiDiv2;
    nSegments -= 3;
  }

  dst[ 0].reset( rxOne  , z      );
  dst[ 1].reset( rxOne  , ryKappa);
  dst[ 2].reset( rxKappa, ryOne  );
  dst[ 3].reset( z      , ryOne  );

  if (nSegments == 4)
    goto _Skip;

  dst[ 4].reset(-rxKappa, ryOne  );
  dst[ 5].reset(-rxOne  , ryKappa);
  dst[ 6].reset(-rxOne  , z      );

  if (nSegments == 7)
    goto _Skip;

  dst[ 7].reset(-rxOne  ,-ryKappa);
  dst[ 8].reset(-rxKappa,-ryOne  );
  dst[ 9].reset( z      ,-ryOne  );

  if (nSegments == 10)
    goto _Skip;

  dst[10].reset( rxKappa,-ryOne  );
  dst[11].reset( rxOne  ,-ryKappa);
  dst[12].reset( rxOne  , z      );

_Skip:
  if (lastSegment < (Math::kPiDiv2 - Math::kEpsilon)) {
    double t = lastSegment / Math::kPiDiv2;
    Geom2D::leftCubic(&dst[nSegments - 4], &dst[nSegments - 4], t);
  }

  for (uint32_t i = 0; i < nSegments; i++) {
    double px = dst[i]._x;
    double py = dst[i]._y;

    dst[i].reset(cx + px * xx + py * yx,
                 cy + px * xy + py * yy);
  }

  return nSegments;
}

B2D_END_NAMESPACE
