// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_SIMD_X86_H
#define _B2D_CORE_SIMD_X86_H

// [Dependencies]
#include "../core/globals.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

//! SIMD namespace contains helper functions to access compiler intrinsics. The
//! names of these functions correspond to names of functions used by Pipe2D.

namespace SIMD {

// ============================================================================
// [b2d::SIMD - Core Definitions]
// ============================================================================

#if B2D_ARCH_AVX2
  #define B2D_SIMD_I 256
  #define B2D_SIMD_F 256
  #define B2D_SIMD_D 256
#elif B2D_ARCH_AVX
  #define B2D_SIMD_I 128
  #define B2D_SIMD_F 256
  #define B2D_SIMD_D 256
#elif B2D_ARCH_SSE2
  #define B2D_SIMD_I 128
  #define B2D_SIMD_F 128
  #define B2D_SIMD_D 128
#elif B2D_ARCH_SSE
  #define B2D_SIMD_I 0
  #define B2D_SIMD_F 128
  #define B2D_SIMD_D 0
#else
  #define B2D_SIMD_I 0
  #define B2D_SIMD_F 0
  #define B2D_SIMD_D 0
#endif

#if B2D_SIMD_I >= 128
typedef __m128i I128;
#endif

#if B2D_SIMD_F >= 128
typedef __m128 F128;
#endif

#if B2D_SIMD_D >= 128
typedef __m128d D128;
#endif

// Accessible through AVX as well (conversion and casting possible).
#if B2D_SIMD_I >= 256 || B2D_SIMD_F >= 256 || B2D_SIMD_D >= 256
typedef __m256i I256;
#endif

#if B2D_SIMD_F >= 256
typedef __m256 F256;
#endif

#if B2D_SIMD_D >= 256
typedef __m256d D256;
#endif

template<typename T>
union alignas(16) Const128 {
  T d[16 / sizeof(T)];

  #if B2D_SIMD_I >= 128
  I128 i128;
  #endif

  #if B2D_SIMD_F >= 128
  F128 f128;
  #endif

  #if B2D_SIMD_D >= 128
  D128 d128;
  #endif
};

template<typename T>
union alignas(32) Const256 {
  T d[32 / sizeof(T)];

  #if B2D_SIMD_I >= 256
  I256 i256;
  #endif

  #if B2D_SIMD_F >= 256
  F256 f256;
  #endif

  #if B2D_SIMD_D >= 256
  D256 d256;
  #endif
};

#define B2D_SIMD_DEF_I128_1xI8(NAME, X0) \
  static constexpr ::b2d::SIMD::Const128<int8_t> NAME = {{ \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0)  \
  }}

#define B2D_SIMD_DEF_I128_16xI8(NAME, X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15) \
  static constexpr ::b2d::SIMD::Const128<int8_t> NAME = {{ \
    int8_t(X0) , int8_t(X1) , int8_t(X2) , int8_t(X3) , \
    int8_t(X4) , int8_t(X5) , int8_t(X6) , int8_t(X7) , \
    int8_t(X8) , int8_t(X9) , int8_t(X10), int8_t(X11), \
    int8_t(X12), int8_t(X13), int8_t(X14), int8_t(X15)  \
  }}

#define B2D_SIMD_DEF_I256_1xI8(NAME, X0) \
  static constexpr ::b2d::SIMD::Const256<int8_t> NAME = {{ \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0), \
    int8_t(X0), int8_t(X0), int8_t(X0), int8_t(X0)  \
  }}

#define B2D_SIMD_DEF_I256_16xI8(NAME, X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15) \
  static constexpr ::b2d::SIMD::Const256<int8_t> NAME = {{ \
    int8_t(X0) , int8_t(X1) , int8_t(X2) , int8_t(X3) , \
    int8_t(X4) , int8_t(X5) , int8_t(X6) , int8_t(X7) , \
    int8_t(X8) , int8_t(X9) , int8_t(X10), int8_t(X11), \
    int8_t(X12), int8_t(X13), int8_t(X14), int8_t(X15), \
    int8_t(X0) , int8_t(X1) , int8_t(X2) , int8_t(X3) , \
    int8_t(X4) , int8_t(X5) , int8_t(X6) , int8_t(X7) , \
    int8_t(X8) , int8_t(X9) , int8_t(X10), int8_t(X11), \
    int8_t(X12), int8_t(X13), int8_t(X14), int8_t(X15)  \
  }

#define B2D_SIMD_DEF_I128_1xI32(NAME, X0) static constexpr ::b2d::SIMD::Const128<int32_t> NAME = {{ int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0) }}
#define B2D_SIMD_DEF_I256_1xI32(NAME, X0) static constexpr ::b2d::SIMD::Const256<int32_t> NAME = {{ int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0), int32_t(X0) }}

#define B2D_SIMD_DEF_F128_1xF32(NAME, X0) static constexpr ::b2d::SIMD::Const128<float  > NAME = {{ float(X0), float(X0), float(X0), float(X0) }}
#define B2D_SIMD_DEF_F256_1xF32(NAME, X0) static constexpr ::b2d::SIMD::Const256<float  > NAME = {{ float(X0), float(X0), float(X0), float(X0), float(X0), float(X0), float(X0), float(X0) }}

#define B2D_SIMD_DEF_I128_1xI64(NAME, X0) static constexpr ::b2d::SIMD::Const128<int64_t> NAME = {{ int64_t(X0), int64_t(X0) }}
#define B2D_SIMD_DEF_I256_1xI64(NAME, X0) static constexpr ::b2d::SIMD::Const256<int64_t> NAME = {{ int64_t(X0), int64_t(X0), int64_t(X0), int64_t(X0) }}

#define B2D_SIMD_DEF_D128_1xD64(NAME, X0) static constexpr ::b2d::SIMD::Const128<double > NAME = {{ double(X0), double(X0) }}
#define B2D_SIMD_DEF_D256_1xD64(NAME, X0) static constexpr ::b2d::SIMD::Const256<double > NAME = {{ double(X0), double(X0), double(X0), double(X0) }}

#define B2D_SIMD_DEF_I128_4xI32(NAME, X0, X1, X2, X3) static constexpr ::b2d::SIMD::Const128<int32_t> NAME = {{ int32_t(X0), int32_t(X1), int32_t(X2), int32_t(X3) }}
#define B2D_SIMD_DEF_I256_4xI32(NAME, X0, X1, X2, X3) static constexpr ::b2d::SIMD::Const256<int32_t> NAME = {{ int32_t(X0), int32_t(X1), int32_t(X2), int32_t(X3), int32_t(X0), int32_t(X1), int32_t(X2), int32_t(X3) }}

#define B2D_SIMD_DEF_F128_4xF32(NAME, X0, X1, X2, X3) static constexpr ::b2d::SIMD::Const128<float> NAME = {{ float(X0), float(X1), float(X2), float(X3) }}
#define B2D_SIMD_DEF_F256_4xF32(NAME, X0, X1, X2, X3) static constexpr ::b2d::SIMD::Const256<float> NAME = {{ float(X0), float(X1), float(X2), float(X3), float(X0), float(X1), float(X2), float(X3) }}

#define B2D_SIMD_DEF_I128_2xI64(NAME, X0, X1) static constexpr ::b2d::SIMD::Const128<int64_t> NAME = {{ int64_t(X0), int64_t(X1) }}
#define B2D_SIMD_DEF_I256_2xI64(NAME, X0, X1) static constexpr ::b2d::SIMD::Const256<int64_t> NAME = {{ int64_t(X0), int64_t(X1), int64_t(X0), int64_t(X1) }}

#define B2D_SIMD_DEF_D128_2xD64(NAME, X0, X1) static constexpr ::b2d::SIMD::Const128<double> NAME = {{ double(X0), double(X1) }}
#define B2D_SIMD_DEF_D256_2xD64(NAME, X0, X1) static constexpr ::b2d::SIMD::Const256<double> NAME = {{ double(X0), double(X1), double(X0), double(X1) }}

// Must be in anonymous namespace.
namespace {

// ============================================================================
// [b2d::SIMD - Cast]
// ============================================================================

template<typename DstT, typename SrcT>
B2D_INLINE DstT vcast(const SrcT& x) noexcept { return x; }

#if B2D_ARCH_SSE2
template<> B2D_INLINE F128 vcast(const I128& x) noexcept { return _mm_castsi128_ps(x); }
template<> B2D_INLINE D128 vcast(const I128& x) noexcept { return _mm_castsi128_pd(x); }
template<> B2D_INLINE I128 vcast(const F128& x) noexcept { return _mm_castps_si128(x); }
template<> B2D_INLINE D128 vcast(const F128& x) noexcept { return _mm_castps_pd(x); }
template<> B2D_INLINE I128 vcast(const D128& x) noexcept { return _mm_castpd_si128(x); }
template<> B2D_INLINE F128 vcast(const D128& x) noexcept { return _mm_castpd_ps(x); }
#endif

#if B2D_ARCH_AVX
template<> B2D_INLINE I128 vcast(const I256& x) noexcept { return _mm256_castsi256_si128(x); }
template<> B2D_INLINE I256 vcast(const I128& x) noexcept { return _mm256_castsi128_si256(x); }

template<> B2D_INLINE F128 vcast(const F256& x) noexcept { return _mm256_castps256_ps128(x); }
template<> B2D_INLINE F256 vcast(const F128& x) noexcept { return _mm256_castps128_ps256(x); }

template<> B2D_INLINE D128 vcast(const D256& x) noexcept { return _mm256_castpd256_pd128(x); }
template<> B2D_INLINE D256 vcast(const D128& x) noexcept { return _mm256_castpd128_pd256(x); }

template<> B2D_INLINE D256 vcast(const F256& x) noexcept { return _mm256_castps_pd(x); }
template<> B2D_INLINE F256 vcast(const D256& x) noexcept { return _mm256_castpd_ps(x); }

template<> B2D_INLINE F256 vcast(const I256& x) noexcept { return _mm256_castsi256_ps(x); }
template<> B2D_INLINE I256 vcast(const F256& x) noexcept { return _mm256_castps_si256(x); }

template<> B2D_INLINE D256 vcast(const I256& x) noexcept { return _mm256_castsi256_pd(x); }
template<> B2D_INLINE I256 vcast(const D256& x) noexcept { return _mm256_castpd_si256(x); }
#endif

// ============================================================================
// [b2d::SIMD - I128]
// ============================================================================

#if B2D_ARCH_SSE2
B2D_SIMD_DEF_I128_1xI32(u16_0080_128, 0x00800080);
B2D_SIMD_DEF_I128_1xI32(u16_0101_128, 0x01010101);

B2D_SIMD_DEF_I128_16xI8(pshufb_u32_to_u8_lo , 0, 4, 8,12, 0, 4, 8,12, 0, 4, 8,12, 0, 4, 8, 12);
B2D_SIMD_DEF_I128_16xI8(pshufb_u32_to_u16_lo, 0, 1, 4, 5, 8, 9,12,13, 0, 1, 4, 5, 8, 9,12, 13);

B2D_INLINE I128 vzeroi128() noexcept { return _mm_setzero_si128(); }

B2D_INLINE I128 vseti128i8(int8_t x) noexcept { return _mm_set1_epi8(x); }
B2D_INLINE I128 vseti128i16(int16_t x) noexcept { return _mm_set1_epi16(x); }
B2D_INLINE I128 vseti128i32(int32_t x) noexcept { return _mm_set1_epi32(x); }

B2D_INLINE I128 vseti128i32(int32_t x1, int32_t x0) noexcept { return _mm_set_epi32(x1, x0, x1, x0); }
B2D_INLINE I128 vseti128i32(int32_t x3, int32_t x2, int32_t x1, int32_t x0) noexcept { return _mm_set_epi32(x3, x2, x1, x0); }

B2D_INLINE I128 vseti128i64(int64_t x) noexcept {
#if B2D_ARCH_BITS >= 64
  return _mm_set1_epi64x(x);
#else
  return vseti128i32(int32_t(uint64_t(x) >> 32), int32_t(x & 0xFFFFFFFFu));
#endif
}

B2D_INLINE I128 vseti128i64(int64_t x1, int64_t x0) noexcept {
  return vseti128i32(int32_t(uint64_t(x1) >> 32), int32_t(x1 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x0) >> 32), int32_t(x0 & 0xFFFFFFFFu));
}

B2D_INLINE I128 vcvti32i128(int32_t x) noexcept { return _mm_cvtsi32_si128(int(x)); }
B2D_INLINE I128 vcvtu32i128(uint32_t x) noexcept { return _mm_cvtsi32_si128(int(x)); }

B2D_INLINE int32_t vcvti128i32(const I128& x) noexcept { return int32_t(_mm_cvtsi128_si32(x)); }
B2D_INLINE uint32_t vcvti128u32(const I128& x) noexcept { return uint32_t(_mm_cvtsi128_si32(x)); }

B2D_INLINE I128 vcvti64i128(int64_t x) noexcept {
#if B2D_ARCH_BITS >= 64
  return _mm_cvtsi64_si128(x);
#else
  return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&x));
