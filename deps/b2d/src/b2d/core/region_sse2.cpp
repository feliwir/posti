// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/region.h"
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Region - Translate (SSE2)]
// ============================================================================

static Error B2D_CDECL Region_translate_SSE2(Region& dst, const Region& src, const IntPoint& pt) noexcept {
  using namespace SIMD;

  RegionImpl* dstI = dst.impl();
  RegionImpl* srcI = src.impl();

  int tx = pt._x;
  int ty = pt._y;

  if (srcI->isInfinite() || (tx | ty) == 0)
    return dst.setRegion(src);

  size_t size = srcI->size();
  if (size == 0)
    return dst.reset();

  // If the translation causes arithmetic overflow or underflow then we first
  // clip the input region into a safe boundary which might be then translated.
  constexpr int kMin = std::numeric_limits<int>::min();
  constexpr int kMax = std::numeric_limits<int>::max();

  bool saturate = ( (tx < 0) & (srcI->_boundingBox.x0() < int(unsigned(kMin) - unsigned(tx))) ) |
                  ( (ty < 0) & (srcI->_boundingBox.y0() < int(unsigned(kMin) - unsigned(ty))) ) |
                  ( (tx > 0) & (srcI->_boundingBox.x1() > int(unsigned(kMax) - unsigned(tx))) ) |
                  ( (ty > 0) & (srcI->_boundingBox.y1() > int(unsigned(kMax) - unsigned(ty))) ) ;

  if (saturate)
    return Region::translateClip(dst, src, pt, IntBox(kMin, kMin, kMax, kMax));

  if (dstI->isShared() || size > dstI->capacity()) {
    dstI = RegionImpl::new_(size);
    if (B2D_UNLIKELY(!dstI))
      return DebugUtils::errored(kErrorNoMemory);
  }

  IntBox* dstData = dstI->_data;
  IntBox* srcData = srcI->_data;

  I128 s0 = vloadi128u(&srcI->_boundingBox);
  I128 tr = vdupli64(vloadi128_64(&pt));

  s0 = vaddi32(s0, tr);
  vstorei128u(&dstI->_boundingBox, s0);

  dstI->_size = size;
  size_t i = size;

  if (Support::isAligned((uintptr_t)dstData | (uintptr_t)srcData, 16)) {
    while (i >= 4) {
      I128 s1, s2, s3;

      s0 = vloadi128a(srcData + 0);
      s1 = vloadi128a(srcData + 1);
      s2 = vloadi128a(srcData + 2);
      s3 = vloadi128a(srcData + 3);

      vstorei128a(dstData + 0, vaddi32(s0, tr));
      vstorei128a(dstData + 1, vaddi32(s1, tr));
      vstorei128a(dstData + 2, vaddi32(s2, tr));
      vstorei128a(dstData + 3, vaddi32(s3, tr));

      dstData += 4;
      srcData += 4;
      i -= 4;
    }

    while (i) {
      s0 = vloadi128a(srcData);
      vstorei128a(dstData, vaddi32(s0, tr));

      dstData++;
      srcData++;
      i--;
    }
  }
  else {
    while (i >= 4) {
      I128 s1, s2, s3;

      s0 = vloadi128u(srcData + 0);
      s1 = vloadi128u(srcData + 1);
      s2 = vloadi128u(srcData + 2);
      s3 = vloadi128u(srcData + 3);

      vstorei128u(dstData + 0, vaddi32(s0, tr));
      vstorei128u(dstData + 1, vaddi32(s1, tr));
      vstorei128u(dstData + 2, vaddi32(s2, tr));
      vstorei128u(dstData + 3, vaddi32(s3, tr));

      dstData += 4;
      srcData += 4;
      i -= 4;
    }

    while (i) {
      s0 = vloadi128u(srcData);
      vstorei128u(dstData, vaddi32(s0, tr));

      dstData++;
      srcData++;
      i--;
    }
  }

  return dstI != dst.impl() ? AnyInternal::replaceImpl(&dst, dstI) : kErrorOk;
}

// ============================================================================
// [b2d::Region - Eq (SSE2)]
// ============================================================================

static bool B2D_CDECL Region_eq_SSE2(const Region* a, const Region* b) noexcept {
  using namespace SIMD;

  const RegionImpl* aImpl = a->impl();
  const RegionImpl* bImpl = b->impl();

  if (aImpl == bImpl)
    return true;

  size_t size = aImpl->size();
  if (size != bImpl->_size)
    return false;

  const IntBox* aData = aImpl->_data;
  const IntBox* bData = bImpl->_data;

  I128 am = vloadi128u(&aImpl->_boundingBox);
  I128 b0 = vloadi128u(&bImpl->_boundingBox);

  am = vcmpeqi32(am, b0);
  if (!vhasmaski8(am, 0xFFFF))
    return false;

  size_t i = size;

  if (Support::isAligned((uintptr_t)aData | (uintptr_t)bData, 16)) {
    while (size >= 4) {
      I128 a1, a2, a3;
      I128 b1, b2, b3;

      am = vloadi128a(aData + 0);
      a1 = vloadi128a(aData + 1);

      b0 = vloadi128a(bData + 0);
      b1 = vloadi128a(bData + 1);

      am = vcmpeqi32(am, b0);
      a1 = vcmpeqi32(a1, b1);

      a2 = vloadi128a(aData + 2);
      a3 = vloadi128a(aData + 3);

      am = vand(am, a1);

      b2 = vloadi128a(bData + 2);
      b3 = vloadi128a(bData + 3);

      a2 = vcmpeqi32(a2, b2);
      a3 = vcmpeqi32(a3, b3);

      am = vand(am, a2);
      aData += 4;

      am = vand(am, a3);
      bData += 4;

      if (!vhasmaski8(am, 0xFFFF))
        return false;

      i -= 4;
    }

    while (i) {
      I128 a0;

      a0 = vloadi128a(aData + 0);
      b0 = vloadi128a(bData + 0);

      aData++;
      a0 = vcmpeqi32(a0, b0);

      bData++;
      am = vand(am, a0);

      i--;
    }
  }
  else {
    while (i >= 4) {
      I128 a1, a2, a3;
      I128 b1, b2, b3;

      am = vloadi128u(aData + 0);
      a1 = vloadi128u(aData + 1);

      b0 = vloadi128u(bData + 0);
      b1 = vloadi128u(bData + 1);

      am = vcmpeqi32(am, b0);
      a1 = vcmpeqi32(a1, b1);

      a2 = vloadi128u(aData + 2);
      a3 = vloadi128u(aData + 3);

      am = vand(am, a1);

      b2 = vloadi128u(bData + 2);
      b3 = vloadi128u(bData + 3);

      a2 = vcmpeqi32(a2, b2);
      a3 = vcmpeqi32(a3, b3);

      am = vand(am, a2);
      aData += 4;

      am = vand(am, a3);
      bData += 4;

      if (!vhasmaski8(am, 0xFFFF))
        return false;

      i -= 4;
    }

    while (i) {
      I128 a0;

      a0 = vloadi128u(aData);
      b0 = vloadi128u(bData);

      aData++;
      a0 = vcmpeqi32(a0, b0);

      bData++;
      am = vand(am, a0);

      i--;
    }
  }

  return vhasmaski8(am, 0xFFFF);
}

// ============================================================================
// [b2d::Region - Init / Fini (SSE2)]
// ============================================================================

void RegionOnInitSSE2(RegionOps& ops) noexcept {
  ops.translate = Region_translate_SSE2;
  ops.eq = Region_eq_SSE2;
}

B2D_END_NAMESPACE
