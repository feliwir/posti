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
#include "../core/math.h"
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Matrix2D - MapPoints]
// ============================================================================

static void B2D_CDECL Matrix2D_mapPointsIdentity_AVX(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  B2D_UNUSED(self);
  if (dst == src)
    return;

  size_t i = size;
  while (i >= 8) {
    vstored256u(dst + 0, vloadd256u(src + 0));
    vstored256u(dst + 2, vloadd256u(src + 2));
    vstored256u(dst + 4, vloadd256u(src + 4));
    vstored256u(dst + 6, vloadd256u(src + 6));

    i -= 8;
    dst += 8;
    src += 8;
  }

  while (i >= 2) {
    vstored256u(dst, vloadd256u(src));

    i -= 2;
    dst += 2;
    src += 2;
  }

  if (i)
    vstored128u(dst, vloadd128u(src));
}

static void B2D_CDECL Matrix2D_mapPointsTranslation_AVX(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D256 m_20_21 = vbroadcastd256_128(self->m + Matrix2D::k20);

  size_t i = size;
  while (i >= 8) {
    vstored256u(dst + 0, vaddpd(vloadd256u(src + 0), m_20_21));
    vstored256u(dst + 2, vaddpd(vloadd256u(src + 2), m_20_21));
    vstored256u(dst + 4, vaddpd(vloadd256u(src + 4), m_20_21));
    vstored256u(dst + 6, vaddpd(vloadd256u(src + 6), m_20_21));

    i -= 8;
    dst += 8;
    src += 8;
  }

  while (i >= 2) {
    vstored256u(dst, vaddpd(vloadd256u(src), m_20_21));

    i -= 2;
    dst += 2;
    src += 2;
  }

  if (i)
    vstored128u(dst, vaddpd(vloadd128u(src), vcast<D128>(m_20_21)));
}

static void B2D_CDECL Matrix2D_mapPointsScaling_AVX(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D256 m_00_11 = vdupld128(vsetd128(self->m11(), self->m00()));
  D256 m_20_21 = vbroadcastd256_128(self->m + Matrix2D::k20);

  size_t i = size;
  while (i >= 8) {
    vstored256u(dst + 0, vaddpd(vmulpd(vloadd256u(src + 0), m_00_11), m_20_21));
    vstored256u(dst + 2, vaddpd(vmulpd(vloadd256u(src + 2), m_00_11), m_20_21));
    vstored256u(dst + 4, vaddpd(vmulpd(vloadd256u(src + 4), m_00_11), m_20_21));
    vstored256u(dst + 6, vaddpd(vmulpd(vloadd256u(src + 6), m_00_11), m_20_21));

    i -= 8;
    dst += 8;
    src += 8;
  }

  while (i >= 2) {
    vstored256u(dst, vaddpd(vmulpd(vloadd256u(src), m_00_11), m_20_21));

    i -= 2;
    dst += 2;
    src += 2;
  }

  if (i)
    vstored128u(dst, vaddpd(vmulpd(vloadd128u(src), vcast<D128>(m_00_11)), vcast<D128>(m_20_21)));
}

static void B2D_CDECL Matrix2D_mapPointsSwap_AVX(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D256 m_01_10 = vdupld128(vsetd128(self->m01(), self->m10()));
  D256 m_20_21 = vbroadcastd256_128(self->m + Matrix2D::k20);

  size_t i = size;
  while (i >= 8) {
    vstored256u(dst + 0, vaddpd(vmulpd(vswapd64(vloadd256u(src + 0)), m_01_10), m_20_21));
    vstored256u(dst + 2, vaddpd(vmulpd(vswapd64(vloadd256u(src + 2)), m_01_10), m_20_21));
    vstored256u(dst + 4, vaddpd(vmulpd(vswapd64(vloadd256u(src + 4)), m_01_10), m_20_21));
    vstored256u(dst + 6, vaddpd(vmulpd(vswapd64(vloadd256u(src + 6)), m_01_10), m_20_21));

    i -= 8;
    dst += 8;
    src += 8;
  }

  while (i >= 2) {
    vstored256u(dst, vaddpd(vmulpd(vswapd64(vloadd256u(src)), m_01_10), m_20_21));

    i -= 2;
    dst += 2;
    src += 2;
  }

  if (i)
    vstored128u(dst, vaddpd(vmulpd(vswapd64(vloadd128u(src)), vcast<D128>(m_01_10)), vcast<D128>(m_20_21)));
}

static void B2D_CDECL Matrix2D_mapPointsAffine_AVX(const Matrix2D* self, Point* dst, const Point* src, size_t size) noexcept {
  using namespace SIMD;

  D256 m_00_11 = vdupld128(vsetd128(self->m11(), self->m00()));
  D256 m_10_01 = vdupld128(vsetd128(self->m01(), self->m10()));
  D256 m_20_21 = vbroadcastd256_128(self->m + Matrix2D::k20);

  size_t i = size;
  while (i >= 8) {
    D256 s0 = vloadd256u(src + 0);
    D256 s1 = vloadd256u(src + 2);
    D256 s2 = vloadd256u(src + 4);
    D256 s3 = vloadd256u(src + 6);

    vstored256u(dst + 0, vaddpd(vaddpd(vmulpd(s0, m_00_11), m_20_21), vmulpd(vswapd64(s0), m_10_01)));
    vstored256u(dst + 2, vaddpd(vaddpd(vmulpd(s1, m_00_11), m_20_21), vmulpd(vswapd64(s1), m_10_01)));
    vstored256u(dst + 4, vaddpd(vaddpd(vmulpd(s2, m_00_11), m_20_21), vmulpd(vswapd64(s2), m_10_01)));
    vstored256u(dst + 6, vaddpd(vaddpd(vmulpd(s3, m_00_11), m_20_21), vmulpd(vswapd64(s3), m_10_01)));

    i -= 8;
    dst += 8;
    src += 8;
  }

  while (i >= 2) {
    D256 s0 = vloadd256u(src);
    vstored256u(dst, vaddpd(vaddpd(vmulpd(s0, m_00_11), m_20_21), vmulpd(vswapd64(s0), m_10_01)));

    i -= 2;
    dst += 2;
    src += 2;
  }

  if (i) {
    D128 s0 = vloadd128u(src);
    vstored128u(dst, vaddpd(vaddpd(vmulpd(s0, vcast<D128>(m_00_11)), vcast<D128>(m_20_21)), vmulpd(vswapd64(s0), vcast<D128>(m_10_01))));
  }
}

// ============================================================================
// [b2d::Matrix2D - Runtime Handlers (AVX)]
// ============================================================================

void Matrix2DOnInitAVX(Matrix2DOps& ops) noexcept {
  ops.mapPoints[Matrix2D::kTypeIdentity   ] = Matrix2D_mapPointsIdentity_AVX;
  ops.mapPoints[Matrix2D::kTypeTranslation] = Matrix2D_mapPointsTranslation_AVX;
  ops.mapPoints[Matrix2D::kTypeScaling    ] = Matrix2D_mapPointsScaling_AVX;
  ops.mapPoints[Matrix2D::kTypeSwap       ] = Matrix2D_mapPointsSwap_AVX;
  ops.mapPoints[Matrix2D::kTypeAffine     ] = Matrix2D_mapPointsAffine_AVX;
  ops.mapPoints[Matrix2D::kTypeInvalid    ] = Matrix2D_mapPointsAffine_AVX;
}

B2D_END_NAMESPACE