#endif
}

B2D_INLINE int64_t vcvti128i64(const I128& x) noexcept {
#if B2D_ARCH_BITS >= 64
  return int64_t(_mm_cvtsi128_si64(x));
#else
  int64_t result;
  _mm_storel_epi64(reinterpret_cast<__m128i*>(&result), x);
  return result;
#endif
}

B2D_INLINE I128 vcvtu64i128(uint64_t x) noexcept { return vcvti64i128(int64_t(x)); }
B2D_INLINE uint64_t vcvti128u64(const I128& x) noexcept { return uint64_t(vcvti128i64(x)); }

template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I128 vswizli16(const I128& x) noexcept { return _mm_shufflelo_epi16(x, _MM_SHUFFLE(A, B, C, D)); }
template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I128 vswizhi16(const I128& x) noexcept { return _mm_shufflehi_epi16(x, _MM_SHUFFLE(A, B, C, D)); }

template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I128 vswizi16(const I128& x) noexcept { return vswizhi16<A, B, C, D>(vswizli16<A, B, C, D>(x)); }
template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I128 vswizi32(const I128& x) noexcept { return _mm_shuffle_epi32(x, _MM_SHUFFLE(A, B, C, D)); }
template<int A, int B>
B2D_INLINE I128 vswizi64(const I128& x) noexcept { return vswizi32<A*2 + 1, A*2, B*2 + 1, B*2>(x); }

#if B2D_ARCH_SSSE3
B2D_INLINE I128 vpshufb(const I128& x, const I128& y) noexcept { return _mm_shuffle_epi8(x, y); }

template<int N_BYTES>
B2D_INLINE I128 vpalignr(const I128& x, const I128& y) noexcept { return _mm_alignr_epi8(x, y, N_BYTES); }
#endif

B2D_INLINE I128 vswapi64(const I128& x) noexcept { return vswizi64<0, 1>(x); }
B2D_INLINE I128 vdupli64(const I128& x) noexcept { return vswizi64<0, 0>(x); }
B2D_INLINE I128 vduphi64(const I128& x) noexcept { return vswizi64<1, 1>(x); }

B2D_INLINE I128 vmovli64u8u16(const I128& x) noexcept {
#if B2D_ARCH_SSE4_1
  return _mm_cvtepu8_epi16(x);
#else
  return _mm_unpacklo_epi8(x, _mm_setzero_si128());
#endif
}

B2D_INLINE I128 vmovli64u16u32(const I128& x) noexcept {
#if B2D_ARCH_SSE4_1
  return _mm_cvtepu16_epi32(x);
#else
  return _mm_unpacklo_epi16(x, _mm_setzero_si128());
#endif
}

B2D_INLINE I128 vmovli64u32u64(const I128& x) noexcept {
#if B2D_ARCH_SSE4_1
  return _mm_cvtepu32_epi64(x);
#else
  return _mm_unpacklo_epi32(x, _mm_setzero_si128());
#endif
}

B2D_INLINE I128 vmovhi64u8u16(const I128& x) noexcept { return _mm_unpackhi_epi8(x, _mm_setzero_si128()); }
B2D_INLINE I128 vmovhi64u16u32(const I128& x) noexcept { return _mm_unpackhi_epi16(x, _mm_setzero_si128()); }
B2D_INLINE I128 vmovhi64u32u64(const I128& x) noexcept { return _mm_unpackhi_epi32(x, _mm_setzero_si128()); }

B2D_INLINE I128 vpacki16i8(const I128& x, const I128& y) noexcept { return _mm_packs_epi16(x, y); }
B2D_INLINE I128 vpacki16u8(const I128& x, const I128& y) noexcept { return _mm_packus_epi16(x, y); }
B2D_INLINE I128 vpacki32i16(const I128& x, const I128& y) noexcept { return _mm_packs_epi32(x, y); }

B2D_INLINE I128 vpacki16i8(const I128& x) noexcept { return vpacki16i8(x, x); }
B2D_INLINE I128 vpacki16u8(const I128& x) noexcept { return vpacki16u8(x, x); }
B2D_INLINE I128 vpacki32i16(const I128& x) noexcept { return vpacki32i16(x, x); }

B2D_INLINE I128 vpacki32u16(const I128& x, const I128& y) noexcept {
#if B2D_ARCH_SSE4_1
  return _mm_packus_epi32(x, y);
#else
  I128 xShifted = _mm_srai_epi32(_mm_slli_epi32(x, 16), 16);
  I128 yShifted = _mm_srai_epi32(_mm_slli_epi32(y, 16), 16);
  return _mm_packs_epi32(xShifted, yShifted);
#endif
}

B2D_INLINE I128 vpacki32u16(const I128& x) noexcept {
#if B2D_ARCH_SSE4_1
  return vpacki32u16(x, x);
#else
  I128 xShifted = _mm_srai_epi32(_mm_slli_epi32(x, 16), 16);
  return _mm_packs_epi32(xShifted, xShifted);
#endif
}

B2D_INLINE I128 vpacki32i8(const I128& x) noexcept { return vpacki16i8(vpacki32i16(x)); }
B2D_INLINE I128 vpacki32i8(const I128& x, const I128& y) noexcept { return vpacki16i8(vpacki32i16(x, y)); }
B2D_INLINE I128 vpacki32i8(const I128& x, const I128& y, const I128& z, const I128& w) noexcept { return vpacki16i8(vpacki32i16(x, y), vpacki32i16(z, w)); }

B2D_INLINE I128 vpacki32u8(const I128& x) noexcept { return vpacki16u8(vpacki32i16(x)); }
B2D_INLINE I128 vpacki32u8(const I128& x, const I128& y) noexcept { return vpacki16u8(vpacki32i16(x, y)); }
B2D_INLINE I128 vpacki32u8(const I128& x, const I128& y, const I128& z, const I128& w) noexcept { return vpacki16u8(vpacki32i16(x, y), vpacki32i16(z, w)); }

// These assume that HI bytes of all inputs are always zero, so the implementation
// can decide between packing with signed/unsigned saturation or vector swizzling.
B2D_INLINE I128 vpackzzwb(const I128& x) noexcept { return vpacki16u8(x); }
B2D_INLINE I128 vpackzzwb(const I128& x, const I128& y) noexcept { return vpacki16u8(x, y); }

B2D_INLINE I128 vpackzzdw(const I128& x) noexcept {
#if B2D_ARCH_SSE4_1 || !B2D_ARCH_SSSE3
  return vpacki32u16(x);
#else
  return vpshufb(x, pshufb_u32_to_u16_lo.i128);
#endif
}

B2D_INLINE I128 vpackzzdw(const I128& x, const I128& y) noexcept {
#if B2D_ARCH_SSE4_1 || !B2D_ARCH_SSSE3
  return vpacki32u16(x, y);
#else
  I128 xLo = vpshufb(x, pshufb_u32_to_u16_lo.i128);
  I128 yLo = vpshufb(y, pshufb_u32_to_u16_lo.i128);
  return _mm_unpacklo_epi64(xLo, yLo);
#endif
}

B2D_INLINE I128 vpackzzdb(const I128& x) noexcept {
#if B2D_ARCH_SSSE3
  return vpshufb(x, pshufb_u32_to_u8_lo.i128);
#else
  return vpacki16u8(vpacki32i16(x));
#endif
}

B2D_INLINE I128 vpackzzdb(const I128& x, const I128& y) noexcept { return vpacki16u8(vpacki32i16(x, y)); }
B2D_INLINE I128 vpackzzdb(const I128& x, const I128& y, const I128& z, const I128& w) noexcept { return vpacki16u8(vpacki32i16(x, y), vpacki32i16(z, w)); }

B2D_INLINE I128 vunpackli8(const I128& x, const I128& y) noexcept { return _mm_unpacklo_epi8(x, y); }
B2D_INLINE I128 vunpackhi8(const I128& x, const I128& y) noexcept { return _mm_unpackhi_epi8(x, y); }

B2D_INLINE I128 vunpackli16(const I128& x, const I128& y) noexcept { return _mm_unpacklo_epi16(x, y); }
B2D_INLINE I128 vunpackhi16(const I128& x, const I128& y) noexcept { return _mm_unpackhi_epi16(x, y); }

B2D_INLINE I128 vunpackli32(const I128& x, const I128& y) noexcept { return _mm_unpacklo_epi32(x, y); }
B2D_INLINE I128 vunpackhi32(const I128& x, const I128& y) noexcept { return _mm_unpackhi_epi32(x, y); }

B2D_INLINE I128 vunpackli64(const I128& x, const I128& y) noexcept { return _mm_unpacklo_epi64(x, y); }
B2D_INLINE I128 vunpackhi64(const I128& x, const I128& y) noexcept { return _mm_unpackhi_epi64(x, y); }

B2D_INLINE I128 vor(const I128& x, const I128& y) noexcept { return _mm_or_si128(x, y); }
B2D_INLINE I128 vxor(const I128& x, const I128& y) noexcept { return _mm_xor_si128(x, y); }
B2D_INLINE I128 vand(const I128& x, const I128& y) noexcept { return _mm_and_si128(x, y); }
B2D_INLINE I128 vandn(const I128& x, const I128& y) noexcept { return _mm_andnot_si128(y, x); }
B2D_INLINE I128 vnand(const I128& x, const I128& y) noexcept { return _mm_andnot_si128(x, y); }
B2D_INLINE I128 vblendmask(const I128& x, const I128& y, const I128& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }

//! Blend BITs or BYTEs, taking advantage of `pblendvb` (SSE4.1), if possible.
B2D_INLINE I128 vblendx(const I128& x, const I128& y, const I128& mask) noexcept {
#if B2D_ARCH_SSE4_1
  return _mm_blendv_epi8(x, y, mask);
#else
  return vblendmask(x, y, mask);
#endif
}

B2D_INLINE I128 vaddi8(const I128& x, const I128& y) noexcept { return _mm_add_epi8(x, y); }
B2D_INLINE I128 vaddi16(const I128& x, const I128& y) noexcept { return _mm_add_epi16(x, y); }
B2D_INLINE I128 vaddi32(const I128& x, const I128& y) noexcept { return _mm_add_epi32(x, y); }
B2D_INLINE I128 vaddi64(const I128& x, const I128& y) noexcept { return _mm_add_epi64(x, y); }

B2D_INLINE I128 vaddsi8(const I128& x, const I128& y) noexcept { return _mm_adds_epi8(x, y); }
B2D_INLINE I128 vaddsu8(const I128& x, const I128& y) noexcept { return _mm_adds_epu8(x, y); }
B2D_INLINE I128 vaddsi16(const I128& x, const I128& y) noexcept { return _mm_adds_epi16(x, y); }
B2D_INLINE I128 vaddsu16(const I128& x, const I128& y) noexcept { return _mm_adds_epu16(x, y); }

B2D_INLINE I128 vsubi8(const I128& x, const I128& y) noexcept { return _mm_sub_epi8(x, y); }
B2D_INLINE I128 vsubi16(const I128& x, const I128& y) noexcept { return _mm_sub_epi16(x, y); }
B2D_INLINE I128 vsubi32(const I128& x, const I128& y) noexcept { return _mm_sub_epi32(x, y); }
B2D_INLINE I128 vsubi64(const I128& x, const I128& y) noexcept { return _mm_sub_epi64(x, y); }

