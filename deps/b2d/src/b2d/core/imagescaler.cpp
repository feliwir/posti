// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/argb.h"
#include "../core/imagescaler.h"
#include "../core/math.h"
#include "../core/membuffer.h"
#include "../core/pixelformat.h"
#include "../core/runtime.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::ImageScaler - Ops]
// ============================================================================

struct ImageScalerOps {
  typedef Error (B2D_CDECL* WeightsFunc)(ImageScaler::Data* d, uint32_t dir, ImageScaler::CustomFunc func, const void* data);
  typedef void (B2D_CDECL* ProcessFunc)(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride);

  WeightsFunc weights;
  ProcessFunc horz[PixelFormat::kCount];
  ProcessFunc vert[PixelFormat::kCount];
};

static ImageScalerOps gImageScalerOps;

// ============================================================================
// [b2d::ImageScaler - BuiltInParams]
// ============================================================================

// Data needed by some functions (that take additional parameters).
struct ImageScalerBuiltInParams {
  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  B2D_INLINE void setupRadius(double r) noexcept {
    radius = r;
  }

  B2D_INLINE void setupMitchell(double b, double c) noexcept {
    mitchell.p0 =  1.0 - Math::k1Div3 * b;
    mitchell.p2 = -3.0 + 2.0          * b + c;
    mitchell.p3 =  2.0 - 1.5          * b - c;

    mitchell.q0 =  Math::k4Div3       * b + c * 4.0;
    mitchell.q1 = -2.0                * b - c * 8.0;
    mitchell.q2 =                       b + c * 5.0;
    mitchell.q3 = -Math::k1Div6       * b - c;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  struct Mitchell {
    double p0, p2, p3;
    double q0, q1, q2, q3;
  };

  double radius;
  Mitchell mitchell;
};

// ============================================================================
// [b2d::ImageScaler - Utils]
// ============================================================================

namespace ImageScalerUtils {
  // Calculates Bessel function of first kind of order `n`.
  //
  // Adapted for use in AGG library by Andy Wilk <castor.vulgaris@gmail.com>
  static B2D_INLINE double bessel(double x, int n) noexcept {
    double d = 1e-6;
    double b0 = 0.0;
    double b1 = 0.0;

    if (Math::abs(x) <= d)
      return n != 0 ? 0.0 : 1.0;

    // Set up a starting order for recurrence.
    int m1 = Math::abs(x) > 5.0 ? int(Math::abs(1.4 * x + 60.0 / x))
                                : int(Math::abs(x) + 6);
    int m2 = Math::max(int(Math::abs(x)) / 4 + 2 + n, m1);

    for (;;) {
      double c2 = Math::kEpsilon;
      double c3 = 0.0;
      double c4 = 0.0;

      int m8 = m2 & 1;
      for (int i = 1, iEnd = m2 - 1; i < iEnd; i++) {
        double c6 = 2 * (m2 - i) * c2 / x - c3;
        c3 = c2;
        c2 = c6;

        if (m2 - i - 1 == n)
          b1 = c6;

        m8 ^= 1;
        if (m8)
          c4 += c6 * 2.0;
      }

      double c6 = 2.0 * c2 / x - c3;
      if (n == 0)
        b1 = c6;

      c4 += c6;
      b1 /= c4;

      if (std::abs(b1 - b0) < d)
        return b1;

      b0 = b1;
      m2 += 3;
    }
  }

  static B2D_INLINE double sinXDivX(double x) noexcept {
    return std::sin(x) / x;
  }

  static B2D_INLINE double lanczos(double x, double y) noexcept {
    return sinXDivX(x) * sinXDivX(y);
  }

