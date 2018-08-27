// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/runtime.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Matrix2D - Ops]
// ============================================================================

Matrix2DOps _bMatrix2DOps;

// ============================================================================
// [b2d::Matrix2D - Type]
// ============================================================================

uint32_t Matrix2D::type() const noexcept {
  // Matrix is not affine/rotation if:
  //   [. 0]
  //   [0 .]
  //   [. .]
  if (!Math::isNearZero(m01()) || !Math::isNearZero(m10())) {
    uint32_t type = kTypeAffine;

    if (Math::isNearZero(m[k00]) && Math::isNearZero(m11())) {
      type = kTypeSwap;
    }
    else {
      double d = m00() * m11() - m01() * m10();
      double r = m00() * m01() + m10() * m11();

      type = kTypeAffine;
      if (Math::isNearZero(r)) {
        if (Math::isNearZero(m00()) && Math::isNearZero(m11()))
          type = kTypeSwap;
      }

      if (Math::isNearZero(d))
        type = kTypeInvalid;
    }
    return type;
  }

  // Matrix is not scaling if:
  //   [1 .]
  //   [. 1]
  //   [. .]
  if (!Math::isNearOne(m00()) || !Math::isNearOne(m11())) {
    double d = m00() * m11();
    uint32_t type = kTypeScaling;

    if (Math::isNearZero(d))
      type = kTypeInvalid;
    return type;
  }

  // Matrix is not translation if:
  //   [. .]
  //   [. .]
  //   [0 0]
  if (!Math::isNearZero(m20()) || !Math::isNearZero(m21())) {
    return kTypeTranslation;
  }

  // Matrix is identity:
  //   [1 0]
  //   [0 1]
  //   [0 0]
  return kTypeIdentity;
}

// ============================================================================
// [b2d::Matrix2D - Op]
// ============================================================================