B2D_INLINE I128 vsubsi8(const I128& x, const I128& y) noexcept { return _mm_subs_epi8(x, y); }
B2D_INLINE I128 vsubsu8(const I128& x, const I128& y) noexcept { return _mm_subs_epu8(x, y); }
B2D_INLINE I128 vsubsi16(const I128& x, const I128& y) noexcept { return _mm_subs_epi16(x, y); }
B2D_INLINE I128 vsubsu16(const I128& x, const I128& y) noexcept { return _mm_subs_epu16(x, y); }

B2D_INLINE I128 vmuli16(const I128& x, const I128& y) noexcept { return _mm_mullo_epi16(x, y); }
B2D_INLINE I128 vmulu16(const I128& x, const I128& y) noexcept { return _mm_mullo_epi16(x, y); }
B2D_INLINE I128 vmulhi16(const I128& x, const I128& y) noexcept { return _mm_mulhi_epi16(x, y); }
B2D_INLINE I128 vmulhu16(const I128& x, const I128& y) noexcept { return _mm_mulhi_epu16(x, y); }

template<uint8_t N_BITS> B2D_INLINE I128 vslli16(const I128& x) noexcept { return _mm_slli_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I128 vslli32(const I128& x) noexcept { return _mm_slli_epi32(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I128 vslli64(const I128& x) noexcept { return _mm_slli_epi64(x, N_BITS); }

template<uint8_t N_BITS> B2D_INLINE I128 vsrli16(const I128& x) noexcept { return _mm_srli_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I128 vsrli32(const I128& x) noexcept { return _mm_srli_epi32(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I128 vsrli64(const I128& x) noexcept { return _mm_srli_epi64(x, N_BITS); }

template<uint8_t N_BITS> B2D_INLINE I128 vsrai16(const I128& x) noexcept { return _mm_srai_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I128 vsrai32(const I128& x) noexcept { return _mm_srai_epi32(x, N_BITS); }

template<uint8_t N_BYTES> B2D_INLINE I128 vslli128b(const I128& x) noexcept { return _mm_slli_si128(x, N_BYTES); }
template<uint8_t N_BYTES> B2D_INLINE I128 vsrli128b(const I128& x) noexcept { return _mm_srli_si128(x, N_BYTES); }

#if B2D_ARCH_SSE4_1
B2D_INLINE I128 vmini8(const I128& x, const I128& y) noexcept { return _mm_min_epi8(x, y); }
B2D_INLINE I128 vmaxi8(const I128& x, const I128& y) noexcept { return _mm_max_epi8(x, y); }
#else
B2D_INLINE I128 vmini8(const I128& x, const I128& y) noexcept { return vblendmask(y, x, _mm_cmpgt_epi8(x, y)); }
B2D_INLINE I128 vmaxi8(const I128& x, const I128& y) noexcept { return vblendmask(x, y, _mm_cmpgt_epi8(x, y)); }
#endif

B2D_INLINE I128 vminu8(const I128& x, const I128& y) noexcept { return _mm_min_epu8(x, y); }
B2D_INLINE I128 vmaxu8(const I128& x, const I128& y) noexcept { return _mm_max_epu8(x, y); }

B2D_INLINE I128 vmini16(const I128& x, const I128& y) noexcept { return _mm_min_epi16(x, y); }
B2D_INLINE I128 vmaxi16(const I128& x, const I128& y) noexcept { return _mm_max_epi16(x, y); }

#if B2D_ARCH_SSE4_1
B2D_INLINE I128 vminu16(const I128& x, const I128& y) noexcept { return _mm_min_epu16(x, y); }
B2D_INLINE I128 vmaxu16(const I128& x, const I128& y) noexcept { return _mm_max_epu16(x, y); }
#else
B2D_INLINE I128 vminu16(const I128& x, const I128& y) noexcept { return _mm_sub_epi16(x, _mm_subs_epu16(x, y)); }
B2D_INLINE I128 vmaxu16(const I128& x, const I128& y) noexcept { return _mm_add_epi16(x, _mm_subs_epu16(x, y)); }
#endif

#if B2D_ARCH_SSE4_1
B2D_INLINE I128 vmini32(const I128& x, const I128& y) noexcept { return _mm_min_epi32(x, y); }
B2D_INLINE I128 vmaxi32(const I128& x, const I128& y) noexcept { return _mm_max_epi32(x, y); }
#else
B2D_INLINE I128 vmini32(const I128& x, const I128& y) noexcept { return vblendmask(y, x, _mm_cmpgt_epi32(x, y)); }
B2D_INLINE I128 vmaxi32(const I128& x, const I128& y) noexcept { return vblendmask(x, y, _mm_cmpgt_epi32(x, y)); }
#endif

B2D_INLINE I128 vcmpeqi8(const I128& x, const I128& y) noexcept { return _mm_cmpeq_epi8(x, y); }
B2D_INLINE I128 vcmpgti8(const I128& x, const I128& y) noexcept { return _mm_cmpgt_epi8(x, y); }

B2D_INLINE I128 vcmpeqi16(const I128& x, const I128& y) noexcept { return _mm_cmpeq_epi16(x, y); }
B2D_INLINE I128 vcmpgti16(const I128& x, const I128& y) noexcept { return _mm_cmpgt_epi16(x, y); }

B2D_INLINE I128 vcmpeqi32(const I128& x, const I128& y) noexcept { return _mm_cmpeq_epi32(x, y); }
B2D_INLINE I128 vcmpgti32(const I128& x, const I128& y) noexcept { return _mm_cmpgt_epi32(x, y); }

#if B2D_ARCH_SSSE3
B2D_INLINE I128 vabsi8(const I128& x) noexcept { return _mm_abs_epi8(x); }
B2D_INLINE I128 vabsi16(const I128& x) noexcept { return _mm_abs_epi16(x); }
B2D_INLINE I128 vabsi32(const I128& x) noexcept { return _mm_abs_epi32(x); }
#else
B2D_INLINE I128 vabsi8(const I128& x) noexcept { return vminu8(vsubi8(vzeroi128(), x), x); }
B2D_INLINE I128 vabsi16(const I128& x) noexcept { return vmaxi16(vsubi16(vzeroi128(), x), x); }
B2D_INLINE I128 vabsi32(const I128& x) noexcept { I128 y = vsrai32<31>(x); return vsubi32(vxor(x, y), y); }
#endif

B2D_INLINE I128 vloadi128_32(const void* p) noexcept { return _mm_cvtsi32_si128(Support::readI32u(p)); }
B2D_INLINE I128 vloadi128_64(const void* p) noexcept { return _mm_loadl_epi64(static_cast<const I128*>(p)); }
B2D_INLINE I128 vloadi128a(const void* p) noexcept { return _mm_load_si128(static_cast<const I128*>(p)); }
B2D_INLINE I128 vloadi128u(const void* p) noexcept { return _mm_loadu_si128(static_cast<const I128*>(p)); }

B2D_INLINE I128 vloadi128_l64(const I128& x, const void* p) noexcept { return vcast<I128>(_mm_loadl_pd(vcast<D128>(x), static_cast<const double*>(p))); }
B2D_INLINE I128 vloadi128_h64(const I128& x, const void* p) noexcept { return vcast<I128>(_mm_loadh_pd(vcast<D128>(x), static_cast<const double*>(p))); }

B2D_INLINE void vstorei32(void* p, const I128& x) noexcept { static_cast<int*>(p)[0] = _mm_cvtsi128_si32(x); }
B2D_INLINE void vstorei64(void* p, const I128& x) noexcept { _mm_storel_epi64(static_cast<I128*>(p), x); }
B2D_INLINE void vstorei128a(void* p, const I128& x) noexcept { _mm_store_si128(static_cast<I128*>(p), x); }
B2D_INLINE void vstorei128u(void* p, const I128& x) noexcept { _mm_storeu_si128(static_cast<I128*>(p), x); }

B2D_INLINE void vstoreli64(void* p, const I128& x) noexcept { _mm_storel_epi64(static_cast<I128*>(p), x); }
B2D_INLINE void vstorehi64(void* p, const I128& x) noexcept { _mm_storeh_pd(static_cast<double*>(p), vcast<D128>(x)); }
#endif

B2D_INLINE bool vhasmaski8(const I128& x, int bits0_15) noexcept { return _mm_movemask_epi8(vcast<I128>(x)) == bits0_15; }
B2D_INLINE bool vhasmaski8(const F128& x, int bits0_15) noexcept { return _mm_movemask_epi8(vcast<I128>(x)) == bits0_15; }
B2D_INLINE bool vhasmaski8(const D128& x, int bits0_15) noexcept { return _mm_movemask_epi8(vcast<I128>(x)) == bits0_15; }

B2D_INLINE bool vhasmaski32(const I128& x, int bits0_3) noexcept { return _mm_movemask_ps(vcast<F128>(x)) == bits0_3; }
B2D_INLINE bool vhasmaski64(const I128& x, int bits0_1) noexcept { return _mm_movemask_pd(vcast<D128>(x)) == bits0_1; }

B2D_INLINE I128 vdiv255u16(const I128& x) noexcept { return vmulhu16(vaddi16(x, u16_0080_128.i128), u16_0101_128.i128); }
#endif

// ============================================================================
// [b2d::SIMD - F128]
// ============================================================================

#if B2D_ARCH_SSE
B2D_INLINE F128 vzerof128() noexcept { return _mm_setzero_ps(); }

B2D_INLINE F128 vsetf128(float x) noexcept { return _mm_set1_ps(x); }
B2D_INLINE F128 vsetf128(float x3, float x2, float x1, float x0) noexcept { return _mm_set_ps(x3, x2, x1, x0); }

//! Cast a scalar `float` to `F128` vector type.
B2D_INLINE F128 vcvtf32f128(float x) noexcept {
  #if B2D_CXX_GNU_ONLY
  // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70708
  F128 reg;
  __asm__("" : "=x" (reg) : "0" (x));
  return reg;
  #elif B2D_CXX_MSC_ONLY
  // MSC generates so much code when it sees `_mm_set_ss()`, this is a better alternative.
  return _mm_setr_ps(x, 0, 0, 0);
  #else
  return _mm_set_ss(x);
  #endif
}
B2D_INLINE float vcvtf128f32(const F128& x) noexcept { return _mm_cvtss_f32(x); }

B2D_INLINE F128 vcvti32f128(int32_t x) noexcept { return _mm_cvtsi32_ss(vzerof128(), x); }
B2D_INLINE int32_t vcvtf128i32(const F128& x) noexcept { return _mm_cvtss_si32(x); }
B2D_INLINE int32_t vcvttf128i32(const F128& x) noexcept { return _mm_cvttss_si32(x); }

#if B2D_ARCH_BITS >= 64
B2D_INLINE F128 vcvti64f128(int64_t x) noexcept { return _mm_cvtsi64_ss(vzerof128(), x); }
B2D_INLINE int64_t vcvtf128i64(const F128& x) noexcept { return _mm_cvtss_si64(x); }
B2D_INLINE int64_t vcvttf128i64(const F128& x) noexcept { return _mm_cvttss_si64(x); }
#endif

template<int A, int B, int C, int D>
B2D_INLINE F128 vshuff32(const F128& x, const F128& y) noexcept { return _mm_shuffle_ps(x, y, _MM_SHUFFLE(A, B, C, D)); }

template<int A, int B, int C, int D>
B2D_INLINE F128 vswizf32(const F128& x) noexcept {
  #if B2D_ARCH_SSE2 && !B2D_ARCH_AVX
  return vcast<F128>(vswizi32<A, B, C, D>(vcast<I128>(x)));
  #else
  return vshuff32<A, B, C, D>(x, x);
  #endif
}

template<int A, int B>
B2D_INLINE F128 vswizf64(const F128& x) noexcept {
  #if B2D_ARCH_SSE2 && !B2D_ARCH_AVX
  return vcast<F128>(vswizi64<A, B>(vcast<I128>(x)));
  #else
  return vswizf32<A*2 + 1, A*2, B*2 + 1, B*2>(x);
  #endif
}

B2D_INLINE F128 vduplf32(const F128& x) noexcept { return vswizf32<2, 2, 0, 0>(x); }
B2D_INLINE F128 vduphf32(const F128& x) noexcept { return vswizf32<3, 3, 1, 1>(x); }

B2D_INLINE F128 vswapf64(const F128& x) noexcept { return vswizf64<0, 1>(x); }
B2D_INLINE F128 vduplf64(const F128& x) noexcept { return vswizf64<0, 0>(x); }
B2D_INLINE F128 vduphf64(const F128& x) noexcept { return vswizf64<1, 1>(x); }

B2D_INLINE F128 vunpacklf32(const F128& x, const F128& y) noexcept { return _mm_unpacklo_ps(x, y); }
B2D_INLINE F128 vunpackhf32(const F128& x, const F128& y) noexcept { return _mm_unpackhi_ps(x, y); }

B2D_INLINE F128 vor(const F128& x, const F128& y) noexcept { return _mm_or_ps(x, y); }
B2D_INLINE F128 vxor(const F128& x, const F128& y) noexcept { return _mm_xor_ps(x, y); }
B2D_INLINE F128 vand(const F128& x, const F128& y) noexcept { return _mm_and_ps(x, y); }
B2D_INLINE F128 vandn(const F128& x, const F128& y) noexcept { return _mm_andnot_ps(y, x); }
B2D_INLINE F128 vnand(const F128& x, const F128& y) noexcept { return _mm_andnot_ps(x, y); }
B2D_INLINE F128 vblendmask(const F128& x, const F128& y, const F128& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }

B2D_INLINE F128 vaddss(const F128& x, const F128& y) noexcept { return _mm_add_ss(x, y); }
B2D_INLINE F128 vaddps(const F128& x, const F128& y) noexcept { return _mm_add_ps(x, y); }

B2D_INLINE F128 vsubss(const F128& x, const F128& y) noexcept { return _mm_sub_ss(x, y); }
B2D_INLINE F128 vsubps(const F128& x, const F128& y) noexcept { return _mm_sub_ps(x, y); }

B2D_INLINE F128 vmulss(const F128& x, const F128& y) noexcept { return _mm_mul_ss(x, y); }
B2D_INLINE F128 vmulps(const F128& x, const F128& y) noexcept { return _mm_mul_ps(x, y); }

B2D_INLINE F128 vdivss(const F128& x, const F128& y) noexcept { return _mm_div_ss(x, y); }
B2D_INLINE F128 vdivps(const F128& x, const F128& y) noexcept { return _mm_div_ps(x, y); }

B2D_INLINE F128 vminss(const F128& x, const F128& y) noexcept { return _mm_min_ss(x, y); }
B2D_INLINE F128 vminps(const F128& x, const F128& y) noexcept { return _mm_min_ps(x, y); }

B2D_INLINE F128 vmaxss(const F128& x, const F128& y) noexcept { return _mm_max_ss(x, y); }
B2D_INLINE F128 vmaxps(const F128& x, const F128& y) noexcept { return _mm_max_ps(x, y); }

B2D_INLINE F128 vcmpeqss(const F128& x, const F128& y) noexcept { return _mm_cmpeq_ss(x, y); }
B2D_INLINE F128 vcmpeqps(const F128& x, const F128& y) noexcept { return _mm_cmpeq_ps(x, y); }

B2D_INLINE F128 vcmpness(const F128& x, const F128& y) noexcept { return _mm_cmpneq_ss(x, y); }
B2D_INLINE F128 vcmpneps(const F128& x, const F128& y) noexcept { return _mm_cmpneq_ps(x, y); }

B2D_INLINE F128 vcmpgess(const F128& x, const F128& y) noexcept { return _mm_cmpge_ss(x, y); }
B2D_INLINE F128 vcmpgeps(const F128& x, const F128& y) noexcept { return _mm_cmpge_ps(x, y); }

B2D_INLINE F128 vcmpgtss(const F128& x, const F128& y) noexcept { return _mm_cmpgt_ss(x, y); }
B2D_INLINE F128 vcmpgtps(const F128& x, const F128& y) noexcept { return _mm_cmpgt_ps(x, y); }

B2D_INLINE F128 vcmpless(const F128& x, const F128& y) noexcept { return _mm_cmple_ss(x, y); }
B2D_INLINE F128 vcmpleps(const F128& x, const F128& y) noexcept { return _mm_cmple_ps(x, y); }

B2D_INLINE F128 vcmpltss(const F128& x, const F128& y) noexcept { return _mm_cmplt_ss(x, y); }
B2D_INLINE F128 vcmpltps(const F128& x, const F128& y) noexcept { return _mm_cmplt_ps(x, y); }

B2D_INLINE F128 vsqrtss(const F128& x) noexcept { return _mm_sqrt_ss(x); }
B2D_INLINE F128 vsqrtps(const F128& x) noexcept { return _mm_sqrt_ps(x); }

B2D_INLINE F128 vloadf128_32(const void* p) noexcept { return _mm_load_ss(static_cast<const float*>(p)); }
B2D_INLINE F128 vloadf128_64(const void* p) noexcept { return vcast<F128>(vloadi128_64(p)); }

B2D_INLINE F128 vloadf128a(const void* p) noexcept { return _mm_load_ps(static_cast<const float*>(p)); }
B2D_INLINE F128 vloadf128u(const void* p) noexcept { return _mm_loadu_ps(static_cast<const float*>(p)); }

B2D_INLINE F128 vloadf128_l64(const F128& x, const void* p) noexcept { return _mm_loadl_pi(x, static_cast<const __m64*>(p)); }
B2D_INLINE F128 vloadf128_h64(const F128& x, const void* p) noexcept { return _mm_loadh_pi(x, static_cast<const __m64*>(p)); }

B2D_INLINE void vstoref32(void* p, const F128& x) noexcept { _mm_store_ss(static_cast<float*>(p), x); }
B2D_INLINE void vstoref64(void* p, const F128& x) noexcept { _mm_storel_pi(static_cast<__m64*>(p), x); }
B2D_INLINE void vstorelf64(void* p, const F128& x) noexcept { _mm_storel_pi(static_cast<__m64*>(p), x); }
B2D_INLINE void vstorehf64(void* p, const F128& x) noexcept { _mm_storeh_pi(static_cast<__m64*>(p), x); }
B2D_INLINE void vstoref128a(void* p, const F128& x) noexcept { _mm_store_ps(static_cast<float*>(p), x); }
B2D_INLINE void vstoref128u(void* p, const F128& x) noexcept { _mm_storeu_ps(static_cast<float*>(p), x); }

B2D_INLINE F128 vbroadcastf128_64(const void* p) noexcept {
  #if B2D_ARCH_SSE3
  return vcast<F128>(_mm_loaddup_pd(static_cast<const double*>(p)));
  #else
  return vduplf64(vloadf128_64(p));
  #endif
}

B2D_INLINE bool vhasmaskf32(const F128& x, int bits0_3) noexcept { return _mm_movemask_ps(vcast<F128>(x)) == bits0_3; }
B2D_INLINE bool vhasmaskf64(const F128& x, int bits0_1) noexcept { return _mm_movemask_pd(vcast<D128>(x)) == bits0_1; }

// ============================================================================
// [b2d::SIMD - D128]
// ============================================================================

#if B2D_ARCH_SSE2
B2D_INLINE D128 vzerod128() noexcept { return _mm_setzero_pd(); }

B2D_INLINE D128 vsetd128(double x) noexcept { return _mm_set1_pd(x); }
B2D_INLINE D128 vsetd128(double x1, double x0) noexcept { return _mm_set_pd(x1, x0); }

//! Cast a scalar `double` to `D128` vector type.
B2D_INLINE D128 vcvtd64d128(double x) noexcept {
  #if B2D_CXX_GNU_ONLY
  // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70708
  D128 reg;
  __asm__("" : "=x" (reg) : "0" (x));
  return reg;
  #elif B2D_CXX_MSC_ONLY
  // MSC generates so much code when it sees `_mm_set_sd()`, this is a better alternative.
  return _mm_setr_pd(x, 0);
  #else
  return _mm_set_sd(x);
  #endif
}
B2D_INLINE double vcvtd128d64(const D128& x) noexcept { return _mm_cvtsd_f64(x); }

B2D_INLINE D128 vcvti32d128(int32_t x) noexcept { return _mm_cvtsi32_sd(vzerod128(), x); }
B2D_INLINE int32_t vcvtd128i32(const D128& x) noexcept { return _mm_cvtsd_si32(x); }
B2D_INLINE int32_t vcvttd128i32(const D128& x) noexcept { return _mm_cvttsd_si32(x); }

#if B2D_ARCH_BITS >= 64
B2D_INLINE D128 vcvti64d128(int64_t x) noexcept { return _mm_cvtsi64_sd(vzerod128(), x); }
B2D_INLINE int64_t vcvtd128i64(const D128& x) noexcept { return _mm_cvtsd_si64(x); }
B2D_INLINE int64_t vcvttd128i64(const D128& x) noexcept { return _mm_cvttsd_si64(x); }
#endif

B2D_INLINE D128 vcvtf128d128(const F128& x) noexcept { return _mm_cvtps_pd(x); }
B2D_INLINE F128 vcvtd128f128(const D128& x) noexcept { return _mm_cvtpd_ps(x); }

B2D_INLINE F128 vcvti128f128(const I128& x) noexcept { return _mm_cvtepi32_ps(x); }
B2D_INLINE D128 vcvti128d128(const I128& x) noexcept { return _mm_cvtepi32_pd(x); }

B2D_INLINE I128 vcvtf128i128(const F128& x) noexcept { return _mm_cvtps_epi32(x); }
B2D_INLINE I128 vcvttf128i128(const F128& x) noexcept { return _mm_cvttps_epi32(x); }

B2D_INLINE I128 vcvtd128i128(const D128& x) noexcept { return _mm_cvtpd_epi32(x); }
B2D_INLINE I128 vcvttd128i128(const D128& x) noexcept { return _mm_cvttpd_epi32(x); }

template<int A, int B>
B2D_INLINE D128 vshufd64(const D128& x, const D128& y) noexcept { return _mm_shuffle_pd(x, y, (A << 1) | B); }

template<int A, int B>
B2D_INLINE D128 vswizd64(const D128& x) noexcept {
  #if !B2D_ARCH_AVX
  return vcast<D128>(vswizi64<A, B>(vcast<I128>(x)));
  #else
  return vshufd64<A, B>(x, x);
  #endif
}

B2D_INLINE D128 vswapd64(const D128& x) noexcept { return vswizd64<0, 1>(x); }
B2D_INLINE D128 vdupld64(const D128& x) noexcept { return vswizd64<0, 0>(x); }
B2D_INLINE D128 vduphd64(const D128& x) noexcept { return vswizd64<1, 1>(x); }

B2D_INLINE D128 vunpackld64(const D128& x, const D128& y) noexcept { return _mm_unpacklo_pd(x, y); }
B2D_INLINE D128 vunpackhd64(const D128& x, const D128& y) noexcept { return _mm_unpackhi_pd(x, y); }

B2D_INLINE D128 vor(const D128& x, const D128& y) noexcept { return _mm_or_pd(x, y); }
B2D_INLINE D128 vxor(const D128& x, const D128& y) noexcept { return _mm_xor_pd(x, y); }
B2D_INLINE D128 vand(const D128& x, const D128& y) noexcept { return _mm_and_pd(x, y); }
B2D_INLINE D128 vandn(const D128& x, const D128& y) noexcept { return _mm_andnot_pd(y, x); }
B2D_INLINE D128 vnand(const D128& x, const D128& y) noexcept { return _mm_andnot_pd(x, y); }
B2D_INLINE D128 vblendmask(const D128& x, const D128& y, const D128& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }

B2D_INLINE D128 vaddsd(const D128& x, const D128& y) noexcept { return _mm_add_sd(x, y); }
B2D_INLINE D128 vaddpd(const D128& x, const D128& y) noexcept { return _mm_add_pd(x, y); }

B2D_INLINE D128 vsubsd(const D128& x, const D128& y) noexcept { return _mm_sub_sd(x, y); }
B2D_INLINE D128 vsubpd(const D128& x, const D128& y) noexcept { return _mm_sub_pd(x, y); }

B2D_INLINE D128 vmulsd(const D128& x, const D128& y) noexcept { return _mm_mul_sd(x, y); }
B2D_INLINE D128 vmulpd(const D128& x, const D128& y) noexcept { return _mm_mul_pd(x, y); }

B2D_INLINE D128 vdivsd(const D128& x, const D128& y) noexcept { return _mm_div_sd(x, y); }
B2D_INLINE D128 vdivpd(const D128& x, const D128& y) noexcept { return _mm_div_pd(x, y); }

B2D_INLINE D128 vminsd(const D128& x, const D128& y) noexcept { return _mm_min_sd(x, y); }
B2D_INLINE D128 vminpd(const D128& x, const D128& y) noexcept { return _mm_min_pd(x, y); }

B2D_INLINE D128 vmaxsd(const D128& x, const D128& y) noexcept { return _mm_max_sd(x, y); }
B2D_INLINE D128 vmaxpd(const D128& x, const D128& y) noexcept { return _mm_max_pd(x, y); }

B2D_INLINE D128 vcmpeqsd(const D128& x, const D128& y) noexcept { return _mm_cmpeq_sd(x, y); }
B2D_INLINE D128 vcmpeqpd(const D128& x, const D128& y) noexcept { return _mm_cmpeq_pd(x, y); }

B2D_INLINE D128 vcmpnesd(const D128& x, const D128& y) noexcept { return _mm_cmpneq_sd(x, y); }
B2D_INLINE D128 vcmpnepd(const D128& x, const D128& y) noexcept { return _mm_cmpneq_pd(x, y); }

B2D_INLINE D128 vcmpgesd(const D128& x, const D128& y) noexcept { return _mm_cmpge_sd(x, y); }
B2D_INLINE D128 vcmpgepd(const D128& x, const D128& y) noexcept { return _mm_cmpge_pd(x, y); }

B2D_INLINE D128 vcmpgtsd(const D128& x, const D128& y) noexcept { return _mm_cmpgt_sd(x, y); }
B2D_INLINE D128 vcmpgtpd(const D128& x, const D128& y) noexcept { return _mm_cmpgt_pd(x, y); }

B2D_INLINE D128 vcmplesd(const D128& x, const D128& y) noexcept { return _mm_cmple_sd(x, y); }
B2D_INLINE D128 vcmplepd(const D128& x, const D128& y) noexcept { return _mm_cmple_pd(x, y); }

B2D_INLINE D128 vcmpltsd(const D128& x, const D128& y) noexcept { return _mm_cmplt_sd(x, y); }
B2D_INLINE D128 vcmpltpd(const D128& x, const D128& y) noexcept { return _mm_cmplt_pd(x, y); }

B2D_INLINE D128 vsqrtsd(const D128& x) noexcept { return _mm_sqrt_sd(x, x); }
B2D_INLINE D128 vsqrtpd(const D128& x) noexcept { return _mm_sqrt_pd(x); }

B2D_INLINE D128 vloadd128_64(const void* p) noexcept { return _mm_load_sd(static_cast<const double*>(p)); }
B2D_INLINE D128 vloadd128a(const void* p) noexcept { return _mm_load_pd(static_cast<const double*>(p)); }
B2D_INLINE D128 vloadd128u(const void* p) noexcept { return _mm_loadu_pd(static_cast<const double*>(p)); }

B2D_INLINE D128 vloadd128_l64(const D128& x, const void* p) noexcept { return _mm_loadl_pd(x, static_cast<const double*>(p)); }
B2D_INLINE D128 vloadd128_h64(const D128& x, const void* p) noexcept { return _mm_loadh_pd(x, static_cast<const double*>(p)); }

B2D_INLINE D128 vbroadcastd128_64(const void* p) noexcept {
  #if B2D_ARCH_SSE3
  return _mm_loaddup_pd(static_cast<const double*>(p));
  #else
  return vdupld64(vloadd128_64(p));
  #endif
}

B2D_INLINE void vstored64(void* p, const D128& x) noexcept { _mm_store_sd(static_cast<double*>(p), x); }
B2D_INLINE void vstoreld64(void* p, const D128& x) noexcept { _mm_storel_pd(static_cast<double*>(p), x); }
B2D_INLINE void vstorehd64(void* p, const D128& x) noexcept { _mm_storeh_pd(static_cast<double*>(p), x); }
B2D_INLINE void vstored128a(void* p, const D128& x) noexcept { _mm_store_pd(static_cast<double*>(p), x); }
B2D_INLINE void vstored128u(void* p, const D128& x) noexcept { _mm_storeu_pd(static_cast<double*>(p), x); }

B2D_INLINE bool vhasmaskd64(const D128& x, int bits0_1) noexcept { return _mm_movemask_pd(vcast<D128>(x)) == bits0_1; }
#endif

// ============================================================================
// [b2d::SIMD::I256]
// ============================================================================

#if B2D_ARCH_AVX
B2D_INLINE I256 vzeroi256() noexcept { return _mm256_setzero_si256(); }

B2D_INLINE F256 vcvti256f256(const I256& x) noexcept { return _mm256_cvtepi32_ps(x); }
B2D_INLINE D256 vcvti128d256(const I128& x) noexcept { return _mm256_cvtepi32_pd(vcast<I128>(x)); }
B2D_INLINE D256 vcvti256d256(const I256& x) noexcept { return _mm256_cvtepi32_pd(vcast<I128>(x)); }
#endif

#if B2D_ARCH_AVX2
B2D_SIMD_DEF_I256_1xI32(u16_0080_256, 0x00800080);
B2D_SIMD_DEF_I256_1xI32(u16_0101_256, 0x01010101);

B2D_INLINE I256 vseti256i8(int8_t x) noexcept { return _mm256_set1_epi8(x); }
B2D_INLINE I256 vseti256i16(int16_t x) noexcept { return _mm256_set1_epi16(x); }

B2D_INLINE I256 vseti256i32(int32_t x) noexcept { return _mm256_set1_epi32(x); }
B2D_INLINE I256 vseti256i32(int32_t x1, int32_t x0) noexcept { return _mm256_set_epi32(x1, x0, x1, x0, x1, x0, x1, x0); }
B2D_INLINE I256 vseti256i32(int32_t x3, int32_t x2, int32_t x1, int32_t x0) noexcept { return _mm256_set_epi32(x3, x2, x1, x0, x3, x2, x1, x0); }
B2D_INLINE I256 vseti256i32(int32_t x7, int32_t x6, int32_t x5, int32_t x4, int32_t x3, int32_t x2, int32_t x1, int32_t x0) noexcept { return _mm256_set_epi32(x7, x6, x5, x4, x3, x2, x1, x0); }

B2D_INLINE I256 vseti256i64(int64_t x) noexcept {
  #if B2D_ARCH_BITS >= 64
  return _mm256_set1_epi64x(x);
  #else
  return vseti256i32(int32_t(uint64_t(x) >> 32), int32_t(x & 0xFFFFFFFFu));
  #endif
}

B2D_INLINE I256 vseti256i64(int64_t x1, int64_t x0) noexcept {
  return vseti256i32(int32_t(uint64_t(x1) >> 32), int32_t(x1 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x0) >> 32), int32_t(x0 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x1) >> 32), int32_t(x1 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x0) >> 32), int32_t(x0 & 0xFFFFFFFFu));
}

B2D_INLINE I256 vseti256i64(int64_t x3, int64_t x2, int64_t x1, int64_t x0) noexcept {
  return vseti256i32(int32_t(uint64_t(x3) >> 32), int32_t(x3 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x2) >> 32), int32_t(x2 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x1) >> 32), int32_t(x1 & 0xFFFFFFFFu),
                     int32_t(uint64_t(x0) >> 32), int32_t(x0 & 0xFFFFFFFFu));
}

B2D_INLINE I256 vcvti32i256(int32_t x) noexcept { return vcast<I256>(vcvti32i128(x)); }
B2D_INLINE I256 vcvtu32i256(uint32_t x) noexcept { return vcast<I256>(vcvtu32i128(x)); }

B2D_INLINE int32_t vcvti256i32(const I256& x) noexcept { return vcvti128i32(vcast<I128>(x)); }
B2D_INLINE uint32_t vcvti256u32(const I256& x) noexcept { return vcvti128u32(vcast<I128>(x)); }

B2D_INLINE I256 vcvti64i256(int64_t x) noexcept { return vcast<I256>(vcvti64i128(x)); }
B2D_INLINE I256 vcvtu64i256(uint64_t x) noexcept { return vcast<I256>(vcvtu64i128(x)); }

B2D_INLINE int64_t vcvti256i64(const I256& x) noexcept { return vcvti128i64(vcast<I128>(x)); }
B2D_INLINE uint64_t vcvti256u64(const I256& x) noexcept { return vcvti128u64(vcast<I128>(x)); }

template<int A, int B>
B2D_INLINE I256 vpermi128(const I256& x, const I256& y) noexcept { return _mm256_permute2x128_si256(x, y, ((A & 0xF) << 4) + (B & 0xF)); }
template<int A, int B>
B2D_INLINE I256 vpermi128(const I256& x) noexcept { return vpermi128<A, B>(x, x); }

template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I256 vswizli16(const I256& x) noexcept { return _mm256_shufflelo_epi16(x, _MM_SHUFFLE(A, B, C, D)); }
template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I256 vswizhi16(const I256& x) noexcept { return _mm256_shufflehi_epi16(x, _MM_SHUFFLE(A, B, C, D)); }

template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I256 vswizi16(const I256& x) noexcept { return vswizhi16<A, B, C, D>(vswizli16<A, B, C, D>(x)); }
template<uint8_t A, uint8_t B, uint8_t C, uint8_t D>
B2D_INLINE I256 vswizi32(const I256& x) noexcept { return _mm256_shuffle_epi32(x, _MM_SHUFFLE(A, B, C, D)); }
template<int A, int B>
B2D_INLINE I256 vswizi64(const I256& x) noexcept { return vswizi32<A*2 + 1, A*2, B*2 + 1, B*2>(x); }

B2D_INLINE I256 vpshufb(const I256& x, const I256& y) noexcept { return _mm256_shuffle_epi8(x, y); }

template<int N_BYTES>
B2D_INLINE I256 vpalignr(const I256& x, const I256& y) noexcept { return _mm256_alignr_epi8(x, y, N_BYTES); }

B2D_INLINE I256 vsplati8i256(const I128& x) noexcept { return _mm256_broadcastb_epi8(vcast<I128>(x)); }
B2D_INLINE I256 vsplati8i256(const I256& x) noexcept { return _mm256_broadcastb_epi8(vcast<I128>(x)); }

B2D_INLINE I256 vsplati16i256(const I128& x) noexcept { return _mm256_broadcastw_epi16(vcast<I128>(x)); }
B2D_INLINE I256 vsplati16i256(const I256& x) noexcept { return _mm256_broadcastw_epi16(vcast<I128>(x)); }

B2D_INLINE I256 vsplati32i256(const I128& x) noexcept { return _mm256_broadcastd_epi32(vcast<I128>(x)); }
B2D_INLINE I256 vsplati32i256(const I256& x) noexcept { return _mm256_broadcastd_epi32(vcast<I128>(x)); }

B2D_INLINE I256 vsplati64i256(const I128& x) noexcept { return _mm256_broadcastq_epi64(vcast<I128>(x)); }
B2D_INLINE I256 vsplati64i256(const I256& x) noexcept { return _mm256_broadcastq_epi64(vcast<I128>(x)); }

B2D_INLINE I256 vswapi64(const I256& x) noexcept { return vswizi64<0, 1>(x); }
B2D_INLINE I256 vdupli64(const I256& x) noexcept { return vswizi64<0, 0>(x); }
B2D_INLINE I256 vduphi64(const I256& x) noexcept { return vswizi64<1, 1>(x); }

B2D_INLINE I256 vswapi128(const I256& x) noexcept { return vpermi128<0, 1>(x); }
B2D_INLINE I256 vdupli128(const I128& x) noexcept { return vpermi128<0, 0>(vcast<I256>(x)); }
B2D_INLINE I256 vdupli128(const I256& x) noexcept { return vpermi128<0, 0>(x); }
B2D_INLINE I256 vduphi128(const I256& x) noexcept { return vpermi128<1, 1>(x); }

B2D_INLINE I256 vmovli128u8u16(const I128& x) noexcept { return _mm256_cvtepu8_epi16(x); }
B2D_INLINE I256 vmovli128u8u32(const I128& x) noexcept { return _mm256_cvtepu8_epi32(x); }
B2D_INLINE I256 vmovli128u8u64(const I128& x) noexcept { return _mm256_cvtepu8_epi64(x); }
B2D_INLINE I256 vmovli128u16u32(const I128& x) noexcept { return _mm256_cvtepu16_epi32(x); }
B2D_INLINE I256 vmovli128u16u64(const I128& x) noexcept { return _mm256_cvtepu16_epi64(x); }
B2D_INLINE I256 vmovli128u32u64(const I128& x) noexcept { return _mm256_cvtepu32_epi64(x); }

B2D_INLINE I256 vpacki16i8(const I256& x, const I256& y) noexcept { return _mm256_packs_epi16(x, y); }
B2D_INLINE I256 vpacki16u8(const I256& x, const I256& y) noexcept { return _mm256_packus_epi16(x, y); }
B2D_INLINE I256 vpacki32i16(const I256& x, const I256& y) noexcept { return _mm256_packs_epi32(x, y); }
B2D_INLINE I256 vpacki32u16(const I256& x, const I256& y) noexcept { return _mm256_packus_epi32(x, y); }

B2D_INLINE I256 vpacki16i8(const I256& x) noexcept { return vpacki16i8(x, x); }
B2D_INLINE I256 vpacki16u8(const I256& x) noexcept { return vpacki16u8(x, x); }
B2D_INLINE I256 vpacki32i16(const I256& x) noexcept { return vpacki32i16(x, x); }
B2D_INLINE I256 vpacki32u16(const I256& x) noexcept { return vpacki32u16(x, x); }

B2D_INLINE I256 vpacki32i8(const I256& x) noexcept { return vpacki16i8(vpacki32i16(x)); }
B2D_INLINE I256 vpacki32i8(const I256& x, const I256& y) noexcept { return vpacki16i8(vpacki32i16(x, y)); }
B2D_INLINE I256 vpacki32i8(const I256& x, const I256& y, const I256& z, const I256& w) noexcept { return vpacki16i8(vpacki32i16(x, y), vpacki32i16(z, w)); }

B2D_INLINE I256 vpacki32u8(const I256& x) noexcept { return vpacki16u8(vpacki32i16(x)); }
B2D_INLINE I256 vpacki32u8(const I256& x, const I256& y) noexcept { return vpacki16u8(vpacki32i16(x, y)); }
B2D_INLINE I256 vpacki32u8(const I256& x, const I256& y, const I256& z, const I256& w) noexcept { return vpacki16u8(vpacki32i16(x, y), vpacki32i16(z, w)); }

B2D_INLINE I256 vpackzzdb(const I256& x, const I256& y) noexcept { return vpacki16u8(vpacki32i16(x, y)); }
B2D_INLINE I256 vpackzzdb(const I256& x, const I256& y, const I256& z, const I256& w) noexcept { return vpacki16u8(vpacki32i16(x, y), vpacki32i16(z, w)); }

B2D_INLINE I256 vunpackli8(const I256& x, const I256& y) noexcept { return _mm256_unpacklo_epi8(x, y); }
B2D_INLINE I256 vunpackhi8(const I256& x, const I256& y) noexcept { return _mm256_unpackhi_epi8(x, y); }

B2D_INLINE I256 vunpackli16(const I256& x, const I256& y) noexcept { return _mm256_unpacklo_epi16(x, y); }
B2D_INLINE I256 vunpackhi16(const I256& x, const I256& y) noexcept { return _mm256_unpackhi_epi16(x, y); }

B2D_INLINE I256 vunpackli32(const I256& x, const I256& y) noexcept { return _mm256_unpacklo_epi32(x, y); }
B2D_INLINE I256 vunpackhi32(const I256& x, const I256& y) noexcept { return _mm256_unpackhi_epi32(x, y); }

B2D_INLINE I256 vunpackli64(const I256& x, const I256& y) noexcept { return _mm256_unpacklo_epi64(x, y); }
B2D_INLINE I256 vunpackhi64(const I256& x, const I256& y) noexcept { return _mm256_unpackhi_epi64(x, y); }

B2D_INLINE I256 vor(const I256& x, const I256& y) noexcept { return _mm256_or_si256(x, y); }
B2D_INLINE I256 vxor(const I256& x, const I256& y) noexcept { return _mm256_xor_si256(x, y); }
B2D_INLINE I256 vand(const I256& x, const I256& y) noexcept { return _mm256_and_si256(x, y); }
B2D_INLINE I256 vandn(const I256& x, const I256& y) noexcept { return _mm256_andnot_si256(y, x); }
B2D_INLINE I256 vnand(const I256& x, const I256& y) noexcept { return _mm256_andnot_si256(x, y); }

B2D_INLINE I256 vblendmask(const I256& x, const I256& y, const I256& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }
B2D_INLINE I256 vblendx(const I256& x, const I256& y, const I256& mask) noexcept { return _mm256_blendv_epi8(x, y, mask); }

B2D_INLINE I256 vaddi8(const I256& x, const I256& y) noexcept { return _mm256_add_epi8(x, y); }
B2D_INLINE I256 vaddi16(const I256& x, const I256& y) noexcept { return _mm256_add_epi16(x, y); }
B2D_INLINE I256 vaddi32(const I256& x, const I256& y) noexcept { return _mm256_add_epi32(x, y); }
B2D_INLINE I256 vaddi64(const I256& x, const I256& y) noexcept { return _mm256_add_epi64(x, y); }

B2D_INLINE I256 vaddsi8(const I256& x, const I256& y) noexcept { return _mm256_adds_epi8(x, y); }
B2D_INLINE I256 vaddsu8(const I256& x, const I256& y) noexcept { return _mm256_adds_epu8(x, y); }
B2D_INLINE I256 vaddsi16(const I256& x, const I256& y) noexcept { return _mm256_adds_epi16(x, y); }
B2D_INLINE I256 vaddsu16(const I256& x, const I256& y) noexcept { return _mm256_adds_epu16(x, y); }

B2D_INLINE I256 vsubi8(const I256& x, const I256& y) noexcept { return _mm256_sub_epi8(x, y); }
B2D_INLINE I256 vsubi16(const I256& x, const I256& y) noexcept { return _mm256_sub_epi16(x, y); }
B2D_INLINE I256 vsubi32(const I256& x, const I256& y) noexcept { return _mm256_sub_epi32(x, y); }
B2D_INLINE I256 vsubi64(const I256& x, const I256& y) noexcept { return _mm256_sub_epi64(x, y); }

B2D_INLINE I256 vsubsi8(const I256& x, const I256& y) noexcept { return _mm256_subs_epi8(x, y); }
B2D_INLINE I256 vsubsu8(const I256& x, const I256& y) noexcept { return _mm256_subs_epu8(x, y); }
B2D_INLINE I256 vsubsi16(const I256& x, const I256& y) noexcept { return _mm256_subs_epi16(x, y); }
B2D_INLINE I256 vsubsu16(const I256& x, const I256& y) noexcept { return _mm256_subs_epu16(x, y); }

B2D_INLINE I256 vmuli16(const I256& x, const I256& y) noexcept { return _mm256_mullo_epi16(x, y); }
B2D_INLINE I256 vmulu16(const I256& x, const I256& y) noexcept { return _mm256_mullo_epi16(x, y); }
B2D_INLINE I256 vmulhi16(const I256& x, const I256& y) noexcept { return _mm256_mulhi_epi16(x, y); }
B2D_INLINE I256 vmulhu16(const I256& x, const I256& y) noexcept { return _mm256_mulhi_epu16(x, y); }

template<uint8_t N_BITS> B2D_INLINE I256 vslli16(const I256& x) noexcept { return _mm256_slli_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I256 vslli32(const I256& x) noexcept { return _mm256_slli_epi32(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I256 vslli64(const I256& x) noexcept { return _mm256_slli_epi64(x, N_BITS); }

template<uint8_t N_BITS> B2D_INLINE I256 vsrli16(const I256& x) noexcept { return _mm256_srli_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I256 vsrli32(const I256& x) noexcept { return _mm256_srli_epi32(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I256 vsrli64(const I256& x) noexcept { return _mm256_srli_epi64(x, N_BITS); }

template<uint8_t N_BITS> B2D_INLINE I256 vsrai16(const I256& x) noexcept { return _mm256_srai_epi16(x, N_BITS); }
template<uint8_t N_BITS> B2D_INLINE I256 vsrai32(const I256& x) noexcept { return _mm256_srai_epi32(x, N_BITS); }

template<uint8_t N_BYTES> B2D_INLINE I256 vslli128b(const I256& x) noexcept { return _mm256_slli_si256(x, N_BYTES); }
template<uint8_t N_BYTES> B2D_INLINE I256 vsrli128b(const I256& x) noexcept { return _mm256_srli_si256(x, N_BYTES); }

B2D_INLINE I256 vmini8(const I256& x, const I256& y) noexcept { return _mm256_min_epi8(x, y); }
B2D_INLINE I256 vmaxi8(const I256& x, const I256& y) noexcept { return _mm256_max_epi8(x, y); }
B2D_INLINE I256 vminu8(const I256& x, const I256& y) noexcept { return _mm256_min_epu8(x, y); }
B2D_INLINE I256 vmaxu8(const I256& x, const I256& y) noexcept { return _mm256_max_epu8(x, y); }

B2D_INLINE I256 vmini16(const I256& x, const I256& y) noexcept { return _mm256_min_epi16(x, y); }
B2D_INLINE I256 vmaxi16(const I256& x, const I256& y) noexcept { return _mm256_max_epi16(x, y); }
B2D_INLINE I256 vminu16(const I256& x, const I256& y) noexcept { return _mm256_min_epu16(x, y); }
B2D_INLINE I256 vmaxu16(const I256& x, const I256& y) noexcept { return _mm256_max_epu16(x, y); }

B2D_INLINE I256 vmini32(const I256& x, const I256& y) noexcept { return _mm256_min_epi32(x, y); }
B2D_INLINE I256 vmaxi32(const I256& x, const I256& y) noexcept { return _mm256_max_epi32(x, y); }
B2D_INLINE I256 vminu32(const I256& x, const I256& y) noexcept { return _mm256_min_epu32(x, y); }
B2D_INLINE I256 vmaxu32(const I256& x, const I256& y) noexcept { return _mm256_max_epu32(x, y); }

B2D_INLINE I256 vcmpeqi8(const I256& x, const I256& y) noexcept { return _mm256_cmpeq_epi8(x, y); }
B2D_INLINE I256 vcmpgti8(const I256& x, const I256& y) noexcept { return _mm256_cmpgt_epi8(x, y); }

B2D_INLINE I256 vcmpeqi16(const I256& x, const I256& y) noexcept { return _mm256_cmpeq_epi16(x, y); }
B2D_INLINE I256 vcmpgti16(const I256& x, const I256& y) noexcept { return _mm256_cmpgt_epi16(x, y); }

B2D_INLINE I256 vcmpeqi32(const I256& x, const I256& y) noexcept { return _mm256_cmpeq_epi32(x, y); }
B2D_INLINE I256 vcmpgti32(const I256& x, const I256& y) noexcept { return _mm256_cmpgt_epi32(x, y); }

B2D_INLINE I256 vloadi256_32(const void* p) noexcept { return vcast<I256>(vloadi128_32(p)); }
B2D_INLINE I256 vloadi256_64(const void* p) noexcept { return vcast<I256>(vloadi128_64(p)); }
B2D_INLINE I256 vloadi256_128a(const void* p) noexcept { return vcast<I256>(vloadi128a(p)); }
B2D_INLINE I256 vloadi256_128u(const void* p) noexcept { return vcast<I256>(vloadi128u(p)); }
B2D_INLINE I256 vloadi256a(const void* p) noexcept { return _mm256_load_si256(static_cast<const I256*>(p)); }
B2D_INLINE I256 vloadi256u(const void* p) noexcept { return _mm256_loadu_si256(static_cast<const I256*>(p)); }

B2D_INLINE I256 vloadi256_l64(const I256& x, const void* p) noexcept { return vcast<I256>(vloadi128_l64(vcast<I128>(x), p)); }
B2D_INLINE I256 vloadi256_h64(const I256& x, const void* p) noexcept { return vcast<I256>(vloadi128_h64(vcast<I128>(x), p)); }

B2D_INLINE void vstorei32(void* p, const I256& x) noexcept { vstorei32(p, vcast<I128>(x)); }
B2D_INLINE void vstorei64(void* p, const I256& x) noexcept { vstorei64(p, vcast<I128>(x)); }
B2D_INLINE void vstorei128a(void* p, const I256& x) noexcept { vstorei128a(p, vcast<I128>(x)); }
B2D_INLINE void vstorei128u(void* p, const I256& x) noexcept { vstorei128u(p, vcast<I128>(x)); }
B2D_INLINE void vstorei256a(void* p, const I256& x) noexcept { _mm256_store_si256(static_cast<I256*>(p), x); }
B2D_INLINE void vstorei256u(void* p, const I256& x) noexcept { _mm256_storeu_si256(static_cast<I256*>(p), x); }

B2D_INLINE void vstoreli64(void* p, const I256& x) noexcept { vstoreli64(p, vcast<I128>(x)); }
B2D_INLINE void vstorehi64(void* p, const I256& x) noexcept { vstorehi64(p, vcast<I128>(x)); }

B2D_INLINE bool vhasmaski8(const I256& x, int bits0_31) noexcept { return _mm256_movemask_epi8(vcast<I256>(x)) == bits0_31; }
B2D_INLINE bool vhasmaski8(const F256& x, int bits0_31) noexcept { return _mm256_movemask_epi8(vcast<I256>(x)) == bits0_31; }
B2D_INLINE bool vhasmaski8(const D256& x, int bits0_31) noexcept { return _mm256_movemask_epi8(vcast<I256>(x)) == bits0_31; }

B2D_INLINE bool vhasmaski32(const I256& x, int bits0_7) noexcept { return _mm256_movemask_ps(vcast<F256>(x)) == bits0_7; }
B2D_INLINE bool vhasmaski64(const I256& x, int bits0_3) noexcept { return _mm256_movemask_pd(vcast<D256>(x)) == bits0_3; }

B2D_INLINE I256 vdiv255u16(const I256& x) noexcept { return vmulhu16(vaddi16(x, u16_0080_256.i256), u16_0101_256.i256); }
#endif

// ============================================================================
// [b2d::SIMD::F256]
// ============================================================================

#if B2D_ARCH_AVX
B2D_INLINE F256 vzerof256() noexcept { return _mm256_setzero_ps(); }

B2D_INLINE F256 vsetf256(float x) noexcept { return _mm256_set1_ps(x); }
B2D_INLINE F256 vsetf256(float x1, float x0) noexcept { return _mm256_set_ps(x1, x0, x1, x0, x1, x0, x1, x0); }
B2D_INLINE F256 vsetf256(float x3, float x2, float x1, float x0) noexcept { return _mm256_set_ps(x3, x2, x1, x0, x3, x2, x1, x0); }
B2D_INLINE F256 vsetf256(float x7, float x6, float x5, float x4, float x3, float x2, float x1, float x0) noexcept { return _mm256_set_ps(x7, x6, x5, x4, x3, x2, x1, x0); }

B2D_INLINE F256 vcvtf32f256(float x) noexcept { return vcast<F256>(vcvtf32f128(x)); }
B2D_INLINE float vcvtf256f32(const F256& x) noexcept { return vcvtf128f32(vcast<F128>(x)); }

B2D_INLINE F256 vcvti32f256(int32_t x) noexcept { return vcast<F256>(vcvti32f128(x)); }
B2D_INLINE int32_t vcvtf256i32(const F256& x) noexcept { return vcvtf128i32(vcast<F128>(x)); }
B2D_INLINE int32_t vcvttf256i32(const F256& x) noexcept { return vcvttf128i32(vcast<F128>(x)); }

#if B2D_ARCH_BITS >= 64
B2D_INLINE F256 vcvti64f256(int64_t x) noexcept { return vcast<F256>(vcvti64f128(x)); }
B2D_INLINE int64_t vcvtf256i64(const F256& x) noexcept { return vcvtf128i64(vcast<F128>(x)); }
B2D_INLINE int64_t vcvttf256i64(const F256& x) noexcept { return vcvttf128i64(vcast<F128>(x)); }
#endif

B2D_INLINE I256 vcvtf256i256(const F256& x) noexcept { return _mm256_cvtps_epi32(x); }
B2D_INLINE I256 vcvttf256i256(const F256& x) noexcept { return _mm256_cvttps_epi32(x); }

B2D_INLINE D256 vcvtf128d256(const F128& x) noexcept { return _mm256_cvtps_pd(vcast<F128>(x)); }
B2D_INLINE D256 vcvtf256d256(const F256& x) noexcept { return _mm256_cvtps_pd(vcast<F128>(x)); }

template<int A, int B, int C, int D>
B2D_INLINE F256 vshuff32(const F256& x, const F256& y) noexcept { return _mm256_shuffle_ps(x, y, _MM_SHUFFLE(A, B, C, D)); }
template<int A, int B, int C, int D>
B2D_INLINE F256 vswizf32(const F256& x) noexcept { return vshuff32<A, B, C, D>(x, x); }

template<int A, int B>
B2D_INLINE F256 vswizf64(const F256& x) noexcept { return vshuff32<A*2 + 1, A*2, B*2 + 1, B*2>(x, x); }

template<int A, int B>
B2D_INLINE F256 vpermf128(const F256& x, const F256& y) noexcept { return _mm256_permute2f128_ps(x, y, ((A & 0xF) << 4) + (B & 0xF)); }
template<int A, int B>
B2D_INLINE F256 vpermf128(const F256& x) noexcept { return vpermf128<A, B>(x, x); }

B2D_INLINE F256 vsplatf32f256(const F128& x) noexcept { return _mm256_broadcastss_ps(vcast<F128>(x)); }
B2D_INLINE F256 vsplatf32f256(const F256& x) noexcept { return _mm256_broadcastss_ps(vcast<F128>(x)); }

B2D_INLINE F256 vduplf32(const F256& x) noexcept { return vswizf32<2, 2, 0, 0>(x); }
B2D_INLINE F256 vduphf32(const F256& x) noexcept { return vswizf32<3, 3, 1, 1>(x); }

B2D_INLINE F256 vswapf64(const F256& x) noexcept { return vswizf64<0, 1>(x); }
B2D_INLINE F256 vduplf64(const F256& x) noexcept { return vswizf64<0, 0>(x); }
B2D_INLINE F256 vduphf64(const F256& x) noexcept { return vswizf64<1, 1>(x); }

B2D_INLINE F256 vswapf128(const F256& x) noexcept { return vpermf128<0, 1>(x); }
B2D_INLINE F256 vduplf128(const F128& x) noexcept { return vpermf128<0, 0>(vcast<F256>(x)); }
B2D_INLINE F256 vduplf128(const F256& x) noexcept { return vpermf128<0, 0>(x); }
B2D_INLINE F256 vduphf128(const F256& x) noexcept { return vpermf128<1, 1>(x); }

B2D_INLINE F256 vunpacklf32(const F256& x, const F256& y) noexcept { return _mm256_unpacklo_ps(x, y); }
B2D_INLINE F256 vunpackhf32(const F256& x, const F256& y) noexcept { return _mm256_unpackhi_ps(x, y); }

B2D_INLINE F256 vor(const F256& x, const F256& y) noexcept { return _mm256_or_ps(x, y); }
B2D_INLINE F256 vxor(const F256& x, const F256& y) noexcept { return _mm256_xor_ps(x, y); }
B2D_INLINE F256 vand(const F256& x, const F256& y) noexcept { return _mm256_and_ps(x, y); }
B2D_INLINE F256 vandn(const F256& x, const F256& y) noexcept { return _mm256_andnot_ps(y, x); }
B2D_INLINE F256 vnand(const F256& x, const F256& y) noexcept { return _mm256_andnot_ps(x, y); }
B2D_INLINE F256 vblendmask(const F256& x, const F256& y, const F256& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }

B2D_INLINE F256 vaddss(const F256& x, const F256& y) noexcept { return vcast<F256>(vaddss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vaddps(const F256& x, const F256& y) noexcept { return _mm256_add_ps(x, y); }

B2D_INLINE F256 vsubss(const F256& x, const F256& y) noexcept { return vcast<F256>(vsubss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vsubps(const F256& x, const F256& y) noexcept { return _mm256_sub_ps(x, y); }

B2D_INLINE F256 vmulss(const F256& x, const F256& y) noexcept { return vcast<F256>(vmulss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vmulps(const F256& x, const F256& y) noexcept { return _mm256_mul_ps(x, y); }

B2D_INLINE F256 vdivss(const F256& x, const F256& y) noexcept { return vcast<F256>(vdivss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vdivps(const F256& x, const F256& y) noexcept { return _mm256_div_ps(x, y); }

B2D_INLINE F256 vminss(const F256& x, const F256& y) noexcept { return vcast<F256>(vminss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vminps(const F256& x, const F256& y) noexcept { return _mm256_min_ps(x, y); }

B2D_INLINE F256 vmaxss(const F256& x, const F256& y) noexcept { return vcast<F256>(vmaxss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vmaxps(const F256& x, const F256& y) noexcept { return _mm256_max_ps(x, y); }

B2D_INLINE F256 vcmpeqss(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpeqss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpeqps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_EQ_OQ); }

B2D_INLINE F256 vcmpness(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpness(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpneps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_NEQ_OQ); }

B2D_INLINE F256 vcmpgess(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpgess(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpgeps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_GE_OQ); }

B2D_INLINE F256 vcmpgtss(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpgtss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpgtps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_GT_OQ); }

B2D_INLINE F256 vcmpless(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpless(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpleps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_LE_OQ); }

B2D_INLINE F256 vcmpltss(const F256& x, const F256& y) noexcept { return vcast<F256>(vcmpltss(vcast<F128>(x), vcast<F128>(y))); }
B2D_INLINE F256 vcmpltps(const F256& x, const F256& y) noexcept { return _mm256_cmp_ps(x, y, _CMP_LT_OQ); }

B2D_INLINE F256 vsqrtss(const F256& x) noexcept { return vcast<F256>(vsqrtss(vcast<F128>(x))); }
B2D_INLINE F256 vsqrtps(const F256& x) noexcept { return _mm256_sqrt_ps(x); }

B2D_INLINE F256 vloadf256_32(const void* p) noexcept { return vcast<F256>(vloadf128_32(p)); }
B2D_INLINE F256 vloadf256_64(const void* p) noexcept { return vcast<F256>(vloadf128_64(p)); }
B2D_INLINE F256 vloadf256_128a(const void* p) noexcept { return vcast<F256>(vloadf128a(p)); }
B2D_INLINE F256 vloadf256_128u(const void* p) noexcept { return vcast<F256>(vloadf128u(p)); }
B2D_INLINE F256 vloadf256a(const void* p) noexcept { return _mm256_load_ps(static_cast<const float*>(p)); }
B2D_INLINE F256 vloadf256u(const void* p) noexcept { return _mm256_loadu_ps(static_cast<const float*>(p)); }

B2D_INLINE F256 vloadf256_l64(const F256& x, const void* p) noexcept { return vcast<F256>(vloadf128_l64(vcast<F128>(x), p)); }
B2D_INLINE F256 vloadf256_h64(const F256& x, const void* p) noexcept { return vcast<F256>(vloadf128_h64(vcast<F128>(x), p)); }

B2D_INLINE F128 vbroadcastf128_32(const void* p) noexcept { return vcast<F128>(_mm_broadcast_ss(static_cast<const float*>(p))); }
B2D_INLINE F256 vbroadcastf256_32(const void* p) noexcept { return vcast<F256>(_mm256_broadcast_ss(static_cast<const float*>(p))); }
B2D_INLINE F256 vbroadcastf256_64(const void* p) noexcept { return vcast<F256>(_mm256_broadcast_sd(static_cast<const double*>(p))); }
B2D_INLINE F256 vbroadcastf256_128(const void* p) noexcept { return vcast<F256>(_mm256_broadcast_ps(static_cast<const __m128*>(p))); }

B2D_INLINE void vstoref32(void* p, const F256& x) noexcept { vstoref32(p, vcast<F128>(x)); }
B2D_INLINE void vstoref64(void* p, const F256& x) noexcept { vstoref64(p, vcast<F128>(x)); }
B2D_INLINE void vstorelf64(void* p, const F256& x) noexcept { vstorelf64(p, vcast<F128>(x)); }
B2D_INLINE void vstorehf64(void* p, const F256& x) noexcept { vstorehf64(p, vcast<F128>(x)); }
B2D_INLINE void vstoref128a(void* p, const F256& x) noexcept { vstoref128a(p, vcast<F128>(x)); }
B2D_INLINE void vstoref128u(void* p, const F256& x) noexcept { vstoref128u(p, vcast<F128>(x)); }
B2D_INLINE void vstoref256a(void* p, const F256& x) noexcept { _mm256_store_ps(static_cast<float*>(p), x); }
B2D_INLINE void vstoref256u(void* p, const F256& x) noexcept { _mm256_storeu_ps(static_cast<float*>(p), x); }

B2D_INLINE bool vhasmaskf32(const F256& x, int bits0_7) noexcept { return _mm256_movemask_ps(vcast<F256>(x)) == bits0_7; }
B2D_INLINE bool vhasmaskf64(const F256& x, int bits0_3) noexcept { return _mm256_movemask_pd(vcast<D256>(x)) == bits0_3; }
#endif

// ============================================================================
// [b2d::SIMD::D256]
// ============================================================================

#if B2D_ARCH_AVX
B2D_INLINE D256 vzerod256() noexcept { return _mm256_setzero_pd(); }
B2D_INLINE D256 vsetd256(double x) noexcept { return _mm256_set1_pd(x); }
B2D_INLINE D256 vsetd256(double x1, double x0) noexcept { return _mm256_set_pd(x1, x0, x1, x0); }
B2D_INLINE D256 vsetd256(double x3, double x2, double x1, double x0) noexcept { return _mm256_set_pd(x3, x2, x1, x0); }

B2D_INLINE D256 vcvtd64d256(double x) noexcept { return vcast<D256>(vcvtd64d128(x)); }
B2D_INLINE double vcvtd256d64(const D256& x) noexcept { return vcvtd128d64(vcast<D128>(x)); }

B2D_INLINE D256 vcvti32d256(int32_t x) noexcept { return vcast<D256>(vcvti32d128(x)); }
B2D_INLINE int32_t vcvtd256i32(const D256& x) noexcept { return vcvtd128i32(vcast<D128>(x)); }
B2D_INLINE int32_t vcvttd256i32(const D256& x) noexcept { return vcvttd128i32(vcast<D128>(x)); }

#if B2D_ARCH_BITS >= 64
B2D_INLINE D256 vcvti64d256(int64_t x) noexcept { return vcast<D256>(vcvti64d128(x)); }
B2D_INLINE int64_t vcvtd256i64(const D256& x) noexcept { return vcvtd128i64(vcast<D128>(x)); }
B2D_INLINE int64_t vcvttd256i64(const D256& x) noexcept { return vcvttd128i64(vcast<D128>(x)); }
#endif

B2D_INLINE I128 vcvtd256i128(const D256& x) noexcept { return vcast<I128>(_mm256_cvtpd_epi32(x)); }
B2D_INLINE I256 vcvtd256i256(const D256& x) noexcept { return vcast<I256>(_mm256_cvtpd_epi32(x)); }

B2D_INLINE I128 vcvttd256i128(const D256& x) noexcept { return vcast<I128>(_mm256_cvttpd_epi32(x)); }
B2D_INLINE I256 vcvttd256i256(const D256& x) noexcept { return vcast<I256>(_mm256_cvttpd_epi32(x)); }

B2D_INLINE F128 vcvtd256f128(const D256& x) noexcept { return vcast<F128>(_mm256_cvtpd_ps(x)); }
B2D_INLINE F256 vcvtd256f256(const D256& x) noexcept { return vcast<F256>(_mm256_cvtpd_ps(x)); }

template<int A, int B>
B2D_INLINE D256 vshufd64(const D256& x, const D256& y) noexcept { return _mm256_shuffle_pd(x, y, (A << 3) | (B << 2) | (A << 1) | B); }
template<int A, int B>
B2D_INLINE D256 vswizd64(const D256& x) noexcept { return vshufd64<A, B>(x, x); }

template<int A, int B>
B2D_INLINE D256 vpermd128(const D256& x, const D256& y) noexcept { return _mm256_permute2f128_pd(x, y, ((A & 0xF) << 4) + (B & 0xF)); }
template<int A, int B>
B2D_INLINE D256 vpermd128(const D256& x) noexcept { return vpermd128<A, B>(x, x); }

B2D_INLINE D256 vsplatd64d256(const D128& x) noexcept { return _mm256_broadcastsd_pd(vcast<D128>(x)); }
B2D_INLINE D256 vsplatd64d256(const D256& x) noexcept { return _mm256_broadcastsd_pd(vcast<D128>(x)); }

B2D_INLINE D256 vswapd64(const D256& x) noexcept { return vswizd64<0, 1>(x); }
B2D_INLINE D256 vdupld64(const D256& x) noexcept { return vswizd64<0, 0>(x); }
B2D_INLINE D256 vduphd64(const D256& x) noexcept { return vswizd64<1, 1>(x); }

B2D_INLINE D256 vswapd128(const D256& x) noexcept { return vpermd128<0, 1>(x); }
B2D_INLINE D256 vdupld128(const D128& x) noexcept { return vpermd128<0, 0>(vcast<D256>(x)); }
B2D_INLINE D256 vdupld128(const D256& x) noexcept { return vpermd128<0, 0>(x); }
B2D_INLINE D256 vduphd128(const D256& x) noexcept { return vpermd128<1, 1>(x); }

B2D_INLINE D256 vunpackld64(const D256& x, const D256& y) noexcept { return _mm256_unpacklo_pd(x, y); }
B2D_INLINE D256 vunpackhd64(const D256& x, const D256& y) noexcept { return _mm256_unpackhi_pd(x, y); }

B2D_INLINE D256 vor(const D256& x, const D256& y) noexcept { return _mm256_or_pd(x, y); }
B2D_INLINE D256 vxor(const D256& x, const D256& y) noexcept { return _mm256_xor_pd(x, y); }
B2D_INLINE D256 vand(const D256& x, const D256& y) noexcept { return _mm256_and_pd(x, y); }
B2D_INLINE D256 vandn(const D256& x, const D256& y) noexcept { return _mm256_andnot_pd(y, x); }
B2D_INLINE D256 vnand(const D256& x, const D256& y) noexcept { return _mm256_andnot_pd(x, y); }
B2D_INLINE D256 vblendmask(const D256& x, const D256& y, const D256& mask) noexcept { return vor(vnand(mask, x), vand(y, mask)); }

B2D_INLINE D256 vaddsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vaddsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vaddpd(const D256& x, const D256& y) noexcept { return _mm256_add_pd(x, y); }

B2D_INLINE D256 vsubsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vsubsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vsubpd(const D256& x, const D256& y) noexcept { return _mm256_sub_pd(x, y); }

B2D_INLINE D256 vmulsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vmulsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vmulpd(const D256& x, const D256& y) noexcept { return _mm256_mul_pd(x, y); }

B2D_INLINE D256 vdivsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vdivsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vdivpd(const D256& x, const D256& y) noexcept { return _mm256_div_pd(x, y); }

B2D_INLINE D256 vminsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vminsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vminpd(const D256& x, const D256& y) noexcept { return _mm256_min_pd(x, y); }

B2D_INLINE D256 vmaxsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vmaxsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vmaxpd(const D256& x, const D256& y) noexcept { return _mm256_max_pd(x, y); }

B2D_INLINE D256 vcmpeqsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmpeqsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmpeqpd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_EQ_OQ); }

B2D_INLINE D256 vcmpnesd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmpnesd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmpnepd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_NEQ_OQ); }

B2D_INLINE D256 vcmpgesd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmpgesd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmpgepd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_GE_OQ); }

B2D_INLINE D256 vcmpgtsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmpgtsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmpgtpd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_GT_OQ); }

B2D_INLINE D256 vcmplesd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmplesd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmplepd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_LE_OQ); }

B2D_INLINE D256 vcmpltsd(const D256& x, const D256& y) noexcept { return vcast<D256>(vcmpltsd(vcast<D128>(x), vcast<D128>(y))); }
B2D_INLINE D256 vcmpltpd(const D256& x, const D256& y) noexcept { return _mm256_cmp_pd(x, y, _CMP_LE_OQ); }

B2D_INLINE D256 vsqrtsd(const D256& x) noexcept { return vcast<D256>(vsqrtsd(vcast<D128>(x))); }
B2D_INLINE D256 vsqrtpd(const D256& x) noexcept { return _mm256_sqrt_pd(x); }

B2D_INLINE D256 vloadd256_64(const void* p) noexcept { return vcast<D256>(vloadd128_64(p)); }
B2D_INLINE D256 vloadd256_128a(const void* p) noexcept { return vcast<D256>(vloadd128a(p)); }
B2D_INLINE D256 vloadd256_128u(const void* p) noexcept { return vcast<D256>(vloadd128u(p)); }
B2D_INLINE D256 vloadd256a(const void* p) noexcept { return _mm256_load_pd(static_cast<const double*>(p)); }
B2D_INLINE D256 vloadd256u(const void* p) noexcept { return _mm256_loadu_pd(static_cast<const double*>(p)); }

B2D_INLINE D256 vloadd256_l64(const D256& x, const void* p) noexcept { return vcast<D256>(vloadd128_l64(vcast<D128>(x), p)); }
B2D_INLINE D256 vloadd256_h64(const D256& x, const void* p) noexcept { return vcast<D256>(vloadd128_h64(vcast<D128>(x), p)); }

B2D_INLINE D256 vbroadcastd256_64(const void* p) noexcept { return _mm256_broadcast_sd(static_cast<const double*>(p)); }
B2D_INLINE D256 vbroadcastd256_128(const void* p) noexcept { return _mm256_broadcast_pd(static_cast<const __m128d*>(p)); }

B2D_INLINE void vstored64(void* p, const D256& x) noexcept { vstored64(p, vcast<D128>(x)); }
B2D_INLINE void vstoreld64(void* p, const D256& x) noexcept { vstoreld64(p, vcast<D128>(x)); }
B2D_INLINE void vstorehd64(void* p, const D256& x) noexcept { vstorehd64(p, vcast<D128>(x)); }
B2D_INLINE void vstored128a(void* p, const D256& x) noexcept { vstored128a(p, vcast<D128>(x)); }
B2D_INLINE void vstored128u(void* p, const D256& x) noexcept { vstored128u(p, vcast<D128>(x)); }
B2D_INLINE void vstored256a(void* p, const D256& x) noexcept { _mm256_store_pd(static_cast<double*>(p), x); }
B2D_INLINE void vstored256u(void* p, const D256& x) noexcept { _mm256_storeu_pd(static_cast<double*>(p), x); }

B2D_INLINE bool vhasmaskd64(const D256& x, int bits0_3) noexcept { return _mm256_movemask_pd(vcast<D256>(x)) == bits0_3; }
#endif

} // anonymous namespace
} // SIMD namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_SIMD_X86_H
