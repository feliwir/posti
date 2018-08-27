// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/geom2d.h"
#include "../core/matrix2d.h"
#include "../core/membuffer.h"
#include "../core/path2d.h"
#include "../core/pathflatten.h"
#include "../core/stroke.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::StrokeParams - Construction / Destruction]
// ============================================================================

StrokeParams::StrokeParams() noexcept
  : _strokeWidth(kStrokeWidthDefault),
    _miterLimit(kMiterLimitDefault),
    _dashOffset(kDashOffsetDefault),
    _dashArray(),
    _hints {{ StrokeCap::kDefault, StrokeCap::kDefault, StrokeJoin::kDefault, 0 }} {}

StrokeParams::StrokeParams(const StrokeParams& other) noexcept
  : _strokeWidth(other._strokeWidth),
    _miterLimit(other._miterLimit),
    _dashOffset(other._dashOffset),
    _dashArray(other._dashArray),
    _hints(other._hints) {}

StrokeParams::~StrokeParams() noexcept {}

// ============================================================================
// [b2d::StrokeParams - Operator Overload]
// ============================================================================

StrokeParams& StrokeParams::operator=(const StrokeParams& other) noexcept {
  _strokeWidth = other._strokeWidth;
  _miterLimit = other._miterLimit;
  _dashOffset = other._dashOffset;
  _dashArray = other._dashArray;
  _hints.packed = other._hints.packed;

  return *this;
}

// ============================================================================
// [b2d::StrokerPrivate]
// ============================================================================

class StrokerPrivate {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE StrokerPrivate(const Stroker& stroker, Path2D& dst) noexcept
    : stroker(stroker),
      dst(dst),
      distances(nullptr),
      distancesAlloc(0) {
    dstInitial = dst.size();
  }

  B2D_INLINE ~StrokerPrivate() noexcept {}

  // --------------------------------------------------------------------------
  // [Stroke]
  // --------------------------------------------------------------------------

  Error strokeArg(uint32_t argId, const void* data) noexcept;
  Error strokePath(const Path2D& src) noexcept;
  Error strokePathPrivate(const Path2D& src) noexcept;

  // --------------------------------------------------------------------------
  // [Prepare / Finalize]
  // --------------------------------------------------------------------------

  Error _begin() noexcept;
  Error _grow() noexcept;

  Error calcArc(
    double x,   double y,
    double dx1, double dy1,
    double dx2, double dy2) noexcept;

  Error calcMiter(
    const Point& v0,
    const Point& v1,
    const Point& v2,
    double dx1, double dy1,
    double dx2, double dy2,
    int lineJoin,
    double mlimit,
    double dbevel) noexcept;

  Error calcCap(
    const Point& v0,
    const Point& v1,
    double len,
    uint32_t cap) noexcept;

  Error calcJoin(
    const Point& v0,
    const Point& v1,
    const Point& v2,
    double len1,
    double len2) noexcept;

  Error strokePathFigure(const Point* src, size_t count, bool outline) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const Stroker& stroker;

  Path2D& dst;
  size_t dstInitial;

  Point* dstCur;
  Point* dstEnd;
  // Point* pts;
  // uint8_t *cmd;
  // size_t remain;

  //! Memory buffer used to store distances.
  MemBufferTmp<1024> buffer;

  double* distances;
  size_t distancesAlloc;
};

// ============================================================================
// [b2d::StrokerPrivate - Stroke]
// ============================================================================

Error StrokerPrivate::strokeArg(uint32_t argId, const void* data) noexcept {
  if (argId == kGeomArgPath2D) {
    return strokePath(*static_cast<const Path2D*>(data));
  }
  else {
    Path2DTmp<32> pathTmp;
    pathTmp._addArg(argId, data, nullptr, Path2D::kDirCW);
    return strokePath(pathTmp);
  }
}

Error StrokerPrivate::strokePath(const Path2D& src) noexcept {
  // Source and destination paths has to be different instances and source path
  // has to be flag (no curves).
  if (&dst == &src || src.hasCurves()) {
    Path2DTmp<256> pathTmp;
    Flattener flattener { stroker._flatness };

    B2D_PROPAGATE(flattener.flattenPath(pathTmp, src));
    return strokePathPrivate(pathTmp);
  }
  else {
    return strokePathPrivate(src);
  }
}