  static B2D_INLINE double blackman(double x, double y) noexcept {
    return sinXDivX(x) * (0.42 + 0.5 * std::cos(y) + 0.08 * std::cos(y * 2.0));
  }
}

// ============================================================================
// [b2d::ImageScaler - Funcs]
// ============================================================================

namespace ImageScalerFuncs {
  static Error B2D_CDECL nearest(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t <= 0.5 ? 1.0 : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL bilinear(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 1.0 ? 1.0 - t : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL bicubic(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    // 0.5t^3 - t^2 + 2/3 == (0.5t - 1.0) t^2 + 2/3
    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 1.0 ? (t * 0.5 - 1.0) * Math::pow2(t) + Math::k2Div3 :
               t < 2.0 ? Math::pow3(2.0 - t) / 6.0 : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL bell(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 0.5 ? 0.75 - Math::pow2(t) :
               t < 1.5 ? 0.50 * Math::pow2(t - 1.5) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL gauss(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);
    const double x = std::sqrt(Math::k2DivPi);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t <= 2.0 ? std::exp(Math::pow2(t) * -2.0) * x : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL hermite(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 1.0 ? (2.0 * t - 3.0) * Math::pow2(t) + 1.0 : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL hanning(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t <= 1.0 ? 0.5 + 0.5 * std::cos(t * Math::kPi) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL catrom(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 1.0 ? 0.5 * (2.0 + t * t * (t * 3.0 - 5.0)) :
               t < 2.0 ? 0.5 * (4.0 + t * (t * (5.0 - t) - 8.0)) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL bessel(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    B2D_UNUSED(data);
    const double x = Math::kPi * 0.25;

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t == 0.0    ? x :
               t <= 3.2383 ? ImageScalerUtils::bessel(t * Math::kPi, 1) / (2.0 * t) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL sinc(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    const double r = static_cast<const ImageScalerBuiltInParams*>(data)->radius;

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t == 0.0 ? 1.0 :
               t <= r   ? ImageScalerUtils::sinXDivX(t * Math::kPi) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL lanczos(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    const double r = static_cast<const ImageScalerBuiltInParams*>(data)->radius;
    const double x = Math::kPi;
    const double y = Math::kPi / r;

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t == 0.0 ? 1.0 :
               t <= r   ? ImageScalerUtils::lanczos(t * x, t * y) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL blackman(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    const double r = static_cast<const ImageScalerBuiltInParams*>(data)->radius;
    const double x = Math::kPi;
    const double y = Math::kPi / r;

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t == 0.0 ? 1.0 :
               t <= r   ? ImageScalerUtils::blackman(t * x, t * y) : 0.0;
    }

    return kErrorOk;
  }

  static Error B2D_CDECL mitchell(double* dst, const double* tArray, size_t n, const void* data) noexcept {
    const ImageScalerBuiltInParams::Mitchell& p = static_cast<const ImageScalerBuiltInParams*>(data)->mitchell;

    for (size_t i = 0; i < n; i++) {
      double t = tArray[i];
      dst[i] = t < 1.0 ? p.p0 + Math::pow2(t) * (p.p2 + t * p.p3) :
               t < 2.0 ? p.q0 + t             * (p.q1 + t * (p.q2 + t * p.q3)) : 0.0;
    }

    return kErrorOk;
  }
}

// ============================================================================
// [b2d::ImageScaler - Weights]
// ============================================================================

static Error B2D_CDECL bImageScaleWeights(ImageScaler::Data* d, uint32_t dir, ImageScaler::CustomFunc func, const void* data) noexcept {
  int32_t* weightList = d->weightList[dir];
  ImageScaler::Record* recordList = d->recordList[dir];

  int dstSize = d->dstSize[dir];
  int srcSize = d->srcSize[dir];
  int kernelSize = d->kernelSize[dir];

  double radius = d->radius[dir];
  double factor = d->factor[dir];
  double scale = d->scale[dir];
  int32_t isUnbound = 0;

  MemBufferTmp<512> wMem;
  double* wData = static_cast<double*>(wMem.alloc(kernelSize * sizeof(double)));

  if (B2D_UNLIKELY(!wData))
    return DebugUtils::errored(kErrorNoMemory);

  for (int i = 0; i < dstSize; i++) {
    double wPos = (double(i) + 0.5) / scale - 0.5;
    double wSum = 0.0;

    int left = int(wPos - radius);
    int right = left + kernelSize;
    int wIndex;

    // Calculate all weights for the destination pixel.
    wPos -= left;
    for (wIndex = 0; wIndex < kernelSize; wIndex++, wPos -= 1.0) {
      wData[wIndex] = std::abs(wPos * factor);
    }

    // User function can fail.
    B2D_PROPAGATE(func(wData, wData, kernelSize, data));

    // Remove padded pixels from left and right.
    wIndex = 0;
    while (left < 0) {
      double w = wData[wIndex];
      wData[++wIndex] += w;
      left++;
    }

    int wCount = kernelSize;
    while (right > srcSize) {
      B2D_ASSERT(wCount > 0);
      double w = wData[--wCount];
      wData[wCount - 1] += w;
      right--;
    }

    recordList[i].pos = 0;
    recordList[i].count = 0;

    if (wIndex < wCount) {
      // Sum all weights.
      int j;

      for (j = wIndex; j < wCount; j++) {
        double w = wData[j];
        wSum += w;
      }

      int iStrongest = 0;
      int32_t iSum = 0;
      int32_t iMax = 0;

      double wScale = 65535 / wSum;
      for (j = wIndex; j < wCount; j++) {
        int32_t w = int32_t(wData[j] * wScale) >> 8;

        // Remove zero weight from the beginning of the list.
        if (w == 0 && wIndex == j) {
          wIndex++;
          left++;
          continue;
        }

        weightList[j - wIndex] = w;
        iSum += w;
        isUnbound |= w;

        if (iMax < w) {
          iMax = w;
          iStrongest = j - wIndex;
        }
      }

      // Normalize the strongest weight so the sum matches `0x100`.
      if (iSum != 0x100)
        weightList[iStrongest] += int32_t(0x100) - iSum;

      // `wCount` is now absolute size of weights in `weightList`.
      wCount -= wIndex;

      // Remove all zero weights from the end of the weight array.
      while (wCount > 0 && weightList[wCount - 1] == 0)
        wCount--;

      if (wCount) {
        B2D_ASSERT(left >= 0);
        recordList[i].pos = left;
        recordList[i].count = wCount;
      }
    }

    weightList += kernelSize;
  }

  d->isUnbound[dir] = isUnbound < 0;
  return kErrorOk;
}

// ============================================================================
// [b2d::ImageScaler - Horz]
// ============================================================================

static void B2D_CDECL bImageScaleHorzPrgb32(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  int dw = d->dstSize[0];
  int sh = d->srcSize[1];
  int kernelSize = d->kernelSize[0];

  if (!d->isUnbound[ImageScaler::kDirHorz]) {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 4;
        const int32_t* wp = weightList;

        uint32_t cr_cb = 0x00800080;
        uint32_t ca_cg = 0x00800080;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t w0 = wp[0];

          ca_cg += ((p0 >> 8) & 0x00FF00FF) * w0;
          cr_cb += ((p0     ) & 0x00FF00FF) * w0;

          sp += 4;
          wp += 1;
        }
        reinterpret_cast<uint32_t*>(dp)[0] = (ca_cg & 0xFF00FF00) + ((cr_cb & 0xFF00FF00) >> 8);

        recordList += 1;
        weightList += kernelSize;

        dp += 4;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
  else {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 4;
        const int32_t* wp = weightList;

        int32_t ca = 0x80;
        int32_t cr = 0x80;
        int32_t cg = 0x80;
        int32_t cb = 0x80;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          int32_t w0 = wp[0];

          ca += int32_t((p0 >> 24)       ) * w0;
          cr += int32_t((p0 >> 16) & 0xFF) * w0;
          cg += int32_t((p0 >>  8) & 0xFF) * w0;
          cb += int32_t((p0      ) & 0xFF) * w0;

          sp += 4;
          wp += 1;
        }

        ca = Math::bound<int32_t>(ca >> 8, 0, 255);
        cr = Math::bound<int32_t>(cr >> 8, 0, ca);
        cg = Math::bound<int32_t>(cg >> 8, 0, ca);
        cb = Math::bound<int32_t>(cb >> 8, 0, ca);
        reinterpret_cast<uint32_t*>(dp)[0] = ArgbUtil::pack32(ca, cr, cg, cb);

        recordList += 1;
        weightList += kernelSize;

        dp += 4;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
}

static void B2D_CDECL bImageScaleHorzXrgb32(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  int dw = d->dstSize[0];
  int sh = d->srcSize[1];
  int kernelSize = d->kernelSize[0];

  if (!d->isUnbound[ImageScaler::kDirHorz]) {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 4;
        const int32_t* wp = weightList;

        uint32_t cx_cg = 0x00008000;
        uint32_t cr_cb = 0x00800080;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t w0 = wp[0];

          cx_cg += (p0 & 0x0000FF00) * w0;
          cr_cb += (p0 & 0x00FF00FF) * w0;

          sp += 4;
          wp += 1;
        }

        reinterpret_cast<uint32_t*>(dp)[0] = 0xFF000000 + (((cx_cg & 0x00FF0000) | (cr_cb & 0xFF00FF00)) >> 8);

        recordList += 1;
        weightList += kernelSize;

        dp += 4;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
  else {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 4;
        const int32_t* wp = weightList;

        int32_t cr = 0x80;
        int32_t cg = 0x80;
        int32_t cb = 0x80;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          int32_t w0 = wp[0];

          cr += int32_t((p0 >> 16) & 0xFF) * w0;
          cg += int32_t((p0 >>  8) & 0xFF) * w0;
          cb += int32_t((p0      ) & 0xFF) * w0;

          sp += 4;
          wp += 1;
        }

        cr = Math::bound<int32_t>(cr >> 8, 0, 255);
        cg = Math::bound<int32_t>(cg >> 8, 0, 255);
        cb = Math::bound<int32_t>(cb >> 8, 0, 255);
        reinterpret_cast<uint32_t*>(dp)[0] = ArgbUtil::pack32(0xFF, cr, cg, cb);

        recordList += 1;
        weightList += kernelSize;

        dp += 4;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
}

static void B2D_CDECL bImageScaleHorzA8(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  int dw = d->dstSize[0];
  int sh = d->srcSize[1];
  int kernelSize = d->kernelSize[0];

  if (!d->isUnbound[ImageScaler::kDirHorz]) {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 1;
        const int32_t* wp = weightList;

        uint32_t ca = 0x80;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = sp[0];
          uint32_t w0 = wp[0];

          ca += p0 * w0;

          sp += 1;
          wp += 1;
        }

        dp[0] = uint8_t(ca >> 8);

        recordList += 1;
        weightList += kernelSize;

        dp += 1;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
  else {
    for (int y = 0; y < sh; y++) {
      const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirHorz];
      const int32_t* weightList = d->weightList[ImageScaler::kDirHorz];

      uint8_t* dp = dstLine;

      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcLine + recordList->pos * 1;
        const int32_t* wp = weightList;

        int32_t ca = 0x80;

        for (int i = recordList->count; i; i--) {
          uint32_t p0 = sp[0];
          int32_t w0 = wp[0];

          ca += (int32_t)p0 * w0;

          sp += 1;
          wp += 1;
        }

        dp[0] = Support::clampToByte(ca >> 8);

        recordList += 1;
        weightList += kernelSize;

        dp += 1;
      }

      dstLine += dstStride;
      srcLine += srcStride;
    }
  }
}

// ============================================================================
// [b2d::ImageScaler - Vert]
// ============================================================================

static void B2D_CDECL bImageScaleVertPrgb32(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  int dw = d->dstSize[0];
  int dh = d->dstSize[1];
  int kernelSize = d->kernelSize[ImageScaler::kDirVert];

  const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirVert];
  const int32_t* weightList = d->weightList[ImageScaler::kDirVert];

  if (!d->isUnbound[ImageScaler::kDirVert]) {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int count = recordList->count;
      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        uint32_t cr_cb = 0x00800080;
        uint32_t ca_cg = 0x00800080;

        for (int i = count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t w0 = wp[0];

          ca_cg += ((p0 >> 8) & 0x00FF00FF) * w0;
          cr_cb += ((p0     ) & 0x00FF00FF) * w0;

          sp += srcStride;
          wp += 1;
        }

        reinterpret_cast<uint32_t*>(dp)[0] = (ca_cg & 0xFF00FF00) + ((cr_cb & 0xFF00FF00) >> 8);

        dp += 4;
        srcData += 4;
      }

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
  else {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int count = recordList->count;
      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        int32_t ca = 0x80;
        int32_t cr = 0x80;
        int32_t cg = 0x80;
        int32_t cb = 0x80;

        for (int i = count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          int32_t w0 = wp[0];

          ca += int32_t((p0 >> 24)       ) * w0;
          cr += int32_t((p0 >> 16) & 0xFF) * w0;
          cg += int32_t((p0 >>  8) & 0xFF) * w0;
          cb += int32_t((p0      ) & 0xFF) * w0;

          sp += srcStride;
          wp += 1;
        }

        ca = Math::bound<int32_t>(ca >> 8, 0, 255);
        cr = Math::bound<int32_t>(cr >> 8, 0, ca);
        cg = Math::bound<int32_t>(cg >> 8, 0, ca);
        cb = Math::bound<int32_t>(cb >> 8, 0, ca);
        reinterpret_cast<uint32_t*>(dp)[0] = ArgbUtil::pack32(ca, cr, cg, cb);

        dp += 4;
        srcData += 4;
      }

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
}

static void B2D_CDECL bImageScaleVertXrgb32(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  int dw = d->dstSize[0];
  int dh = d->dstSize[1];
  int kernelSize = d->kernelSize[ImageScaler::kDirVert];

  const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirVert];
  const int32_t* weightList = d->weightList[ImageScaler::kDirVert];

  if (!d->isUnbound[ImageScaler::kDirVert]) {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int count = recordList->count;
      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        uint32_t cx_cg = 0x00008000;
        uint32_t cr_cb = 0x00800080;

        for (int i = count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t w0 = wp[0];

          cx_cg += (p0 & 0x0000FF00) * w0;
          cr_cb += (p0 & 0x00FF00FF) * w0;

          sp += srcStride;
          wp += 1;
        }

        reinterpret_cast<uint32_t*>(dp)[0] = 0xFF000000 + (((cx_cg & 0x00FF0000) | (cr_cb & 0xFF00FF00)) >> 8);

        dp += 4;
        srcData += 4;
      }

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
  else {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int count = recordList->count;
      for (int x = 0; x < dw; x++) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        int32_t cr = 0x80;
        int32_t cg = 0x80;
        int32_t cb = 0x80;

        for (int i = count; i; i--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          int32_t w0 = wp[0];

          cr += int32_t((p0 >> 16) & 0xFF) * w0;
          cg += int32_t((p0 >>  8) & 0xFF) * w0;
          cb += int32_t((p0      ) & 0xFF) * w0;

          sp += srcStride;
          wp += 1;
        }

        cr = Math::bound<int32_t>(cr >> 8, 0, 255);
        cg = Math::bound<int32_t>(cg >> 8, 0, 255);
        cb = Math::bound<int32_t>(cb >> 8, 0, 255);
        reinterpret_cast<uint32_t*>(dp)[0] = ArgbUtil::pack32(0xFF, cr, cg, cb);

        dp += 4;
        srcData += 4;
      }

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
}

static void B2D_CDECL bImageScaleVertBytes(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride, uint32_t wScale) noexcept {
  int dw = d->dstSize[0] * wScale;
  int dh = d->dstSize[1];
  int kernelSize = d->kernelSize[ImageScaler::kDirVert];

  const ImageScaler::Record* recordList = d->recordList[ImageScaler::kDirVert];
  const int32_t* weightList = d->weightList[ImageScaler::kDirVert];

  if (!d->isUnbound[ImageScaler::kDirVert]) {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int x = dw;
      int i = 0;
      int count = recordList->count;

      if (((intptr_t)dp & 0x7) == 0)
        goto BoundLarge;
      i = 8 - int((intptr_t)dp & 0x7);

BoundSmall:
      x -= i;
      do {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        uint32_t c0 = 0x80;

        for (int j = count; j; j--) {
          uint32_t p0 = sp[0];
          uint32_t w0 = wp[0];

          c0 += p0 * w0;

          sp += srcStride;
          wp += 1;
        }

        dp[0] = (uint8_t)(c0 >> 8);

        dp += 1;
        srcData += 1;
      } while (--i);

BoundLarge:
      while (x >= 8) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        uint32_t c0 = 0x00800080;
        uint32_t c1 = 0x00800080;
        uint32_t c2 = 0x00800080;
        uint32_t c3 = 0x00800080;

        for (int j = count; j; j--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t p1 = reinterpret_cast<const uint32_t*>(sp)[1];
          uint32_t w0 = wp[0];

          c0 += ((p0     ) & 0x00FF00FF) * w0;
          c1 += ((p0 >> 8) & 0x00FF00FF) * w0;
          c2 += ((p1     ) & 0x00FF00FF) * w0;
          c3 += ((p1 >> 8) & 0x00FF00FF) * w0;

          sp += srcStride;
          wp += 1;
        }

        reinterpret_cast<uint32_t*>(dp)[0] = ((c0 & 0xFF00FF00) >> 8) + (c1 & 0xFF00FF00);
        reinterpret_cast<uint32_t*>(dp)[1] = ((c2 & 0xFF00FF00) >> 8) + (c3 & 0xFF00FF00);

        dp += 8;
        srcData += 8;

        x -= 8;
      }

      i = x;
      if (i != 0)
        goto BoundSmall;

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
  else {
    for (int y = 0; y < dh; y++) {
      const uint8_t* srcData = srcLine + intptr_t(recordList->pos) * srcStride;
      uint8_t* dp = dstLine;

      int x = dw;
      int i = 0;
      int count = recordList->count;

      if (((size_t)dp & 0x3) == 0)
        goto UnboundLarge;
      i = 4 - int((intptr_t)dp & 0x3);

UnboundSmall:
      x -= i;
      do {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        int32_t c0 = 0x80;

        for (int j = count; j; j--) {
          uint32_t p0 = sp[0];
          int32_t w0 = wp[0];

          c0 += int32_t(p0) * w0;

          sp += srcStride;
          wp += 1;
        }

        dp[0] = (uint8_t)(uint32_t)Math::bound<int32_t>(c0 >> 8, 0, 255);

        dp += 1;
        srcData += 1;
      } while (--i);

UnboundLarge:
      while (x >= 4) {
        const uint8_t* sp = srcData;
        const int32_t* wp = weightList;

        int32_t c0 = 0x80;
        int32_t c1 = 0x80;
        int32_t c2 = 0x80;
        int32_t c3 = 0x80;

        for (int j = count; j; j--) {
          uint32_t p0 = reinterpret_cast<const uint32_t*>(sp)[0];
          uint32_t w0 = wp[0];

          c0 += ((p0      ) & 0xFF) * w0;
          c1 += ((p0 >>  8) & 0xFF) * w0;
          c2 += ((p0 >> 16) & 0xFF) * w0;
          c3 += ((p0 >> 24)       ) * w0;

          sp += srcStride;
          wp += 1;
        }

        reinterpret_cast<uint32_t*>(dp)[0] =
          (Math::bound<int32_t>(c0 >> 8, 0, 255)      ) +
          (Math::bound<int32_t>(c1 >> 8, 0, 255) <<  8) +
          (Math::bound<int32_t>(c2 >> 8, 0, 255) << 16) +
          (Math::bound<int32_t>(c3 >> 8, 0, 255) << 24) ;

        dp += 4;
        srcData += 4;

        x -= 4;
      }

      i = x;
      if (i != 0)
        goto UnboundSmall;

      recordList += 1;
      weightList += kernelSize;

      dstLine += dstStride;
    }
  }
}

static void B2D_CDECL bImageScaleVertA8(const ImageScaler::Data* d, uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride) noexcept {
  bImageScaleVertBytes(d, dstLine, dstStride, srcLine, srcStride, 1);
}

// ============================================================================
// [b2d::ImageScaler - Reset]
// ============================================================================

Error ImageScaler::reset(uint32_t resetPolicy) noexcept {
  if (_data == nullptr)
    return kErrorOk;

  if (resetPolicy == Globals::kResetSoft) {
    _data->dstSize[0] = 0;
    _data->dstSize[1] = 0;
    _data->srcSize[0] = 0;
    _data->srcSize[1] = 0;
  }
  else {
    _data->allocator->release(_data, _data->size);
    _data = nullptr;
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::ImageScaler - Interface]
// ============================================================================

Error ImageScaler::create(const IntSize& to, const IntSize& from, const Params& params, Allocator* allocator) noexcept {
  ImageScalerBuiltInParams p;

  ImageScaler::CustomFunc func;
  const void* funcData = &p;

  // --------------------------------------------------------------------------
  // [Setup Parameters]
  // --------------------------------------------------------------------------

  if (!to.isValid() || !from.isValid())
    return DebugUtils::errored(kErrorInvalidArgument);

  switch (params.filter()) {
    case kFilterNearest : func = ImageScalerFuncs::nearest ; p.setupRadius(1.0); break;
    case kFilterBilinear: func = ImageScalerFuncs::bilinear; p.setupRadius(1.0); break;
    case kFilterBicubic : func = ImageScalerFuncs::bicubic ; p.setupRadius(2.0); break;
    case kFilterBell    : func = ImageScalerFuncs::bell    ; p.setupRadius(1.5); break;
    case kFilterGauss   : func = ImageScalerFuncs::gauss   ; p.setupRadius(2.0); break;
    case kFilterHermite : func = ImageScalerFuncs::hermite ; p.setupRadius(1.0); break;
    case kFilterHanning : func = ImageScalerFuncs::hanning ; p.setupRadius(1.0); break;
    case kFilterCatrom  : func = ImageScalerFuncs::catrom  ; p.setupRadius(2.0); break;
    case kFilterBessel  : func = ImageScalerFuncs::bessel  ; p.setupRadius(3.2383); break;

    case kFilterSinc    : func = ImageScalerFuncs::sinc    ; p.setupRadius(params.radius()); break;
    case kFilterLanczos : func = ImageScalerFuncs::lanczos ; p.setupRadius(params.radius()); break;
    case kFilterBlackman: func = ImageScalerFuncs::blackman; p.setupRadius(params.radius()); break;

    case kFilterMitchell: {
      double b = params.mitchellB();
      double c = params.mitchellC();

      if (!std::isfinite(b) || !std::isfinite(c))
        return DebugUtils::errored(kErrorInvalidArgument);

      func = ImageScalerFuncs::mitchell;
      p.setupRadius(2.0);
      p.setupMitchell(b, c);
      break;
    }

    case kFilterCustom: {
      func = params.customFunc();
      funcData = params.customData();

      if (!func)
        return DebugUtils::errored(kErrorInvalidArgument);

      break;
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  if (!(p.radius >= 1.0 && p.radius <= 16.0))
    return DebugUtils::errored(kErrorInvalidArgument);

  if (!allocator)
    allocator = Allocator::host();

  // --------------------------------------------------------------------------
  // [Setup Weights]
  // --------------------------------------------------------------------------

  double scale[2];
  double factor[2];
  double radius[2];
  int kernelSize[2];
  int isUnbound[2];

  scale[0] = double(to.w()) / double(from.w());
  scale[1] = double(to.h()) / double(from.h());

  factor[0] = 1.0;
  factor[1] = 1.0;

  radius[0] = p.radius;
  radius[1] = p.radius;

  if (scale[0] < 1.0) { factor[0] = scale[0]; radius[0] = p.radius / scale[0]; }
  if (scale[1] < 1.0) { factor[1] = scale[1]; radius[1] = p.radius / scale[1]; }

  kernelSize[0] = int(Math::ceil(1.0 + 2.0 * radius[0]));
  kernelSize[1] = int(Math::ceil(1.0 + 2.0 * radius[1]));

  isUnbound[0] = false;
  isUnbound[1] = false;

  size_t wWeightDataSize = size_t(to.w()) * kernelSize[0] * sizeof(int32_t);
  size_t hWeightDataSize = size_t(to.h()) * kernelSize[1] * sizeof(int32_t);
  size_t wRecordDataSize = size_t(to.w()) * sizeof(Record);
  size_t hRecordDataSize = size_t(to.h()) * sizeof(Record);
  size_t dataSize = sizeof(Data) + wWeightDataSize + hWeightDataSize + wRecordDataSize + hRecordDataSize;

  // Release the previous data if allocators are not the same.
  if (_data && (_data->size < dataSize || _data->allocator != allocator)) {
    _data->allocator->release(_data, _data->size);
    _data = nullptr;
  }

  // Reallocate data if we need more space than previously.
  if (!_data) {
    size_t sizeAllocated = 0;
    _data = static_cast<Data*>(allocator->alloc(dataSize, sizeAllocated));

    if (B2D_UNLIKELY(!_data))
      return DebugUtils::errored(kErrorNoMemory);

    _data->allocator = allocator;
    _data->size = sizeAllocated;
  }

  // Init data.
  Data* d = _data;
  d->dstSize[0] = to.w();
  d->dstSize[1] = to.h();
  d->srcSize[0] = from.w();
  d->srcSize[1] = from.h();
  d->kernelSize[0] = kernelSize[0];
  d->kernelSize[1] = kernelSize[1];
  d->isUnbound[0] = isUnbound[0];
  d->isUnbound[1] = isUnbound[1];

  d->scale[0] = scale[0];
  d->scale[1] = scale[1];
  d->factor[0] = factor[0];
  d->factor[1] = factor[1];
  d->radius[0] = radius[0];
  d->radius[1] = radius[1];

  // Distribute the memory buffer.
  uint8_t* dataPtr = reinterpret_cast<uint8_t*>(d) + sizeof(Data);

  d->weightList[kDirHorz] = reinterpret_cast<int32_t*>(dataPtr); dataPtr += wWeightDataSize;
  d->weightList[kDirVert] = reinterpret_cast<int32_t*>(dataPtr); dataPtr += hWeightDataSize;

  d->recordList[kDirHorz] = reinterpret_cast<Record*>(dataPtr); dataPtr += wRecordDataSize;
  d->recordList[kDirVert] = reinterpret_cast<Record*>(dataPtr);

  // Built-in filters will probably never fail, however, custom filters can and
  // it wouldn't be safe to just continue.
  B2D_PROPAGATE(gImageScalerOps.weights(d, kDirHorz, func, funcData));
  B2D_PROPAGATE(gImageScalerOps.weights(d, kDirVert, func, funcData));

  return kErrorOk;
}

Error ImageScaler::processHorzData(uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride, uint32_t pixelFormat) const noexcept {
  B2D_ASSERT(isInitialized());
  gImageScalerOps.horz[pixelFormat](_data, dstLine, dstStride, srcLine, srcStride);
  return kErrorOk;
}

Error ImageScaler::processVertData(uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride, uint32_t pixelFormat) const noexcept {
  B2D_ASSERT(isInitialized());
  gImageScalerOps.vert[pixelFormat](_data, dstLine, dstStride, srcLine, srcStride);
  return kErrorOk;
}

// ============================================================================
// [b2d::ImageScaler - Runtime Handlers]
// ============================================================================

void ImageScalerOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  gImageScalerOps.weights = bImageScaleWeights;

  gImageScalerOps.horz[PixelFormat::kPRGB32] = bImageScaleHorzPrgb32;
  gImageScalerOps.horz[PixelFormat::kXRGB32] = bImageScaleHorzXrgb32;
  gImageScalerOps.horz[PixelFormat::kA8    ] = bImageScaleHorzA8;

  gImageScalerOps.vert[PixelFormat::kPRGB32] = bImageScaleVertPrgb32;
  gImageScalerOps.vert[PixelFormat::kXRGB32] = bImageScaleVertXrgb32;
  gImageScalerOps.vert[PixelFormat::kA8    ] = bImageScaleVertA8;

  #if B2D_MAYBE_SSE2
  // void ImageScalerOnInitSSE2() noexcept;
  // if (Runtime::hwInfo().hasSSE2())
  //   ImageScalerOnInitSSE2();
  #endif
}

B2D_END_NAMESPACE
