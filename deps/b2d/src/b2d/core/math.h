// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_MATH_H
#define _B2D_CORE_MATH_H

// [Dependencies]
#include "../core/globals.h"
#include "../core/simd.h"

#include <float.h>
#include <math.h>

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::TypeDefs]
// ============================================================================

typedef Error (B2D_CDECL* EvaluateFunc)(void* funcData, double* dst, const double* t, size_t n);

// ============================================================================
// [b2d::FloatBits / DoubleBits]
// ============================================================================

// Data structures that represent `float` and `double`:
//
//     +-----------+------+------------+----------+------+
//     | Data Type | Sign |  Exponent  | Fraction | Bias |
//     +-----------+------+------------+----------+------+
//     | float     | 1[31]|  8 [30-23] |23 [22-00]|  127 |
//     | double    | 1[63]| 11 [62-52] |52 [51-00]| 1023 |
//     +-----------+------+------------+----------+------+

//! Single-precision floating-point representation and utilities.
union FloatBits {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  static B2D_INLINE FloatBits fromFloat(float val) noexcept { FloatBits u; u.f = val; return u; }
  static B2D_INLINE FloatBits fromUInt(uint32_t val) noexcept { FloatBits u; u.u = val; return u; }

  // --------------------------------------------------------------------------
  // [Check]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isNaN() const noexcept { return (u & 0x7FFFFFFFu) > 0x7F800000u; }
  B2D_INLINE bool isInf() const noexcept { return (u & 0x7FFFFFFFu) == 0x7F800000u; }
  B2D_INLINE bool isFinite() const noexcept { return (u & 0x7F800000u) != 0x7F800000u; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  B2D_INLINE void setNaN() noexcept { u = 0x7FC00000u; }
  B2D_INLINE void setInf() noexcept { u = 0x7F800000u; }

  B2D_INLINE void fillSign() noexcept { u |= 0x80000000u; }
  B2D_INLINE void zeroSign() noexcept { u &= 0x7FFFFFFFu; }

  B2D_INLINE void fillMagnitude() noexcept { u |= 0x7FFFFFFFu; }
  B2D_INLINE void zeroMagnitude() noexcept { u &= 0x80000000u; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! UInt32 value.
  //!
  //! NOTE: Has to be the first so it's possible to set it as `FloatBits = {...};`.
  uint32_t u;

  //! Float value.
  float f;
};

//! Double-precision floating-point representation and utilities.
union DoubleBits {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  static B2D_INLINE DoubleBits fromDouble(double val) noexcept { DoubleBits u; u.d = val; return u; }
  static B2D_INLINE DoubleBits fromUInt(uint64_t val) noexcept { DoubleBits u; u.u = val; return u; }

  // --------------------------------------------------------------------------
  // [Check]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isNaN() const noexcept {
    if (B2D_ARCH_BITS >= 64)
      return (u & 0x7FFFFFFFFFFFFFFFu) > 0x7F80000000000000u;
    else
      return ((hi & 0x7FF00000u)) == 0x7FF00000u && ((hi & 0x000FFFFFu) | lo) != 0x00000000u;
  }

  B2D_INLINE bool isInf() const noexcept {
    if (B2D_ARCH_BITS >= 64)
      return (u & 0x7FFFFFFFFFFFFFFFu) == 0x7FF0000000000000u;
    else
      return (hi & 0x7FFFFFFFu) == 0x7FF00000u && lo == 0x00000000u;
  }

  B2D_INLINE bool isFinite() const noexcept {
    if (B2D_ARCH_BITS >= 64)
      return (u & 0x7FF0000000000000u) != 0x7FF0000000000000u;
    else
      return (hi & 0x7FF00000u) != 0x7FF00000u;
  }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  B2D_INLINE void setNaN() noexcept { u = 0x7FF8000000000000u; }
  B2D_INLINE void setInf() noexcept { u = 0x7FF0000000000000u; }

  B2D_INLINE void fillSign() noexcept { hi |= 0x80000000u; }
  B2D_INLINE void zeroSign() noexcept { hi &= 0x7FFFFFFFu; }

  B2D_INLINE void fillMagnitude() noexcept { u |= 0x7FFFFFFFFFFFFFFFu; }
  B2D_INLINE void zeroMagnitude() noexcept { u &= 0x8000000000000000u; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! UInt64 value.
  //!
  //! NOTE: Has to be first so it's possible to set it as `DoubleBits = {...};`.
  uint64_t u;

#if B2D_ARCH_LE
  struct { uint32_t lo; uint32_t hi; };
#else
  struct { uint32_t hi; uint32_t lo; };
#endif

  //! Double value.
  double d;
};

// ============================================================================
// [b2d::Math - Namespace]
// ============================================================================

namespace Math {

// ============================================================================
// [b2d::Math - Constants]
// ============================================================================

static constexpr double kE           = 2.71828182845904523536;  //!< e.
static constexpr double kLn2         = 0.69314718055994530942;  //!< ln(2).
static constexpr double kLn10        = 2.30258509299404568402;  //!< ln(10).
static constexpr double kLog2E       = 1.44269504088896340736;  //!< log2(e).
static constexpr double kLog10E      = 0.43429448190325182765;  //!< log10(e).

static constexpr double kPi          = 3.14159265358979323846;  //!< pi.
static constexpr double k2Pi         = 6.28318530717958647692;  //!< pi * 2.
static constexpr double k3Pi         = 9.42477796076937971539;  //!< pi * 3.
static constexpr double k4Pi         = 12.56637061435917295385; //!< pi * 4.
static constexpr double k1_5Pi       = 4.71238898038468985769;  //!< pi * 1.5.
static constexpr double k2_5Pi       = 7.85398163397448309616;  //!< pi * 2.5.
static constexpr double k3_5Pi       = 10.99557428756427633462; //!< pi * 3.5
static constexpr double k4_5Pi       = 14.13716694115406957308; //!< pi * 4.5
static constexpr double kPiDiv2      = 1.57079632679489661923;  //!< pi / 2.
static constexpr double kPiDiv3      = 1.04719755119659774615;  //!< pi / 3.
static constexpr double kPiDiv4      = 0.78539816339744830962;  //!< pi / 4.

static constexpr double k1DivPi      = 0.31830988618379067154;  //!< 1 / pi.
static constexpr double k1Div2Pi     = 0.15915494309189533577;  //!< 1 / (2 * pi).
static constexpr double k1Div3Pi     = 0.10610329539459689051;  //!< 1 / (3 * pi).
static constexpr double k1Div4Pi     = 0.07957747154594766788;  //!< 1 / (4 * pi).
static constexpr double k2DivPi      = 0.63661977236758134308;  //!< 2 / pi.
static constexpr double k2Div3Pi     = 0.21220659078919378103;  //!< 2 / (3 * pi).
static constexpr double k2Div4Pi     = 0.15915494309189533577;  //!< 2 / (4 * pi).
static constexpr double k3DivPi      = 0.95492965855137201461;  //!< 3 / pi.
static constexpr double k3Div2Pi     = 0.47746482927568600731;  //!< 3 / (2 * pi).
static constexpr double k3Div4Pi     = 0.23873241463784300365;  //!< 3 / (4 * pi).

static constexpr double k1DivSqrtPi  = 0.56418958354775628695;  //!< 1 / sqrt(pi).
static constexpr double k2DivSqrtPi  = 1.12837916709551257390;  //!< 2 / sqrt(pi).
static constexpr double k3DivSqrtPi  = 1.69256875064326886084;  //!< 3 / sqrt(pi).

static constexpr double kSqrtHalf    = 0.707107;                //!< sqrt(0.5).
static constexpr double kSqrt2       = 1.41421356237309504880;  //!< sqrt(2).
static constexpr double kSqrt3       = 1.73205080756887729353;  //!< sqrt(3).

static constexpr double k1DivSqrt2   = 0.70710678118654752440;  //!< 1 / sqrt(2).
static constexpr double k1DivSqrt3   = 0.57735026918962576451;  //!< 1 / sqrt(3).

static constexpr double kKappa       = 0.55228474983079339840;  //!< 4/3 * (sqrt(2) - 1).
static constexpr double k1MinusKappa = 0.44771525016920660160;  //!< 1 - (4/3 * (sqrt(2) - 1)).

static constexpr double kGoldenRatio = 1.61803398874989484820;  //!< Golden ratio.

static constexpr double kDeg2Rad     = 0.017453292519943295769; //!< Degrees to radians.
static constexpr double kRad2Deg     = 57.29577951308232087680; //!< Radians to degrees.

static constexpr double k1Div2       = 0.5;                     //!< "1/2".
static constexpr double k1Div3       = 0.333333333333333333334; //!< "1/3".
static constexpr double k1Div4       = 0.25;                    //!< "1/4".
static constexpr double k1Div5       = 0.20;                    //!< "1/5".
static constexpr double k1Div6       = 0.166666666666666666667; //!< "1/6".
static constexpr double k1Div7       = 0.142857142857142857143; //!< "1/7".
static constexpr double k1Div8       = 0.125;                   //!< "1/8".
static constexpr double k1Div9       = 0.111111111111111111112; //!< "1/9".
static constexpr double k1Div255     = 0.003921568627450980392; //!< "1/255".
static constexpr double k1Div256     = 0.00390625;              //!< "1/256".
static constexpr double k1Div65535   = 0.000015259021896696422; //!< "1/65535".
static constexpr double k1Div65536   = 0.0000152587890625;      //!< "1/65536".

static constexpr double k2Div3       = 0.666666666666666666667; //!< "2/3".
static constexpr double k3Div4       = 0.75;                    //!< "3/4".
static constexpr double k4Div3       = 1.333333333333333333334; //!< "4/3".

static constexpr double kFlatness    = 0.20;                    //!< Default flatness.

static constexpr double kEpsilon     = 1e-14;                   //!< Epsilon (double).
static constexpr float  kEpsilonF    = 1e-8f;                   //!< Epsilon (float).

static constexpr double kIntersectionEpsilon = 1e-30;           //!< Default intersection epsilon
static constexpr double kCollinearityEpsilon = 1e-30;           //!< Default collinearity epsilon
static constexpr double kDistanceEpsilon     = 1e-14;           //!< Coinciding points maximal distance.

static constexpr double kAfter0  = 1.40129846432481707092e-45;  //!< Value after 0.0.
static constexpr double kBefore1 = 0.999999940395355224609375;  //!< Value before 1.0.

// ============================================================================
// [b2d::Math - Epsilon]
// ============================================================================

template<typename T>
constexpr T epsilon() noexcept = delete;
template<>
constexpr float epsilon<float>() noexcept { return kEpsilonF; }
template<>
constexpr double epsilon<double>() noexcept { return kEpsilon; }

// ============================================================================
// [b2d::Math - Normalize]
// ============================================================================

//! Normalize a floating point by adding `0` to it.
template<typename T>
constexpr T normalize(const T& x) noexcept { return x + T(0); }

// ============================================================================
// [b2d::Math - Abs]
// ============================================================================

template<typename T>
constexpr T abs(const T& x) noexcept { return x < 0 ? -x : x; }

// ============================================================================
// [b2d::Math - Min/Max/Bound]
// ============================================================================

// NOTE: It's not like we want to duplicate `std::min<>` and `std::max<>`, but
// we want these constexpr by default and also want to provide implementations
// for common geometric primitives defined in code part of Blend2D.

template<typename T>
constexpr T min(const T& a, const T& b) noexcept { return b < a ? b : a; }

template<typename T>
constexpr T max(const T& a, const T& b) noexcept { return a < b ? b : a; }

//! Bound a value `x` to be at least `a` and at most `b`.
template<typename T>
constexpr T bound(const T& x, const T& a, const T& b) noexcept { return min(max(x, a), b); }

// ============================================================================
// [b2d::Math - Rounding]
// ============================================================================

// +--------------------------+-----+-----+-----+-----+-----+-----+-----+-----+
// | Input                    |-1.5 |-0.9 |-0.5 |-0.1 | 0.1 | 0.5 | 0.9 | 1.5 |
// +--------------------------+-----+-----+-----+-----+-----+-----+-----+-----+
// | Trunc                    |-1.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 1.0 |
// | Floor                    |-2.0 |-1.0 |-1.0 |-1.0 | 0.0 | 0.0 | 0.0 | 1.0 |
// | Ceil                     |-1.0 | 0.0 | 0.0 | 0.0 | 1.0 | 1.0 | 1.0 | 2.0 |
// | Round to nearest         |-1.0 |-1.0 | 0.0 | 0.0 | 0.0 | 1.0 | 1.0 | 2.0 |
// | Round to even            |-2.0 |-1.0 | 0.0 | 0.0 | 0.0 | 0.0 | 1.0 | 2.0 |
// | Round away from zero     |-2.0 |-1.0 |-1.0 | 0.0 | 0.0 | 1.0 | 1.0 | 2.0 |
// +--------------------------+-----+-----+-----+-----+-----+-----+-----+-----+

// Rounding is very expensive on X86 as it has to alter rounding bits of the
// FPU/SSE state. The only method which is cheap is `rint()`, which uses the
// current SSE state to round a floating point. The following functions can be
// used to implement all rounding operators:
//
//   float roundeven(float x) {
//     float maxn = pow(2, 23);                  // 8388608
//     float magic = pow(2, 23) + pow(2, 22);    // 12582912
//     return x >= maxn ? x : x + magic - magic;
//   }
//
//   double roundeven(double x) {
//     double maxn = pow(2, 52);                 // 4503599627370496
//     double magic = pow(2, 52) + pow(2, 51);   // 6755399441055744
//     return x >= maxn ? x : x + magic - magic;
//   }

#if B2D_ARCH_SSE4_1
namespace Internal {
  // SSE4.1 implementation.
  template<int ControlFlags>
  static B2D_INLINE __m128 roundf_sse4_1(float x) noexcept {
    __m128 y = SIMD::vcvtf32f128(x);
    return _mm_round_ss(y, y, ControlFlags | _MM_FROUND_NO_EXC);
  }
}
static B2D_INLINE float nearby(float x) noexcept { return _mm_cvtss_f32(Internal::roundf_sse4_1<_MM_FROUND_CUR_DIRECTION>(x)); }
static B2D_INLINE float floor (float x) noexcept { return _mm_cvtss_f32(Internal::roundf_sse4_1<_MM_FROUND_TO_NEG_INF   >(x)); }
static B2D_INLINE float ceil  (float x) noexcept { return _mm_cvtss_f32(Internal::roundf_sse4_1<_MM_FROUND_TO_POS_INF   >(x)); }
static B2D_INLINE float trunc (float x) noexcept { return _mm_cvtss_f32(Internal::roundf_sse4_1<_MM_FROUND_TO_ZERO      >(x)); }
#elif B2D_ARCH_SSE
// SSE implementation.
static B2D_INLINE float nearby(float x) noexcept {
  __m128 src = SIMD::vcvtf32f128(x);
  __m128 maxn = _mm_set_ss(8388608.0f);
  __m128 magic = _mm_set_ss(12582912.0f);

  __m128 msk = _mm_cmpnlt_ss(src, maxn);
  __m128 rnd = _mm_sub_ss(_mm_add_ss(src, magic), magic);

  return SIMD::vcvtf128f32(SIMD::vblendmask(rnd, src, msk));
}

static B2D_INLINE float floor(float x) noexcept {
  __m128 src = SIMD::vcvtf32f128(x);
  __m128 maxn = _mm_set_ss(8388608.0f);
  __m128 magic = _mm_set_ss(12582912.0f);

  __m128 msk = _mm_cmpnlt_ss(src, maxn);
  __m128 rnd = _mm_sub_ss(_mm_add_ss(src, magic), magic);
  __m128 maybeone = _mm_and_ps(_mm_cmplt_ss(src, rnd), _mm_set_ss(1.0f));

  return SIMD::vcvtf128f32(SIMD::vblendmask(_mm_sub_ss(rnd, maybeone), src, msk));
}

static B2D_INLINE float ceil(float x) noexcept {
  __m128 src = SIMD::vcvtf32f128(x);
  __m128 maxn = _mm_set_ss(8388608.0f);
  __m128 magic = _mm_set_ss(12582912.0f);

  __m128 msk = _mm_cmpnlt_ss(src, maxn);
  __m128 rnd = _mm_sub_ss(_mm_add_ss(src, magic), magic);
  __m128 maybeone = _mm_and_ps(_mm_cmpnle_ss(src, rnd), _mm_set_ss(1.0f));

  return SIMD::vcvtf128f32(SIMD::vblendmask(_mm_add_ss(rnd, maybeone), src, msk));
}

static B2D_INLINE float trunc(float x) noexcept {
  static const uint32_t sep_mask[1] = { 0x7FFFFFFFu };

  __m128 src = SIMD::vcvtf32f128(x);
  __m128 sep = _mm_load_ss(reinterpret_cast<const float*>(sep_mask));

  __m128 src_abs = _mm_and_ps(src, sep);
  __m128 sign = _mm_andnot_ps(sep, src);

  __m128 maxn = _mm_set_ss(8388608.0f);
  __m128 magic = _mm_set_ss(12582912.0f);

  __m128 msk = _mm_or_ps(_mm_cmpnlt_ss(src_abs, maxn), sign);
  __m128 rnd = _mm_sub_ss(_mm_add_ss(src_abs, magic), magic);
  __m128 maybeone = _mm_and_ps(_mm_cmplt_ss(src_abs, rnd), _mm_set_ss(1.0f));

  return SIMD::vcvtf128f32(SIMD::vblendmask(_mm_sub_ss(rnd, maybeone), src, msk));
}
#else
// Portable implementation.
static B2D_INLINE float nearby(float x) noexcept { return ::rintf(x); }
static B2D_INLINE float floor(float x) noexcept { return ::floorf(x); }
static B2D_INLINE float ceil(float x) noexcept { return ::ceilf(x); }
static B2D_INLINE float trunc(float x) noexcept { return ::truncf(x); }
#endif

#if B2D_ARCH_SSE4_1
namespace Internal {
  // SSE4.1 implementation.
  template<int ControlFlags>
  static B2D_INLINE __m128d roundd_sse4_1(double x) noexcept {
    __m128d y = SIMD::vcvtd64d128(x);
    return _mm_round_sd(y, y, ControlFlags | _MM_FROUND_NO_EXC);
  }
}
static B2D_INLINE double nearby(double x) noexcept { return _mm_cvtsd_f64(Internal::roundd_sse4_1<_MM_FROUND_CUR_DIRECTION>(x)); }
static B2D_INLINE double floor (double x) noexcept { return _mm_cvtsd_f64(Internal::roundd_sse4_1<_MM_FROUND_TO_NEG_INF   >(x)); }
static B2D_INLINE double ceil  (double x) noexcept { return _mm_cvtsd_f64(Internal::roundd_sse4_1<_MM_FROUND_TO_POS_INF   >(x)); }
static B2D_INLINE double trunc (double x) noexcept { return _mm_cvtsd_f64(Internal::roundd_sse4_1<_MM_FROUND_TO_ZERO      >(x)); }
#elif B2D_ARCH_SSE2
// SSE2 implementation.
static B2D_INLINE double nearby(double x) noexcept {
  __m128d src = SIMD::vcvtd64d128(x);
  __m128d maxn = _mm_set_sd(4503599627370496.0);
  __m128d magic = _mm_set_sd(6755399441055744.0);

  __m128d msk = _mm_cmpnlt_sd(src, maxn);
  __m128d rnd = _mm_sub_sd(_mm_add_sd(src, magic), magic);

  return SIMD::vcvtd128d64(SIMD::vblendmask(rnd, src, msk));
}

static B2D_INLINE double floor(double x) noexcept {
  __m128d src = SIMD::vcvtd64d128(x);
  __m128d maxn = _mm_set_sd(4503599627370496.0);
  __m128d magic = _mm_set_sd(6755399441055744.0);

  __m128d msk = _mm_cmpnlt_sd(src, maxn);
  __m128d rnd = _mm_sub_sd(_mm_add_sd(src, magic), magic);
  __m128d maybeone = _mm_and_pd(_mm_cmplt_sd(src, rnd), _mm_set_sd(1.0));

  return SIMD::vcvtd128d64(SIMD::vblendmask(_mm_sub_sd(rnd, maybeone), src, msk));
}

static B2D_INLINE double ceil(double x) noexcept {
  __m128d src = SIMD::vcvtd64d128(x);
  __m128d maxn = _mm_set_sd(4503599627370496.0);
  __m128d magic = _mm_set_sd(6755399441055744.0);

  __m128d msk = _mm_cmpnlt_sd(src, maxn);
  __m128d rnd = _mm_sub_sd(_mm_add_sd(src, magic), magic);
  __m128d maybeone = _mm_and_pd(_mm_cmpnle_sd(src, rnd), _mm_set_sd(1.0));

  return SIMD::vcvtd128d64(SIMD::vblendmask(_mm_add_sd(rnd, maybeone), src, msk));
}

static B2D_INLINE double trunc(double x) noexcept {
  static const uint64_t sep_mask[1] = { 0x7FFFFFFFFFFFFFFFu };

  __m128d src = _mm_set_sd(x);
  __m128d sep = _mm_load_sd(reinterpret_cast<const double*>(sep_mask));

  __m128d src_abs = _mm_and_pd(src, sep);
  __m128d sign = _mm_andnot_pd(sep, src);

  __m128d maxn = _mm_set_sd(4503599627370496.0);
  __m128d magic = _mm_set_sd(6755399441055744.0);

  __m128d msk = _mm_or_pd(_mm_cmpnlt_sd(src_abs, maxn), sign);
  __m128d rnd = _mm_sub_sd(_mm_add_sd(src_abs, magic), magic);
  __m128d maybeone = _mm_and_pd(_mm_cmplt_sd(src_abs, rnd), _mm_set_sd(1.0));

  return SIMD::vcvtd128d64(SIMD::vblendmask(_mm_sub_sd(rnd, maybeone), src, msk));
}
#else
// Portable implementation.
static B2D_INLINE double nearby(double x) noexcept { return ::rint(x); }
static B2D_INLINE double floor(double x) noexcept { return ::floor(x); }
static B2D_INLINE double ceil(double x) noexcept { return ::ceil(x); }
static B2D_INLINE double trunc(double x) noexcept { return ::trunc(x); }
#endif

static B2D_INLINE float round(float x) noexcept {
  float y = floor(x);
  return y + (x - y >= 0.5f ? 1.0f : 0.0f);
}

static B2D_INLINE double round(double x) noexcept {
  double y = floor(x);
  return y + (x - y >= 0.5 ? 1.0 : 0.0);
}

static B2D_INLINE float roundeven(float x) noexcept { return nearby(x); }
static B2D_INLINE double roundeven(double x) noexcept { return nearby(x); }

// ============================================================================
// [b2d::Math - Float/Double -> Int]
// ============================================================================

static B2D_INLINE int inearby(float x) noexcept {
#if B2D_ARCH_SSE
  return _mm_cvtss_si32(SIMD::vcvtf32f128(x));
#elif B2D_ARCH_X86 == 32 && B2D_CXX_MSC
  int y;
  __asm {
    fld   dword ptr [x]
    fistp dword ptr [y]
  }
  return y;
#elif B2D_ARCH_X86 == 32 && B2D_CXX_GNU
  int y;
  __asm__ __volatile__("flds %1\n" "fistpl %0\n" : "=m" (y) : "m" (x));
  return y;
#else
# error "[b2d] Math::inearby() - Unsupported architecture or compiler."
#endif
}

static B2D_INLINE int inearby(double x) noexcept {
#if B2D_ARCH_SSE2
  return _mm_cvtsd_si32(SIMD::vcvtd64d128(x));
#elif B2D_ARCH_X86 == 32 && B2D_CXX_MSC
  int y;
  __asm {
    fld   qword ptr [x]
    fistp dword ptr [y]
  }
  return y;
#elif B2D_ARCH_X86 == 32 && B2D_CXX_GNU
  int y;
  __asm__ __volatile__("fldl %1\n" "fistpl %0\n" : "=m" (y) : "m" (x));
  return y;
#else
# error "[b2d] Math::inearby() - Unsupported architecture or compiler."
#endif
}

static B2D_INLINE int itrunc(float x) noexcept { return int(x); }
static B2D_INLINE int itrunc(double x) noexcept { return int(x); }

#if B2D_ARCH_SSE4_1
static B2D_INLINE int ifloor(float x) noexcept { return _mm_cvttss_si32(Internal::roundf_sse4_1<_MM_FROUND_TO_NEG_INF>(x)); }
static B2D_INLINE int iceil(float x) noexcept { return _mm_cvttss_si32(Internal::roundf_sse4_1<_MM_FROUND_TO_POS_INF>(x)); }

static B2D_INLINE int ifloor(double x) noexcept { return _mm_cvttsd_si32(Internal::roundd_sse4_1<_MM_FROUND_TO_NEG_INF>(x)); }
static B2D_INLINE int iceil(double x) noexcept { return _mm_cvttsd_si32(Internal::roundd_sse4_1<_MM_FROUND_TO_POS_INF>(x)); }
#else
static B2D_INLINE int ifloor(float x) noexcept { int y = inearby(x); return y - (float(y) > x); }
static B2D_INLINE int ifloor(double x) noexcept { int y = inearby(x); return y - (double(y) > x); }

static B2D_INLINE int iceil(float x) noexcept { int y = inearby(x); return y + (float(y) < x); }
static B2D_INLINE int iceil(double x) noexcept { int y = inearby(x); return y + (double(y) < x); }
#endif

static B2D_INLINE int iround(float x) noexcept { int y = inearby(x); return y + (float(y) - x == -0.5f); }
static B2D_INLINE int iround(double x) noexcept { int y = inearby(x); return y + (double(y) - x == -0.5);  }

static B2D_INLINE int iroundeven(float x) noexcept { return inearby(x); }
static B2D_INLINE int iroundeven(double x) noexcept { return inearby(x); }

// ============================================================================
// [b2d::Math - Float/Double -> Int64]
// ============================================================================

static B2D_INLINE int64_t i64nearby(float x) noexcept {
#if B2D_ARCH_X86 == 64
  return _mm_cvtss_si64(SIMD::vcvtf32f128(x));
#elif B2D_ARCH_X86 == 32 && B2D_CXX_MSC
  int64_t y;
  __asm {
    fld   dword ptr [x]
    fistp qword ptr [y]
  }
  return y;
#elif B2D_ARCH_X86 == 32 && B2D_CXX_GNU
  int64_t y;
  __asm__ __volatile__("flds %1\n" "fistpq %0\n" : "=m" (y) : "m" (x));
  return y;
#else
# error "[b2d] Math::i64nearby() - Unsupported architecture or compiler."
#endif
}

static B2D_INLINE int64_t i64nearby(double x) noexcept {
#if B2D_ARCH_X86 == 64
  return _mm_cvtsd_si64(_mm_set_sd(x));
#elif B2D_ARCH_X86 == 32 && B2D_CXX_MSC
  int64_t y;
  __asm {
    fld   qword ptr [x]
    fistp qword ptr [y]
  }
  return y;
#elif B2D_ARCH_X86 == 32 && B2D_CXX_GNU
  int64_t y;
  __asm__ __volatile__("fldl %1\n" "fistpq %0\n" : "=m" (y) : "m" (x));
  return y;
#else
# error "[b2d] Math::i64nearby() - Unsupported architecture or compiler."
#endif
}

static B2D_INLINE int64_t i64trunc(float x) noexcept { return int64_t(x); }
static B2D_INLINE int64_t i64trunc(double x) noexcept { return int64_t(x); }

static B2D_INLINE int64_t i64floor(float x) noexcept { int64_t y = i64trunc(x); return y - int64_t(float(y) > x); }
static B2D_INLINE int64_t i64floor(double x) noexcept { int64_t y = i64trunc(x); return y - int64_t(double(y) > x); }

static B2D_INLINE int64_t i64ceil(float x) noexcept { int64_t y = i64trunc(x); return y - int64_t(float(y) < x); }
static B2D_INLINE int64_t i64ceil(double x) noexcept { int64_t y = i64trunc(x); return y - int64_t(double(y) < x); }

static B2D_INLINE int64_t i64round(float x) noexcept { int64_t y = i64nearby(x); return y + int64_t(float(y) - x == -0.5f); }
static B2D_INLINE int64_t i64round(double x) noexcept { int64_t y = i64nearby(x); return y + int64_t(double(y) - x == -0.5);  }

static B2D_INLINE int64_t i64roundeven(float x) noexcept { return i64nearby(x); }
static B2D_INLINE int64_t i64roundeven(double x) noexcept { return i64nearby(x); }

// ============================================================================
// [b2d::Math - PowN]
// ============================================================================

constexpr float pow2(float x) noexcept { return x * x; }
constexpr double pow2(double x) noexcept { return x * x; }

constexpr float pow3(float x) noexcept { return x * x * x; }
constexpr double pow3(double x) noexcept { return x * x * x; }

// ============================================================================
// [b2d::Math - Fraction]
// ============================================================================

//! Get a fractional part of `x`.
//!
//! NOTE: Fractional part returned is always equal or greater than zero. The
//! implementation is compatible to many shader implementations defined as
//! `frac(x) == x - floor(x)`, which will return `0.25` for `-1.75`.
template<typename T>
B2D_INLINE T frac(T x) noexcept { return x - floor(x); }

//! Get a fractional part of `x` with sign preserved.
//!
//! NOTE: This is a complementary implementation of `frac(x)`, which does always
//! return a fractional part having the sign of the input. It will return `-0.75`
//! for input value of `-1.75`.
template<typename T>
B2D_INLINE T fracWithSign(T x) noexcept { return x - trunc(x); }

// ============================================================================
// [b2d::Math - Repeat]
// ============================================================================

//! Repeat the given value `x` in `y`, returning a value that is always equal
//! to or greater than zero and lesser than `y`. The return of `repeat(x, 1.0)`
//! should be identical to the return of `frac(x)`.
template<typename T>
B2D_INLINE T repeat(T x, T y) noexcept {
  T a = x;
  if (a >= y || a <= -y)
    a = std::fmod(a, y);
  if (a < T(0))
    a += y;
  return a;
}

// ============================================================================
// [b2d::Math - Lerp]
// ============================================================================

//! Linear interpolation of `a` and `b` at `t`.
//!
//! Returns `a * (1 - t) + b * t`.
//!
//! NOTE: This function should work with most geometric types Blend2D offers
//! that use double precision, however, it's not compatible with inregral types.
template<typename T>
constexpr T lerp(const T& a, const T& b, double t) noexcept { return (a * (1.0 - t)) + (b * t); }

// ============================================================================
// [b2d::Math - Dot / Cross / Triple]
// ============================================================================

// NOTE: There is no default implementation for those, any class that supports
// these operations must specialize `Math::dot<>` and `Math::cross<>` templates.

template<typename T>
constexpr double dot(const T& a, const T& b) noexcept = delete;

template<typename T>
constexpr double cross(const T& a, const T& b) noexcept = delete;

// ============================================================================
// [b2d::Math - UnitInterval]
// ============================================================================

template<typename T>
static B2D_INLINE T toUnitInterval(const T& x) noexcept { return bound<T>(x, T(0), T(1)); }

//! Check if `x` is a value in unit interval [0, 1].
template<typename T>
static B2D_INLINE bool isUnitInterval(const T& x) noexcept { return x >= T(0) && x <= T(1); }

// ============================================================================
// [b2d::Math - Length / LengthSq]
// ============================================================================

static B2D_INLINE double lengthSq(double vx, double vy) noexcept { return Math::pow2(vx) + Math::pow2(vy); }
static B2D_INLINE double lengthSq(double x0, double y0, double x1, double y1) noexcept { return lengthSq(x1 - x0, y1 - y0); }

static B2D_INLINE double length(double vx, double vy) noexcept { return std::sqrt(lengthSq(vx, vy)); }
static B2D_INLINE double length(double x0, double y0, double x1, double y1) noexcept { return length(x1 - x0, y1 - y0); }

// ============================================================================
// [b2d::Math - Unit Conversion]
// ============================================================================

template<typename T>
constexpr T deg2rad(T deg) noexcept { return deg * T(kDeg2Rad); }

template<typename T>
constexpr T rad2deg(T rad) noexcept { return rad * T(kRad2Deg); }

// ============================================================================
// [b2d::Math - Near]
// ============================================================================

template<typename T>
constexpr bool isNear(T x, T y, T eps = epsilon<T>()) noexcept { return std::abs(x - y) <= eps; }

template<typename T>
constexpr bool isNearZero(T x, T eps = epsilon<T>()) noexcept { return std::abs(x) <= eps; }

template<typename T>
constexpr bool isNearZeroPositive(T x, T eps = epsilon<T>()) noexcept { return x >= T(0) && x <= eps; }

template<typename T>
constexpr bool isNearOne(T x, T eps = epsilon<T>()) noexcept { return isNear(x, T(1), eps); }

} // Math namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_MATH_H