Error StrokerPrivate::strokePathPrivate(const Path2D& src) noexcept {
  Path2DImpl* srcI = src.impl();

  const uint8_t* srcCmdData = srcI->commandData();
  const Point* srcVtxData = srcI->vertexData();

  // Traverse path, find moveTo / lineTo segments and stroke them.
  const uint8_t* subCmdData = nullptr;
  const uint8_t* curCmdData = srcCmdData;

  size_t remain = srcI->size();
  while (remain) {
    uint8_t cmd = curCmdData[0];

    // LineTo is the most used command, so it's first.
    if (cmd == Path2D::kCmdLineTo) {
      // Nothing here...
    }
    else if (cmd == Path2D::kCmdMoveTo) {
      // Send path to the stroker if there is anything to process.
      if (B2D_LIKELY(subCmdData && subCmdData != curCmdData)) {
        size_t subStart = (size_t)(subCmdData - srcCmdData);
        size_t subSize = (size_t)(curCmdData - subCmdData);

        B2D_PROPAGATE(
          strokePathFigure(srcVtxData + subStart, subSize, false)
        );
      }

      // Advance to new subpath.
      subCmdData = curCmdData;
    }
    else if (cmd == Path2D::kCmdClose) {
      // Send path to the stroker if there is anything to process.
      if (B2D_LIKELY(subCmdData)) {
        size_t subStart = (size_t)(subCmdData - srcCmdData);
        size_t subSize = (size_t)(curCmdData - subCmdData);

        B2D_PROPAGATE(
          strokePathFigure(srcVtxData + subStart, subSize, subSize > 2)
        );
      }

      // We clear the beginning mark, because we expect PATH_MOVE_TO command from now.
      subCmdData = nullptr;
    }

    curCmdData++;
    remain--;
  }

  // Send path to stroker if there is something (this is the last, unclosed, figure)
  if (subCmdData != nullptr && subCmdData != curCmdData) {
    size_t subStart = (size_t)(subCmdData - srcCmdData);
    size_t subSize = (size_t)(curCmdData - subCmdData);

    B2D_PROPAGATE(
      strokePathFigure(srcVtxData + subStart, subSize, false)
    );
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::StrokerPrivate - Calc]
// ============================================================================

#define B2D_STROKER_EMIT(X, Y)          \
  B2D_MACRO_BEGIN                       \
    if (B2D_UNLIKELY(dstCur == dstEnd)) \
      B2D_PROPAGATE(_grow());           \
                                        \
    dstCur->reset(X, Y);                \
    dstCur++;                           \
  B2D_MACRO_END

#define B2D_STROKER_INDEX()( (size_t)(dstCur - dst.impl()->vertexData()) )

Error StrokerPrivate::_begin() noexcept {
  Path2DImpl* dstI = dst.impl();

  size_t needed = dstI->capacity();
  size_t size = dstI->size();
  size_t remain = needed - size;

  if (remain < 64 || dstI->isShared()) {
    if (needed < 256)
      needed = 256;
    else
      needed *= 2;

    B2D_PROPAGATE(dst.reserve(needed));
    dstI = dst.impl();
  }

  dstCur = dstI->vertexData();
  dstEnd = dstCur;

  dstCur += size;
  dstEnd += dstI->capacity();

  return kErrorOk;
}

Error StrokerPrivate::_grow() noexcept {
  Path2DImpl* dstI = dst.impl();
  size_t capacity = dstI->capacity();

  dstI->_size = capacity;
  if (capacity < 256)
    capacity = 512;
  else
    capacity *= 2;

  B2D_PROPAGATE(dst.reserve(capacity));
  dstI = dst.impl();

  dstCur = dstI->vertexData();
  dstEnd = dstCur;

  dstCur += dstI->size();
  dstEnd += dstI->capacity();

  return kErrorOk;
};

Error StrokerPrivate::calcArc(
  double x,   double y,
  double dx1, double dy1,
  double dx2, double dy2) noexcept {

  int i, n;

  double a1, a2;
  double da = stroker._da;

  B2D_STROKER_EMIT(x + dx1, y + dy1);
  if (stroker._wSign > 0) {
    a1 = std::atan2(dy1, dx1);
    a2 = std::atan2(dy2, dx2);
    if (a1 > a2) a2 += Math::k2Pi;
    n = int((a2 - a1) / da);
  }
  else {
    a1 = std::atan2(-dy1, -dx1);
    a2 = std::atan2(-dy2, -dx2);
    if (a1 < a2) a2 -= Math::k2Pi;
    n = int((a1 - a2) / da);
  }

  da = (a2 - a1) / (n + 1);
  a1 += da;

  for (i = 0; i < n; i++) {
    double a1Sin = std::sin(a1);
    double a1Cos = std::cos(a1);

    B2D_STROKER_EMIT(x + a1Cos * stroker._w, y + a1Sin * stroker._w);
    a1 += da;
  }

  B2D_STROKER_EMIT(x + dx2, y + dy2);
  return kErrorOk;
}

Error StrokerPrivate::calcMiter(
  const Point& v0,
  const Point& v1,
  const Point& v2,
  double dx1, double dy1,
  double dx2, double dy2,
  int lineJoin,
  double mlimit,
  double dbevel) noexcept {

  Point pi(v1._x, v1._y);
  double di = 1.0;
  double lim = stroker._wAbs * mlimit;
  bool intersectionFailed  = true; // Assume the worst

  if (Geom2D::intersectLines(
    Point(v0._x + dx1, v0._y - dy1),
    Point(v1._x + dx1, v1._y - dy1),
    Point(v1._x + dx2, v1._y - dy2),
    Point(v2._x + dx2, v2._y - dy2), pi)) {

    // Calculation of the intersection succeeded.
    di = Math::length(v1, pi);
    if (di <= lim) {
      // Inside the miter limit.
      B2D_STROKER_EMIT(pi._x, pi._y);
      return kErrorOk;
    }
    intersectionFailed = false;
  }
  else {
    // Calculation of the intersection failed, most probably the three points
    // lie one straight line. First check if v0 and v2 lie on the opposite
    // sides of vector: (v1._x, v1._y) -> (v1._x + dx1, v1._y - dy1), that is,
    // the perpendicular to the line determined by vertices v0 and v1.
    //
    // This condition determines whether the next line segments continues
    // the previous one or goes back.
    Point vt(v1._x + dx1, v1._y - dy1);

    if ((Geom2D::crossProduct(v0, v1, vt) < 0.0) == (Geom2D::crossProduct(v1, v2, vt) < 0.0)) {
      // This case means that the next segment continues the previous one
      // (straight line).
      B2D_STROKER_EMIT(v1._x + dx1, v1._y - dy1);
      return kErrorOk;
    }
  }

  // Miter limit exceeded.
  switch (lineJoin) {
    case StrokeJoin::kMiterRev:
      // For the compatibility with SVG, PDF, etc, we use a simple bevel
      // join instead of "smart" bevel.
      B2D_STROKER_EMIT(v1._x + dx1, v1._y - dy1);
      B2D_STROKER_EMIT(v1._x + dx2, v1._y - dy2);
      break;

    case StrokeJoin::kMiterRound:
      B2D_PROPAGATE( calcArc(v1._x, v1._y, dx1, -dy1, dx2, -dy2) );
      break;

    default:
      // If no miter-revert, calculate new dx1, dy1, dx2, dy2.
      if (intersectionFailed) {
        mlimit *= stroker._wSign;
        B2D_STROKER_EMIT(v1._x + dx1 + dy1 * mlimit, v1._y - dy1 + dx1 * mlimit);
        B2D_STROKER_EMIT(v1._x + dx2 - dy2 * mlimit, v1._y - dy2 - dx2 * mlimit);
      }
      else {
        double x1 = v1._x + dx1;
        double y1 = v1._y - dy1;
        double x2 = v1._x + dx2;
        double y2 = v1._y - dy2;
        di = (lim - dbevel) / (di - dbevel);
        B2D_STROKER_EMIT(x1 + (pi._x - x1) * di, y1 + (pi._y - y1) * di);
        B2D_STROKER_EMIT(x2 + (pi._x - x2) * di, y2 + (pi._y - y2) * di);
      }
      break;
  }

  return kErrorOk;
}

Error StrokerPrivate::calcCap(const Point& v0, const Point& v1, double len, uint32_t cap) noexcept {
  double ilen = 1.0 / len;

  double dx1 = (v1._y - v0._y) * ilen;
  double dy1 = (v1._x - v0._x) * ilen;

  dx1 *= stroker._w;
  dy1 *= stroker._w;

  switch (cap) {
    case StrokeCap::kButt: {
      B2D_STROKER_EMIT(v0._x - dx1, v0._y + dy1);
      B2D_STROKER_EMIT(v0._x + dx1, v0._y - dy1);
      break;
    }

    case StrokeCap::kSquare: {
      double dx2 = dy1 * stroker._wSign;
      double dy2 = dx1 * stroker._wSign;

      B2D_STROKER_EMIT(v0._x - dx1 - dx2, v0._y + dy1 - dy2);
      B2D_STROKER_EMIT(v0._x + dx1 - dx2, v0._y - dy1 - dy2);
      break;
    }

    case StrokeCap::kRound: {
      int i;
      int n = int(Math::kPi / stroker._da);

      double da = Math::kPi / double(n + 1);
      double a1;

      B2D_STROKER_EMIT(v0._x - dx1, v0._y + dy1);

      if (stroker._wSign > 0) {
        a1 = std::atan2(dy1, -dx1) + da;
      }
      else {
        da = -da;
        a1 = std::atan2(-dy1, dx1) + da;
      }

      for (i = 0; i < n; i++) {
        double a1_sin = std::sin(a1);
        double a1_cos = std::cos(a1);

        B2D_STROKER_EMIT(v0._x + a1_cos * stroker._w, v0._y + a1_sin * stroker._w);
        a1 += da;
      }

      B2D_STROKER_EMIT(v0._x + dx1, v0._y - dy1);
      break;
    }

    case StrokeCap::kRoundRev: {
      int i;
      int n = int(Math::kPi / stroker._da);

      double da = Math::kPi / double(n + 1);
      double a1;

      double dx2 = dy1 * stroker._wSign;
      double dy2 = dx1 * stroker._wSign;

      double vx = v0._x - dx2;
      double vy = v0._y - dy2;

      B2D_STROKER_EMIT(vx - dx1, vy + dy1);

      if (stroker._wSign > 0) {
        da = -da;
        a1 = std::atan2(dy1, -dx1) + da;
      }
      else {
        a1 = std::atan2(-dy1, dx1) + da;
      }

      for (i = 0; i < n; i++) {
        double a1_sin = std::sin(a1);
        double a1_cos = std::cos(a1);

        B2D_STROKER_EMIT(vx + a1_cos * stroker._w, vy + a1_sin * stroker._w);
        a1 += da;
      }

      B2D_STROKER_EMIT(vx + dx1, vy - dy1);
      break;
    }

    case StrokeCap::kTriangle: {
      double dx2 = dy1 * stroker._wSign;
      double dy2 = dx1 * stroker._wSign;

      B2D_STROKER_EMIT(v0._x - dx1, v0._y + dy1);
      B2D_STROKER_EMIT(v0._x - dx2, v0._y - dy2);
      B2D_STROKER_EMIT(v0._x + dx1, v0._y - dy1);
      break;
    }

    case StrokeCap::kTriangleRev: {
      double dx2 = dy1 * stroker._wSign;
      double dy2 = dx1 * stroker._wSign;

      B2D_STROKER_EMIT(v0._x - dx1 - dx2, v0._y + dy1 - dy2);
      B2D_STROKER_EMIT(v0._x, v0._y);
      B2D_STROKER_EMIT(v0._x + dx1 - dx2, v0._y - dy1 - dy2);
    }
  }

  return kErrorOk;
}

// ============================================================================
// [INNER_JOIN]
// ============================================================================

Error StrokerPrivate::calcJoin(
  const Point& v0,
  const Point& v1,
  const Point& v2,
  double len1,
  double len2) noexcept
{
  double wilen1 = (stroker._w / len1);
  double wilen2 = (stroker._w / len2);

  double dx1 = (v1._y - v0._y) * wilen1;
  double dy1 = (v1._x - v0._x) * wilen1;
  double dx2 = (v2._y - v1._y) * wilen2;
  double dy2 = (v2._x - v1._x) * wilen2;

  double cp = Geom2D::crossProduct(v0, v1, v2);

  if (cp != 0 && (cp > 0) == (stroker._w > 0)) {
    B2D_STROKER_EMIT(v1._x + dx1, v1._y - dy1);
    B2D_STROKER_EMIT(v1._x + dx2, v1._y - dy2);
  }
  else {
    // Outer join

    // Calculate the distance between v1 and the central point of the bevel
    // line segment.
    double dx = (dx1 + dx2) / 2;
    double dy = (dy1 + dy2) / 2;
    double dbevel = std::sqrt(dx * dx + dy * dy);
    // TODO:
    /*
    if (stroker._params.strokeJoin() == StrokeJoin::kRound ||
        stroker._params.strokeJoin() == StrokeJoin::kBevel)
    {
      // This is an optimization that reduces the number of points
      // in cases of almost collinear segments. If there's no
      // visible difference between bevel and miter joins we'd rather
      // use miter join because it adds only one point instead of two.
      //
      // Here we calculate the middle point between the bevel points
      // and then, the distance between v1 and this middle point.
      // At outer joins this distance always less than stroke width,
      // because it's actually the height of an isosceles triangle of
      // v1 and its two bevel points. If the difference between this
      // width and this value is small (no visible bevel) we can
      // add just one point.
      //
      // The constant in the expression makes the result approximately
      // the same as in round joins and caps. You can safely comment
      // out this entire "if".
      // TODO: ApproxScale used
      if (stroker._flatness * (stroker._wAbs - dbevel) < stroker._wEps) {
        Point pi;
        if (Geom2D::intersectLines(
          Point(v0._x + dx1, v0._y - dy1),
          Point(v1._x + dx1, v1._y - dy1),
          Point(v1._x + dx2, v1._y - dy2),
          Point(v2._x + dx2, v2._y - dy2), pi)) {

          B2D_STROKER_EMIT(pi._x, pi._y);
        }
        else {
          B2D_STROKER_EMIT(v1._x + dx1, v1._y - dy1);
        }
        return kErrorOk;
      }
    }
    */

    switch (stroker._params.strokeJoin()) {
      case StrokeJoin::kMiter:
      case StrokeJoin::kMiterRev:
      case StrokeJoin::kMiterRound:
        B2D_PROPAGATE(
          calcMiter(v0, v1, v2, dx1, dy1, dx2, dy2,
            stroker._params.strokeJoin(),
            stroker._params.miterLimit(), dbevel)
        );
        break;

      case StrokeJoin::kRound:
        B2D_PROPAGATE(
          calcArc(v1._x, v1._y, dx1, -dy1, dx2, -dy2)
        );
        break;

      case StrokeJoin::kBevel:
        B2D_STROKER_EMIT(v1._x + dx1, v1._y - dy1);
        B2D_STROKER_EMIT(v1._x + dx2, v1._y - dy2);
        break;

      default:
        B2D_NOT_REACHED();
    }
  }

  return kErrorOk;
}

Error StrokerPrivate::strokePathFigure(const Point* src, size_t count, bool outline) noexcept {
  // Can't stroke one-vertex array.
  if (count <= 1)
    return kErrorOk;
    //return kErrorGeometryUnstrokeable;

  // To do outline we need at least three vertices.
  if (outline && count <= 2)
    return kErrorOk;
    // return kErrorGeometryUnstrokeable;

  const Point* cur;
  size_t i;
  size_t moveToPosition0 = dst.size();
  size_t moveToPosition1 = Globals::kInvalidIndex;

  B2D_PROPAGATE(_begin());

  // Alloc or realloc array for our distances (distance between individual
  // vertices [0]->[1], [1]->[2], ...). Distance at index[0] means distance
  // between src[0] and src[1], etc. Last distance is special and it is 0.0
  // if path is not closed, otherwise src[count-1]->src[0].
  if (distancesAlloc < count) {
    // Need to realloc, we align count to 128 vertices.
    if (distances) buffer.reset();

    distancesAlloc = (count + 127) & ~127;
    distances = reinterpret_cast<double*>(buffer.alloc(distancesAlloc * sizeof(double)));

    if (distances == nullptr) {
      distancesAlloc = 0;
      return DebugUtils::errored(kErrorNoMemory);
    }
  }

  double* dist;
  for (i = 0; i < count - 1; i++) {
    double d = Math::length(src[i], src[i + 1]);
    if (d <= Math::kDistanceEpsilon)
      d = 0.0;
    distances[i] = d;
  }

/*
  for (i = count - 1, cur = src, dist = distances; i; i--, cur++, dist++)
  {
    double d = Math::length(cur[0], cur[1]);
    if (d <= Math::kDistanceEpsilon)
      d = 0.0;
    dist[0] = d;
  }
*/
  const Point* srcEnd = src + count;

  Point cp[3]; // Current points.
  double cd[3]; // Current distances.

  // --------------------------------------------------------------------------
  // [Outline]
  // --------------------------------------------------------------------------

#define IS_DEGENERATED_DIST(_Dist_) \
  ((_Dist_) <= Math::kDistanceEpsilon)

  if (outline) {
    // We need also to calc distance between first and last point.
    {
      double d = Math::length(src[count - 1], src[0]);
      if (d <= Math::kDistanceEpsilon)
        d = 0.0;
      distances[count - 1] = d;
    }

    Point fp[2]; // First points.
    double fd[2]; // First distances.

    // ------------------------------------------------------------------------
    // [Outline 1]
    // ------------------------------------------------------------------------

    cur = src;
    dist = distances;

    // Fill the current points / distances and skip degenerate cases.
    i = 0;

    // FirstI is first point. When first and last points are equal, we need to
    // mark the second one as first.
    //
    // Examine two paths:
    //
    // - [100, 100] -> [300, 300] -> [100, 300] -> CLOSE
    //
    //   The firstI will be zero in that case. Path is self-closing, but the
    //   close end-point is not shared with the first vertex.
    //
    // - [100, 100] -> [300, 300] -> [100, 300] -> [100, 100] -> CLOSE
    //
    //   The firstI will be one. Path is self-closing, but unfortunatelly the
    //   closing point vertex is already there (fourth command).
    //
    size_t firstI = i;

    cp[i] = src[count - 1];
    cd[i] = dist[count - 1];
    if (B2D_LIKELY(!IS_DEGENERATED_DIST(cd[i]))) { i++; firstI++; }

    do {
      if (B2D_UNLIKELY(cur == srcEnd))
        return kErrorOk;
        //return kErrorGeometryUnstrokeable;

      cp[i] = *cur++;
      cd[i] = *dist++;
      if (B2D_LIKELY(!IS_DEGENERATED_DIST(cd[i]))) i++;
    } while (i < 3);

    // Save two first points and distances (we need them to finish the outline).
    fp[0] = cp[firstI];
    fd[0] = cd[firstI];

    fp[1] = cp[firstI + 1];
    fd[1] = cd[firstI + 1];

    // Make the outline.
    for (;;) {
      calcJoin(cp[0], cp[1], cp[2], cd[0], cd[1]);

      if (cur == srcEnd) goto _Outline1Done;
      while (B2D_UNLIKELY(IS_DEGENERATED_DIST(dist[0])))
      {
        dist++;
        if (++cur == srcEnd) goto _Outline1Done;
      }

      cp[0] = cp[1];
      cd[0] = cd[1];

      cp[1] = cp[2];
      cd[1] = cd[2];

      cp[2] = *cur++;
      cd[2] = *dist++;
    }

    // End joins.
_Outline1Done:
    B2D_PROPAGATE( calcJoin(cp[1], cp[2], fp[0], cd[1], cd[2]) );
    B2D_PROPAGATE( calcJoin(cp[2], fp[0], fp[1], cd[2], fd[0]) );

    // Close path (CW).
    B2D_STROKER_EMIT(std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::quiet_NaN());

    // ------------------------------------------------------------------------
    // [Outline 2]
    // ------------------------------------------------------------------------

    moveToPosition1 = B2D_STROKER_INDEX();

    cur = src + count;
    dist = distances + count;

    // Fill the current points / distances and skip degenerate cases.
    i = 0;
    firstI = 0;
    cp[i] = src[0];
    cd[i] = distances[0];
    if (cd[i] != 0.0) { i++; firstI++; }

    do {
      cp[i] = *--cur;
      cd[i] = *--dist;
      if (B2D_LIKELY(cd[i] != 0.0)) i++;
    } while (i < 3);

    // Save two first points and distances (we need them to finish the outline).
    fp[0] = cp[firstI];
    fd[0] = cd[firstI];

    fp[1] = cp[firstI + 1];
    fd[1] = cd[firstI + 1];

    // Make the outline.
    for (;;) {
      calcJoin(cp[0], cp[1], cp[2], cd[1], cd[2]);

      do {
        if (cur == src) goto _Outline2Done;
        cur--;
        dist--;
      } while (B2D_UNLIKELY(dist[0] == 0.0));

      cp[0] = cp[1];
      cd[0] = cd[1];

      cp[1] = cp[2];
      cd[1] = cd[2];

      cp[2] = *cur;
      cd[2] = *dist;
    }

    // End joins.
_Outline2Done:
    B2D_PROPAGATE( calcJoin(cp[1], cp[2], fp[0], cd[2], fd[0]) );
    B2D_PROPAGATE( calcJoin(cp[2], fp[0], fp[1], fd[0], fd[1]) );

    // Close path (CCW).
    B2D_STROKER_EMIT(std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::quiet_NaN());
  }

  // --------------------------------------------------------------------------
  // [Pen]
  // --------------------------------------------------------------------------

  else {
    distances[count - 1] = distances[count - 2];

    // ------------------------------------------------------------------------
    // [Outline 1]
    // ------------------------------------------------------------------------

    cur = src;
    dist = distances;

    // Fill the current points / distances and skip degenerate cases.
    i = 0;

    do {
      if (B2D_UNLIKELY(cur == srcEnd))
        return kErrorOk;
        //return kErrorGeometryUnstrokeable;

      cp[i] = *cur++;
      cd[i] = *dist++;
      if (B2D_LIKELY(cd[i] != 0.0)) i++;
    } while (i < 2);

    // Start cap.
    B2D_PROPAGATE( calcCap(cp[0], cp[1], cd[0], stroker._params.startCap()) );

    // Make the outline.
    if (cur == srcEnd) goto _Pen1Done;
    while (B2D_UNLIKELY(dist[0] == 0.0)) {
      dist++;
      if (++cur == srcEnd) goto _Pen1Done;
    }

    goto _Pen1Loop;

    for (;;) {
      cp[0] = cp[1];
      cd[0] = cd[1];

      cp[1] = cp[2];
      cd[1] = cd[2];

_Pen1Loop:
      cp[2] = *cur++;
      cd[2] = *dist++;

      B2D_PROPAGATE( calcJoin(cp[0], cp[1], cp[2], cd[0], cd[1]) );
      if (cur == srcEnd) goto _Pen1Done;

      while (B2D_UNLIKELY(dist[0] == 0.0)) {
        dist++;
        if (++cur == srcEnd) goto _Pen1Done;
      }
    }

    // End joins.
_Pen1Done:

    // ------------------------------------------------------------------------
    // [Outline 2]
    // ------------------------------------------------------------------------

    // Fill the current points / distances and skip degenerate cases.
    i = 0;

    do {
      cp[i] = *--cur;
      cd[i] = *--dist;
      if (B2D_LIKELY(cd[i] != 0.0)) i++;
    } while (i < 2);

    // End cap.
    B2D_PROPAGATE( calcCap(cp[0], cp[1], cd[1], stroker._params.endCap()) );

    // Make the outline.
    if (cur == src) goto _Pen2Done;

    cur--;
    dist--;

    while (B2D_UNLIKELY(dist[0] == 0.0)) {
      if (cur == src)
        goto _Pen2Done;
      dist--;
      cur--;
    }

    goto _Pen2Loop;

    for (;;) {
      do {
        if (cur == src)
          goto _Pen2Done;
        cur--;
        dist--;
      } while (B2D_UNLIKELY(dist[0] == 0.0));

      cp[0] = cp[1];
      cd[0] = cd[1];

      cp[1] = cp[2];
      cd[1] = cd[2];

_Pen2Loop:
      cp[2] = *cur;
      cd[2] = *dist;

      B2D_PROPAGATE( calcJoin(cp[0], cp[1], cp[2], cd[1], cd[2]) );
    }
_Pen2Done:
    // Close path (CCW).
    B2D_STROKER_EMIT(std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::quiet_NaN());
  }

  {
    // Fix the size of the path.
    Path2DImpl* dstI = dst.impl();
    size_t finalSize = B2D_STROKER_INDEX();

    B2D_ASSERT(finalSize <= dstI->capacity());
    dstI->_size = finalSize;

    // Fix path adding Path2D::kCmdMoveTo/Path2D::kCmdClose commands at the beginning
    // of each outline and filling rest by Path2D::kCmdLineTo. This allowed us to
    // simplify B2D_STROKER_EMIT() macro.
    uint8_t* dstCmd = dstI->commandData();

    // Close clockwise path.
    if (moveToPosition0 < finalSize) {
      std::memset(dstCmd + moveToPosition0, Path2D::kCmdLineTo, finalSize - moveToPosition0);
      dstCmd[moveToPosition0] = Path2D::kCmdMoveTo;
      dstCmd[finalSize - 1] = Path2D::kCmdClose;
    }

    // Close counter-clockwise path.
    if (moveToPosition1 < finalSize) {
      dstCmd[moveToPosition1 - 1] = Path2D::kCmdClose;
      dstCmd[moveToPosition1    ] = Path2D::kCmdMoveTo;
    }
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::Stroker - Construction / Destruction]
// ============================================================================

Stroker::Stroker() noexcept
  : _params(),
    _w(0),
    _wAbs(0),
    _wEps(0),
    _da(0),
    _flatness(Math::kFlatness),
    _wSign(1),
    _dirty(true) {}

Stroker::Stroker(const Stroker& other) noexcept
  : _params(other._params),
    _w(other._w),
    _wAbs(other._wAbs),
    _wEps(other._wEps),
    _da(other._da),
    _flatness(other._flatness),
    _wSign(other._wSign),
    _dirty(other._dirty) {}

Stroker::Stroker(const StrokeParams& params) noexcept
  : _params(params),
    _w(0),
    _wAbs(0),
    _wEps(0),
    _da(0),
    _flatness(Math::kFlatness),
    _wSign(1),
    _dirty(true) {}

Stroker::~Stroker() noexcept {}

// ============================================================================
// [b2d::Stroker - Parameters]
// ============================================================================

Error Stroker::setParams(const StrokeParams& params) noexcept {
  _params = params;

  _dirty = true;
  return kErrorOk;
}

Error Stroker::setFlatness(double flatness) noexcept {
  _flatness = flatness;

  _dirty = true;
  return kErrorOk;
}

// ============================================================================
// [b2d::Stroker - Update]
// ============================================================================

void Stroker::_update() noexcept {
  _w = _params._strokeWidth * 0.5;
  _wAbs = _w;
  _wSign = 1;

  if (_w < 0.0) {
    _wAbs  = -_w;
    _wSign = -1;
  }

  _wEps = _w / 1024.0;
  _da = std::acos(_wAbs / (_wAbs + 0.125 * _flatness)) * 2.0;

  _dirty = false;
}

// ============================================================================
// [b2d::Stroker - Stroke]
// ============================================================================

Error Stroker::_strokeArg(Path2D& dst, uint32_t argId, const void* data) const noexcept {
  update();

  StrokerPrivate engine(*this, dst);
  return engine.strokeArg(argId, data);
}

// ============================================================================
// [b2d::Stroker - Operator Overload]
// ============================================================================

Stroker& Stroker::operator=(const Stroker& other) noexcept {
  _params = other._params;

  _w = other._w;
  _wAbs = other._wAbs;
  _wEps = other._wEps;
  _da = other._da;
  _flatness = other._flatness;
  _wSign = other._wSign;
  _dirty = other._dirty;

  return *this;
}

B2D_END_NAMESPACE