Matrix2D& Matrix2D::_matrixOp(uint32_t op, const void* data_) noexcept {
  const double* data = static_cast<const double*>(data_);

  switch (op) {
    //      |1 0|
    // M' = |0 1| * M
    //      |X Y|
    case kOpTranslateP: {
      double x = data[0];
      double y = data[1];

      m[k20] += x * m[k00] + y * m[k10];
      m[k21] += x * m[k01] + y * m[k11];

      return *this;
    }

    //          |1 0|
    // M' = M * |0 1|
    //          |X Y|
    case kOpTranslateA: {
      double x = data[0];
      double y = data[1];

      m[k20] += x;
      m[k21] += y;

      return *this;
    }

    //      [X 0]
    // M' = [0 Y] * M
    //      [0 0]
    case kOpScaleP: {
      double x = data[0];
      double y = data[1];

      m[k00] *= x; m[k01] *= y;
      m[k10] *= x; m[k11] *= y;

      return *this;
    }

    //          [X 0]
    // M' = M * [0 Y]
    //          [0 0]
    case kOpScaleA: {
      double x = data[0];
      double y = data[1];

      m[k00] *= x; m[k01] *= y;
      m[k10] *= x; m[k11] *= y;
      m[k20] *= x; m[k21] *= y;

      return *this;
    }

    //      [  1    tan(y)]
    // M' = [tan(x)   1   ] * M
    //      [  0      0   ]
    case kOpSkewP: {
      double x = data[0];
      double y = data[1];

      double xTan = std::tan(x);
      double yTan = x == y ? xTan : std::tan(y);

      double t00 = yTan * m[k10];
      double t01 = yTan * m[k11];

      m[k10] += xTan * m[k00];
      m[k11] += xTan * m[k01];

      m[k00] += t00;
      m[k01] += t01;

      return *this;
    }

    //          [  1    tan(y)]
    // M' = M * [tan(x)   1   ]
    //          [  0      0   ]
    case kOpSkewA:{
      double x = data[0];
      double y = data[1];

      double xTan = std::tan(x);
      double yTan = x == y ? xTan : std::tan(y);

      double t00 = m[k01] * xTan;
      double t10 = m[k11] * xTan;
      double t20 = m[k21] * xTan;

      m[k01] += m[k00] * yTan;
      m[k11] += m[k10] * yTan;
      m[k21] += m[k20] * yTan;

      m[k00] += t00;
      m[k10] += t10;
      m[k20] += t20;

      return *this;
    }

    // Tx and Ty are zero unless rotating about a point:
    //
    //   Tx = Px - cos(a) * Px + sin(a) * Py
    //   Ty = Py - sin(a) * Px - cos(a) * Py
    //
    //      [ cos(a) sin(a)]
    // M' = [-sin(a) cos(a)] * M
    //      [   Tx     Ty  ]
    case kOpRotateP:
    case kOpRotatePtP: {
      double angle = data[0];

      double aSin = std::sin(angle);
      double aCos = std::cos(angle);

      double t00 = aSin * m[k10] + aCos * m[k00];
      double t01 = aSin * m[k11] + aCos * m[k01];
      double t10 = aCos * m[k10] - aSin * m[k00];
      double t11 = aCos * m[k11] - aSin * m[k01];

      if (op == kOpRotatePtP) {
        double px = data[1];
        double py = data[2];

        double tx = px - aCos * px + aSin * py;
        double ty = py - aSin * px - aCos * py;

        double t20 = tx * m[k00] + ty * m[k10] + m[k20];
        double t21 = tx * m[k01] + ty * m[k11] + m[k21];

        m[k20] = t20;
        m[k21] = t21;
      }

      m[k00] = t00;
      m[k01] = t01;

      m[k10] = t10;
      m[k11] = t11;

      return *this;
    }

    //          [ cos(a) sin(a)]
    // M' = M * [-sin(a) cos(a)]
    //          [   x'     y'  ]
    case kOpRotateA:
    case kOpRotatePtA: {
      double angle = data[0];

      double aSin = std::sin(angle);
      double aCos = std::cos(angle);

      double t00 = m[k00] * aCos - m[k01] * aSin;
      double t01 = m[k00] * aSin + m[k01] * aCos;
      double t10 = m[k10] * aCos - m[k11] * aSin;
      double t11 = m[k10] * aSin + m[k11] * aCos;
      double t20 = m[k20] * aCos - m[k21] * aSin;
      double t21 = m[k20] * aSin + m[k21] * aCos;

      m[k00] = t00;
      m[k01] = t01;
      m[k10] = t10;
      m[k11] = t11;

      if (op == kOpRotatePtA) {
        double px = data[1];
        double py = data[2];

        t20 += px - aCos * px + aSin * py;
        t21 += py - aSin * px - aCos * py;
      }

      m[k20] = t20;
      m[k21] = t21;

      return *this;
    }

    // M = X * M
    case kOpMultiplyP: {
      const double* x = data;

      double t00 = x[k00] * m[k00] + x[k01] * m[k10];
      double t01 = x[k00] * m[k01] + x[k01] * m[k11];

      double t10 = x[k10] * m[k00] + x[k11] * m[k10];
      double t11 = x[k10] * m[k01] + x[k11] * m[k11];

      double t20 = x[k20] * m[k00] + x[k21] * m[k10] + m[k20];
      double t21 = x[k20] * m[k01] + x[k21] * m[k11] + m[k21];

      m[k00] = t00;
      m[k01] = t01;

      m[k10] = t10;
      m[k11] = t11;

      m[k20] = t20;
      m[k21] = t21;

      return *this;
    }

    // M = M * X
    case kOpMultiplyA: {
      const double* x = data;

      double t00 = m[k00] * x[k00] + m[k01] * x[k10];
      double t01 = m[k00] * x[k01] + m[k01] * x[k11];

      double t10 = m[k10] * x[k00] + m[k11] * x[k10];
      double t11 = m[k10] * x[k01] + m[k11] * x[k11];

      double t20 = m[k20] * x[k00] + m[k21] * x[k10] + x[k20];
      double t21 = m[k20] * x[k01] + m[k21] * x[k11] + x[k21];

      m[k00] = t00;
      m[k01] = t01;

      m[k10] = t10;
      m[k11] = t11;

      m[k20] = t20;
      m[k21] = t21;

      return *this;
    }

    default:
      return *this;
  }
}

// ============================================================================
// [b2d::Matrix2D - Invert]
// ============================================================================

bool Matrix2D::invert(Matrix2D& dst, const Matrix2D& src) noexcept {
  double d = src.m00() * src.m11() - src.m01() * src.m10();
  if (Math::isNearZero(d))
    return false;

  double t00 =  src.m11();
  double t01 = -src.m01();
  double t10 = -src.m10();
  double t11 =  src.m00();

  if (d != 1.0) {
    t00 /= d;
    t01 /= d;
    t10 /= d;
    t11 /= d;
  }

  double t20 = -(src.m20() * t00 + src.m21() * t10);
  double t21 = -(src.m20() * t01 + src.m21() * t11);
  dst.reset(t00, t01, t10, t11, t20, t21);

  return true;
}

