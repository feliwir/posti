// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/simd.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Matrix2D - MapPoints]
// ============================================================================

static void B2D_CDECL Matrix2D_mapPointsIdentity_SSE2(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  B2D_UNUSED(self);
  if (dst == src)
    return;

  size_t i = size;
  if (Support::isAligned(((uintptr_t)dst | (uintptr_t)src), 16)) {
    while (i >= 4) {
      D128 s0 = vloadd128a(src + 0);
      D128 s1 = vloadd128a(src + 1);
      D128 s2 = vloadd128a(src + 2);
      D128 s3 = vloadd128a(src + 3);

      vstored128a(dst + 0, s0);
      vstored128a(dst + 1, s1);
      vstored128a(dst + 2, s2);
      vstored128a(dst + 3, s3);

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      vstored128a(dst, vloadd128a(src));
      i--;
      dst++;
      src++;
    }
  }
  else {
    while (i) {
      D128 sx = vloadd128_64(&src->_x);
      D128 sy = vloadd128_64(&src->_y);

      vstored64(&dst->_x, sx);
      vstored64(&dst->_y, sy);

      i--;
      dst++;
      src++;
    }
  }
}

static void B2D_CDECL Matrix2D_mapPointsTranslation_SSE2(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D128 m_20_21 = vloadd128u(self->m + Matrix2D::k20);

  size_t i = size;
  if (Support::isAligned(((uintptr_t)dst | (uintptr_t)src), 16)) {
    while (i >= 4) {
      D128 s0 = vloadd128a(src + 0);
      D128 s1 = vloadd128a(src + 1);
      D128 s2 = vloadd128a(src + 2);
      D128 s3 = vloadd128a(src + 3);

      vstored128a(dst + 0, vaddpd(s0, m_20_21));
      vstored128a(dst + 1, vaddpd(s1, m_20_21));
      vstored128a(dst + 2, vaddpd(s2, m_20_21));
      vstored128a(dst + 3, vaddpd(s3, m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128a(src);
      vstored128a(dst, vaddpd(s0, m_20_21));

      i--;
      dst++;
      src++;
    }
  }
  else {
    while (i >= 4) {
      D128 s0 = vloadd128u(src + 0);
      D128 s1 = vloadd128u(src + 1);
      D128 s2 = vloadd128u(src + 2);
      D128 s3 = vloadd128u(src + 3);

      vstored128u(dst + 0, vaddpd(s0, m_20_21));
      vstored128u(dst + 1, vaddpd(s1, m_20_21));
      vstored128u(dst + 2, vaddpd(s2, m_20_21));
      vstored128u(dst + 3, vaddpd(s3, m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128u(src + 0);
      vstored128u(dst + 0, vaddpd(s0, m_20_21));

      i--;
      dst++;
      src++;
    }
  }
}

static void B2D_CDECL Matrix2D_mapPointsScaling_SSE2(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D128 m_00_11 = vsetd128(self->m11(), self->m00());
  D128 m_20_21 = vloadd128u(self->m + Matrix2D::k20);

  size_t i = size;
  if (Support::isAligned(((uintptr_t)dst | (uintptr_t)src), 16)) {
    while (i >= 4) {
      D128 s0 = vloadd128a(src + 0);
      D128 s1 = vloadd128a(src + 1);
      D128 s2 = vloadd128a(src + 2);
      D128 s3 = vloadd128a(src + 3);

      vstored128a(dst + 0, vaddpd(vmulpd(s0, m_00_11), m_20_21));
      vstored128a(dst + 1, vaddpd(vmulpd(s1, m_00_11), m_20_21));
      vstored128a(dst + 2, vaddpd(vmulpd(s2, m_00_11), m_20_21));
      vstored128a(dst + 3, vaddpd(vmulpd(s3, m_00_11), m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128a(src);
      vstored128a(dst, vaddpd(vmulpd(s0, m_00_11), m_20_21));

      i--;
      dst++;
      src++;
    }
  }
  else {
    while (i >= 4) {
      D128 s0 = vloadd128u(src + 0);
      D128 s1 = vloadd128u(src + 1);
      D128 s2 = vloadd128u(src + 2);
      D128 s3 = vloadd128u(src + 3);

      vstored128u(dst + 0, vaddpd(vmulpd(s0, m_00_11), m_20_21));
      vstored128u(dst + 1, vaddpd(vmulpd(s1, m_00_11), m_20_21));
      vstored128u(dst + 2, vaddpd(vmulpd(s2, m_00_11), m_20_21));
      vstored128u(dst + 3, vaddpd(vmulpd(s3, m_00_11), m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128u(src);
      vstored128u(dst, vaddpd(vmulpd(s0, m_00_11), m_20_21));

      i--;
      dst++;
      src++;
    }
  }
}

static void B2D_CDECL Matrix2D_mapPointsSwap_SSE2(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D128 m_01_10 = vsetd128(self->m01(), self->m10());
  D128 m_20_21 = vloadd128u(self->m + Matrix2D::k20);

  size_t i = size;
  if (Support::isAligned(((uintptr_t)dst | (uintptr_t)src), 16)) {
    while (i >= 4) {
      D128 s0 = vloadd128a(src + 0);
      D128 s1 = vloadd128a(src + 1);
      D128 s2 = vloadd128a(src + 2);
      D128 s3 = vloadd128a(src + 3);

      s0 = vmulpd(vswapd64(s0), m_01_10);
      s1 = vmulpd(vswapd64(s1), m_01_10);
      s2 = vmulpd(vswapd64(s2), m_01_10);
      s3 = vmulpd(vswapd64(s3), m_01_10);

      vstored128a(dst + 0, vaddpd(s0, m_20_21));
      vstored128a(dst + 1, vaddpd(s1, m_20_21));
      vstored128a(dst + 2, vaddpd(s2, m_20_21));
      vstored128a(dst + 3, vaddpd(s3, m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128a(src);
      s0 = vmulpd(vswapd64(s0), m_01_10);
      vstored128a(dst, vaddpd(s0, m_20_21));

      i--;
      dst++;
      src++;
    }
  }
  else {
    while (i >= 4) {
      D128 s0 = vloadd128u(src + 0);
      D128 s1 = vloadd128u(src + 1);
      D128 s2 = vloadd128u(src + 2);
      D128 s3 = vloadd128u(src + 3);

      s0 = vmulpd(vswapd64(s0), m_01_10);
      s1 = vmulpd(vswapd64(s1), m_01_10);
      s2 = vmulpd(vswapd64(s2), m_01_10);
      s3 = vmulpd(vswapd64(s3), m_01_10);

      vstored128u(dst + 0, vaddpd(s0, m_20_21));
      vstored128u(dst + 1, vaddpd(s1, m_20_21));
      vstored128u(dst + 2, vaddpd(s2, m_20_21));
      vstored128u(dst + 3, vaddpd(s3, m_20_21));

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128u(src);
      s0 = vmulpd(vswapd64(s0), m_01_10);
      vstored128u(dst, vaddpd(s0, m_20_21));

      i--;
      dst++;
      src++;
    }
  }
}

static void B2D_CDECL Matrix2D_mapPointsAffine_SSE2(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D128 m_00_11 = vsetd128(self->m11(), self->m00());
  D128 m_10_01 = vsetd128(self->m01(), self->m10());
  D128 m_20_21 = vloadd128u(self->m + Matrix2D::k20);

  size_t i = size;
  if (Support::isAligned(((uintptr_t)dst | (uintptr_t)src), 16)) {
    while (i >= 4) {
      D128 s0 = vloadd128a(src + 0);
      D128 s1 = vloadd128a(src + 1);
      D128 s2 = vloadd128a(src + 2);
      D128 s3 = vloadd128a(src + 3);

      D128 r0 = vswapd64(s0);
      D128 r1 = vswapd64(s1);
      D128 r2 = vswapd64(s2);
      D128 r3 = vswapd64(s3);

      s0 = vmulpd(s0, m_00_11);
      s1 = vmulpd(s1, m_00_11);
      s2 = vmulpd(s2, m_00_11);
      s3 = vmulpd(s3, m_00_11);

      s0 = vaddpd(vaddpd(s0, m_20_21), vmulpd(r0, m_10_01));
      s1 = vaddpd(vaddpd(s1, m_20_21), vmulpd(r1, m_10_01));
      s2 = vaddpd(vaddpd(s2, m_20_21), vmulpd(r2, m_10_01));
      s3 = vaddpd(vaddpd(s3, m_20_21), vmulpd(r3, m_10_01));

      vstored128a(dst + 0, s0);
      vstored128a(dst + 1, s1);
      vstored128a(dst + 2, s2);
      vstored128a(dst + 3, s3);

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128a(src);
      D128 r0 = vswapd64(s0);

      s0 = vmulpd(s0, m_00_11);
      s0 = vaddpd(vaddpd(s0, m_20_21), vmulpd(r0, m_10_01));

      vstored128a(dst, s0);

      i--;
      dst++;
      src++;
    }
  }
  else {
    while (i >= 4) {
      D128 s0 = vloadd128u(src + 0);
      D128 s1 = vloadd128u(src + 1);
      D128 s2 = vloadd128u(src + 2);
      D128 s3 = vloadd128u(src + 3);

      D128 r0 = vswapd64(s0);
      D128 r1 = vswapd64(s1);
      D128 r2 = vswapd64(s2);
      D128 r3 = vswapd64(s3);

      s0 = vmulpd(s0, m_00_11);
      s1 = vmulpd(s1, m_00_11);
      s2 = vmulpd(s2, m_00_11);
      s3 = vmulpd(s3, m_00_11);

      s0 = vaddpd(vaddpd(s0, m_20_21), vmulpd(r0, m_10_01));
      s1 = vaddpd(vaddpd(s1, m_20_21), vmulpd(r1, m_10_01));
      s2 = vaddpd(vaddpd(s2, m_20_21), vmulpd(r2, m_10_01));
      s3 = vaddpd(vaddpd(s3, m_20_21), vmulpd(r3, m_10_01));

      vstored128u(dst + 0, s0);
      vstored128u(dst + 1, s1);
      vstored128u(dst + 2, s2);
      vstored128u(dst + 3, s3);

      i -= 4;
      dst += 4;
      src += 4;
    }

    while (i) {
      D128 s0 = vloadd128u(src);
      D128 r0 = vswapd64(s0);

      s0 = vmulpd(s0, m_00_11);
      s0 = vaddpd(vaddpd(s0, m_20_21), vmulpd(r0, m_10_01));

      vstored128u(dst, s0);

      i--;
      dst++;
      src++;
    }
  }
}

// ============================================================================
// [b2d::Matrix2D - Runtime Handlers (SSE2)]
// ============================================================================

void Matrix2DOnInitSSE2(Matrix2DOps& ops) noexcept {
  ops.mapPoints[Matrix2D::kTypeIdentity   ] = Matrix2D_mapPointsIdentity_SSE2;
  ops.mapPoints[Matrix2D::kTypeTranslation] = Matrix2D_mapPointsTranslation_SSE2;
  ops.mapPoints[Matrix2D::kTypeScaling    ] = Matrix2D_mapPointsScaling_SSE2;
  ops.mapPoints[Matrix2D::kTypeSwap       ] = Matrix2D_mapPointsSwap_SSE2;
  ops.mapPoints[Matrix2D::kTypeAffine     ] = Matrix2D_mapPointsAffine_SSE2;
  ops.mapPoints[Matrix2D::kTypeInvalid    ] = Matrix2D_mapPointsAffine_SSE2;
}

B2D_END_NAMESPACE
