// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math.h"
#include "../core/pathflatten.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Flattener - Helpers]
// ============================================================================

enum {
  kFlattenerRecursionLimit = 32,
  kFlattenerPreallocSize = 128
};

static B2D_INLINE void _bPathCopyCmd(uint8_t* dst, const uint8_t* src, size_t size) {
  std::memcpy(dst, src, size);
}

static B2D_INLINE void _bPathCopyPts(Point* dst, const Point* src, size_t size) {
  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
}

// ============================================================================
// [b2d::Flattener - Flatten]
// ============================================================================

#define B2D_FLATTENER_RESERVE(N) \
  B2D_MACRO_BEGIN \
    size_t _curSize = (size_t)(dstCmdData - dstI->commandData()); \
    size_t _growTo = bDataGrow(sizeof(Path2DImpl) - 16, _curSize, _curSize + N, sizeof(Point) + 1); \
    \
    dstI->_size = _curSize; \
    e = dst.reserve(_growTo); \
    \
    dstI = dst.impl(); \
    if (B2D_UNLIKELY(e != kErrorOk)) \
      goto _Fail; \
    \
    dstCmdData = dstI->commandData() + _curSize; \
    dstCmdEnd  = dstI->commandData() + dstI->capacity(); \
    dstVtxData = dstI->vertexData() + _curSize; \
  B2D_MACRO_END

#define B2D_FLATTENER_ADD(X, Y, CMD) \
  B2D_MACRO_BEGIN \
    B2D_ASSERT(dstCmdData < dstCmdEnd); \
    dstVtxData[0].reset(X, Y); \
    dstCmdData[0] = CMD; \
    \
    dstVtxData++; \
    dstCmdData++; \
  B2D_MACRO_END