// ============================================================================
// [b2d::Matrix2D - MapPoints]
// ============================================================================

static void B2D_CDECL Matrix2D_mapPointsIdentity(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  B2D_UNUSED(self);
  if (dst == src)
    return;

  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
}

static void B2D_CDECL Matrix2D_mapPointsTranslation(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  double m20 = self->m20();
  double m21 = self->m21();

  for (size_t i = 0; i < size; i++) {
    dst[i].reset(src[i]._x + m20, src[i]._y + m21);
  }
}

static void B2D_CDECL Matrix2D_mapPointsScaling(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  double m00 = self->m00();
  double m11 = self->m11();
  double m20 = self->m20();
  double m21 = self->m21();

  for (size_t i = 0; i < size; i++) {
    dst[i].reset(src[i]._x * m00 + m20, src[i]._y * m11 + m21);
  }
}

static void B2D_CDECL Matrix2D_mapPointsSwap(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  double m10 = self->m10();
  double m01 = self->m01();
  double m20 = self->m20();
  double m21 = self->m21();

  for (size_t i = 0; i < size; i++) {
    dst[i].reset(src[i]._y * m10 + m20, src[i]._x * m01 + m21);
  }
}

static void B2D_CDECL Matrix2D_mapPointsAffine(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  double m00 = self->m00();
  double m01 = self->m01();
  double m10 = self->m10();
  double m11 = self->m11();
  double m20 = self->m20();
  double m21 = self->m21();

  for (size_t i = 0; i < size; i++) {
    double x = src[i]._x;
    double y = src[i]._y;
    dst[i].reset(x * m00 + y * m10 + m20, x * m01 + y * m11 + m21);
  }
}

// ============================================================================
// [b2d::Matrix2D - MapPath / MapPathImpl]
// ============================================================================

Error Matrix2D::mapPath(Path2D& dst, const Path2D& src) const noexcept {
  uint32_t mType = type();
  size_t size = src.size();

  if (!size)
    return dst.reset();

  if (&dst == &src) {
    B2D_PROPAGATE(dst.detach());
    mapPointsByType(mType, dst.impl()->vertexData(), src.impl()->vertexData(), size);
    dst.makeDirty(Path2DInfo::kGeometryFlags);
  }
  else {
    size_t pos = dst._prepare(kContainerOpReplace, size, Path2DInfo::kGeometryFlags);
    if (B2D_UNLIKELY(pos == Globals::kInvalidIndex))
      return DebugUtils::errored(kErrorNoMemory);

    Path2DImpl* dstI = dst.impl();
    Path2DImpl* srcI = src.impl();

    std::memcpy(dstI->commandData() + pos, srcI->commandData(), size);
    mapPointsByType(mType, dstI->vertexData() + pos, srcI->vertexData(), size);
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::Matrix2D - Runtime Handlers]
// ============================================================================

#if B2D_MAYBE_SSE2
void Matrix2DOnInitSSE2(Matrix2DOps& ops) noexcept;
#endif

#if B2D_MAYBE_AVX
void Matrix2DOnInitAVX(Matrix2DOps& ops) noexcept;
#endif

void Matrix2DOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  Matrix2DOps& ops = _bMatrix2DOps;

  #if !B2D_ARCH_SSE2
  ops.mapPoints[Matrix2D::kTypeIdentity   ] = Matrix2D_mapPointsIdentity;
  ops.mapPoints[Matrix2D::kTypeTranslation] = Matrix2D_mapPointsTranslation;
  ops.mapPoints[Matrix2D::kTypeScaling    ] = Matrix2D_mapPointsScaling;
  ops.mapPoints[Matrix2D::kTypeSwap       ] = Matrix2D_mapPointsSwap;
  ops.mapPoints[Matrix2D::kTypeAffine     ] = Matrix2D_mapPointsAffine;
  ops.mapPoints[Matrix2D::kTypeInvalid    ] = Matrix2D_mapPointsAffine;
  #endif

  #if B2D_MAYBE_SSE2
  if (Runtime::hwInfo().hasSSE2()) Matrix2DOnInitSSE2(ops);
  #endif

  #if B2D_MAYBE_AVX
  if (Runtime::hwInfo().hasAVX()) Matrix2DOnInitAVX(ops);
  #endif
}

B2D_END_NAMESPACE