static Error bFlattenerFlatten(const Flattener* self, Path2D& dst,
  const uint8_t* srcCmd, const Point* srcPts, size_t srcSize) {

  Path2DImpl* dstI = dst.impl();
  Error e = kErrorOk;

  size_t i = 0;
  uint32_t c = 0;

  double flatnessSquare = self->_flatnessSq;
  Wrap<Point> _stack[kFlattenerRecursionLimit * 4];

  // Minimal count of vertices added to the dst path is 'size'. We assume
  // that there are curves in the source path so size is multiplied by 4
  // and small constant is added to ensure that we will not reallocate if the
  // given path is small, but still contains curves. Please note that this
  // can't overflow, because a single path command has 17 bytes (point+byte)
  // and we are multiplying only by 6.
  size_t dstInitial = dstI->size();
  size_t predict = dstInitial + (srcSize * 6u) + kFlattenerPreallocSize;

  B2D_PROPAGATE(dst.reserve(predict));
  dstI = dst.impl();

  Point* dstVtxData = dstI->vertexData();
  uint8_t* dstCmdData = dstI->commandData();
  uint8_t* dstCmdEnd = dstCmdData + dstI->capacity();

  dstCmdData += dstInitial;
  dstVtxData += dstInitial;

  for (;;) {
    // Collect 'move-to' and 'line-to' commands.
    while (i < srcSize) {
      c = srcCmd[i];
      if (!Path2D::isMoveOrLineCmd(c))
        break;
      i++;
    }

    // Invalid state if 'i' is zero (no 'move-to' or 'line-to').
    if (i == 0)
      goto _InvalidState;

    // Include 'close' command if used.
    if (c == Path2D::kCmdClose)
      i++;

    // Copy non-bezier cmd/pts to the destination path.
    if ((size_t)(dstCmdEnd - dstCmdData) < i) {
      B2D_FLATTENER_RESERVE(i);
    }

    _bPathCopyCmd(dstCmdData, srcCmd, i); dstCmdData += i; srcCmd += i;
    _bPathCopyPts(dstVtxData, srcPts, i); dstVtxData += i; srcPts += i;

    // Advance.
    B2D_ASSERT(i <= srcSize);
    if ((srcSize -= i) == 0)
      break;
    i = 0;

    if (c == Path2D::kCmdClose)
      continue;

    // Flatten quadratic or cubic bezier curve(s).
    do {
      c = srcCmd[i];
      if (!Path2D::isQuadOrCubicCmd(c))
        break;

      Point* stack = &_stack[0];
      uint32_t level = 0;

      if (c == Path2D::kCmdQuadTo) {
        if ((i += 2) > srcSize)
          goto _InvalidState;

        double x0 = srcPts[i - 3].x();
        double y0 = srcPts[i - 3].y();
        double x1 = srcPts[i - 2].x();
        double y1 = srcPts[i - 2].y();
        double x2 = srcPts[i - 1].x();
        double y2 = srcPts[i - 1].y();

        if ((size_t)(dstCmdEnd - dstCmdData) < kFlattenerPreallocSize) {
_QBezier_Realloc:
          B2D_FLATTENER_RESERVE(kFlattenerPreallocSize);
        }

        for (;;) {
          if (B2D_UNLIKELY((size_t)(dstCmdEnd - dstCmdData) < 2))
            goto _QBezier_Realloc;

          // Calculate all the mid-points of the line segments.
          double x01  = (x0 + x1) * 0.5;
          double y01  = (y0 + y1) * 0.5;
          double x12  = (x1 + x2) * 0.5;
          double y12  = (y1 + y2) * 0.5;
          double x012 = (x01 + x12) * 0.5;
          double y012 = (y01 + y12) * 0.5;

          double dx = x2 - x0;
          double dy = y2 - y0;
          double d = std::abs((x1 - x2) * dy - (y1 - y2) * dx);
          double da;

          if (d > Math::kCollinearityEpsilon) {
            // Regular case.
            if (d * d <= flatnessSquare * (dx * dx + dy * dy)) {
              B2D_FLATTENER_ADD(x012, y012, Path2D::kCmdLineTo);
              goto _QBezier_Pop;
            }
          }
          else {
            // Collinear case.
            da = dx * dx + dy * dy;
            if (da == 0) {
              d = Math::lengthSq(x0, y0, x1, y1);
            }
            else {
              d = ((x1 - x0) * dx + (y1 - y0) * dy) / da;

              // Simple collinear case, 0---1---2.
              if (d > 0 && d < 1)
                goto _QBezier_Pop;

              if (d <= 0)
                d = Math::lengthSq(x1, y1, x0, y0);
              else if (d >= 1)
                d = Math::lengthSq(x1, y1, x2, y2);
              else
                d = Math::lengthSq(x1, y1, x0 + d * dx, y0 + d * dy);
            }

            if (d < flatnessSquare) {
              B2D_FLATTENER_ADD(x1, y1, Path2D::kCmdLineTo);
              goto _QBezier_Pop;
            }
          }

          // Continue subdivision.
          if (level < kFlattenerRecursionLimit) {
            stack[0].reset(x012, y012);
            stack[1].reset(x12, y12);
            stack[2].reset(x2, y2);

            stack += 3;
            level++;

            x1 = x01;
            y1 = y01;
            x2 = x012;
            y2 = y012;
            continue;
          }

          if (!std::isfinite(x012))
            goto _InvalidState;

_QBezier_Pop:
          if (level == 0)
            break;

          stack -= 3;
          level--;

          x0 = stack[0].x();
          y0 = stack[0].y();
          x1 = stack[1].x();
          y1 = stack[1].y();
          x2 = stack[2].x();
          y2 = stack[2].y();
        }

        // End point.
        B2D_FLATTENER_ADD(x2, y2, Path2D::kCmdLineTo);
      }
      else {
        B2D_ASSERT(c == Path2D::kCmdCubicTo);
        if ((i += 3) > srcSize)
          goto _InvalidState;

        double x0 = srcPts[i - 4].x();
        double y0 = srcPts[i - 4].y();
        double x1 = srcPts[i - 3].x();
        double y1 = srcPts[i - 3].y();
        double x2 = srcPts[i - 2].x();
        double y2 = srcPts[i - 2].y();
        double x3 = srcPts[i - 1].x();
        double y3 = srcPts[i - 1].y();

        if ((size_t)(dstCmdEnd - dstCmdData) < kFlattenerPreallocSize) {
_CBezier_Realloc:
          B2D_FLATTENER_RESERVE(kFlattenerPreallocSize);
        }

        for (;;) {
          if (B2D_UNLIKELY((size_t)(dstCmdEnd - dstCmdData) < 2))
            goto _CBezier_Realloc;

          // Calculate all the mid-points of the line segments.
          double x01   = (x0 + x1) * 0.5;
          double y01   = (y0 + y1) * 0.5;
          double x12   = (x1 + x2) * 0.5;
          double y12   = (y1 + y2) * 0.5;
          double x23   = (x2 + x3) * 0.5;
          double y23   = (y2 + y3) * 0.5;
          double x012  = (x01 + x12) * 0.5;
          double y012  = (y01 + y12) * 0.5;
          double x123  = (x12 + x23) * 0.5;
          double y123  = (y12 + y23) * 0.5;
          double x0123 = (x012 + x123) * 0.5;
          double y0123 = (y012 + y123) * 0.5;

          // Try to approximate the full cubic curve by a single straight line.
          double dx = x3 - x0;
          double dy = y3 - y0;

          double d2 = std::abs(((x1 - x3) * dy - (y1 - y3) * dx));
          double d3 = std::abs(((x2 - x3) * dy - (y2 - y3) * dx));
          double da1, da2, k;

          switch ((unsigned(d2 > Math::kCollinearityEpsilon) << 1) +
                   unsigned(d3 > Math::kCollinearityEpsilon)     ) {
            // All collinear FGR p0 == p3.
            case 0:
              k = dx * dx + dy * dy;

              if (k == 0) {
                d2 = Math::lengthSq(x0, y0, x1, y1);
                d3 = Math::lengthSq(x3, y3, x2, y2);
              }
              else {
                k   = 1.0 / k;
                da1 = x1 - x0;
                da2 = y1 - y0;
                d2  = k * (da1 * dx + da2 * dy);
                da1 = x2 - x0;
                da2 = y2 - y0;
                d3  = k * (da1 * dx + da2 * dy);

                // Simple collinear case, 0---1---2---3.
                if (d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1)
                  goto _CBezier_Pop;

                if (d2 <= 0)
                  d2 = Math::lengthSq(x1, y1, x0, y0);
                else if (d2 >= 1)
                  d2 = Math::lengthSq(x1, y1, x3, y3);
                else
                  d2 = Math::lengthSq(x1, y1, x0 + d2*dx, y0 + d2*dy);

                if (d3 <= 0)
                  d3 = Math::lengthSq(x2, y2, x0, y0);
                else if (d3 >= 1)
                  d3 = Math::lengthSq(x2, y2, x3, y3);
                else
                  d3 = Math::lengthSq(x2, y2, x0 + d3*dx, y0 + d3*dy);
              }

              if (d2 > d3) {
                if (d2 < flatnessSquare) {
                  B2D_FLATTENER_ADD(x1, y1, Path2D::kCmdLineTo);
                  goto _CBezier_Pop;
                }
              }
              else {
                if (d3 < flatnessSquare) {
                  B2D_FLATTENER_ADD(x2, y2, Path2D::kCmdLineTo);
                  goto _CBezier_Pop;
                }
              }
              break;

            // p0, p1, p3 are collinear, p2 is significant.
            case 1:
              if (d3 * d3 <= flatnessSquare * (dx * dx + dy * dy)) {
                B2D_FLATTENER_ADD(x12, y12, Path2D::kCmdLineTo);
                goto _CBezier_Pop;
              }
              break;

            // p0, p2, p3 are collinear, p1 is significant.
            case 2:
              if (d2 * d2 <= flatnessSquare * (dx * dx + dy * dy)) {
                B2D_FLATTENER_ADD(x12, y12, Path2D::kCmdLineTo);
                goto _CBezier_Pop;
              }
              break;

            // Regular case.
            case 3:
              if ((d2 + d3) * (d2 + d3) <= flatnessSquare * (dx * dx + dy * dy)) {
                B2D_FLATTENER_ADD(x12, y12, Path2D::kCmdLineTo);
                goto _CBezier_Pop;
              }
              break;
          }

          // Continue subdivision.
          if (level < kFlattenerRecursionLimit) {
            stack[0].reset(x0123, y0123);
            stack[1].reset(x123, y123);
            stack[2].reset(x23, y23);
            stack[3].reset(x3, y3);

            stack += 4;
            level++;

            x1 = x01;
            y1 = y01;
            x2 = x012;
            y2 = y012;
            x3 = x0123;
            y3 = y0123;
            continue;
          }

          if (!std::isfinite(x0123))
            goto _InvalidState;

_CBezier_Pop:
          if (level == 0)
            break;

          stack -= 4;
          level--;

          x0 = stack[0].x();
          y0 = stack[0].y();
          x1 = stack[1].x();
          y1 = stack[1].y();
          x2 = stack[2].x();
          y2 = stack[2].y();
          x3 = stack[3].x();
          y3 = stack[3].y();
        }

        // End point.
        B2D_FLATTENER_ADD(x3, y3, Path2D::kCmdLineTo);
      }
    } while (i < srcSize);

    // Advance.
    B2D_ASSERT(i <= srcSize);
    if ((srcSize -= i) == 0)
      break;

    srcCmd += i;
    srcPts += i;

    i = size_t(c == Path2D::kCmdClose);
    c = Path2D::kCmdMoveTo;
  }

  dstI->_size = (size_t)(dstCmdData - dstI->commandData());
  return kErrorOk;

_InvalidState:
  e = DebugUtils::errored(kErrorInvalidGeometry);
  goto _Fail;

_Fail:
  dstI->_size = dstInitial;
  return e;
}

Error Flattener::flattenPath(Path2D& dst, const Path2D& src) const {
  Path2DImpl* srcI = src.impl();
  if (srcI->size() == 0)
    return kErrorOk;

  if (&dst == &src) {
    Path2D pathTmp;
    B2D_PROPAGATE(bFlattenerFlatten(this, pathTmp, srcI->commandData(), srcI->vertexData(), srcI->size()));
    return dst.addPath(pathTmp);
  }
  else {
    return bFlattenerFlatten(this, dst, srcI->commandData(), srcI->vertexData(), srcI->size());
  }
}

B2D_END_NAMESPACE
