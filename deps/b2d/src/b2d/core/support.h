// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_SUPPORT_H
#define _B2D_CORE_SUPPORT_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

//! \internal
//!
//! Contains support classes and functions that may be used by Blend2D source
//! and header files. Anything defined here is considered internal and should
//! not be used outside of Blend2D and its plugins.
namespace Support {

// ============================================================================
// [b2d::Support - Architecture Features & Constraints]
// ============================================================================

using Globals::kByteOrderLE;
using Globals::kByteOrderBE;
using Globals::kByteOrderNative;

static constexpr bool kUnalignedAccess16 = B2D_ARCH_X86 != 0;
static constexpr bool kUnalignedAccess32 = B2D_ARCH_X86 != 0;
static constexpr bool kUnalignedAccess64 = B2D_ARCH_X86 != 0;

// ============================================================================
// [b2d::Support - IntBySize / Int32Or64]
// ============================================================================

namespace Internal {
  template<typename T, size_t Alignment>
  struct AlignedInt {};

  template<> struct AlignedInt<uint16_t, 1> { typedef uint16_t B2D_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint16_t, 2> { typedef uint16_t T; };
  template<> struct AlignedInt<uint32_t, 1> { typedef uint32_t B2D_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint32_t, 2> { typedef uint32_t B2D_ALIGN_TYPE(T, 2); };
  template<> struct AlignedInt<uint32_t, 4> { typedef uint32_t T; };
  template<> struct AlignedInt<uint64_t, 1> { typedef uint64_t B2D_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint64_t, 2> { typedef uint64_t B2D_ALIGN_TYPE(T, 2); };
  template<> struct AlignedInt<uint64_t, 4> { typedef uint64_t B2D_ALIGN_TYPE(T, 4); };
  template<> struct AlignedInt<uint64_t, 8> { typedef uint64_t T; };

  // IntBySize  - Make an int-type by size (signed or unsigned) that is the
  //              same as types defined by <stdint.h>.
  // Int32Or64 - Make an int-type that has at least 32 bits: [u]int[32|64]_t.

  template<size_t SIZE, int IS_SIGNED>
  struct IntBySize {}; // Fail if not specialized.

  template<> struct IntBySize<1, 0> { typedef uint8_t  Type; };
  template<> struct IntBySize<1, 1> { typedef int8_t   Type; };
  template<> struct IntBySize<2, 0> { typedef uint16_t Type; };
  template<> struct IntBySize<2, 1> { typedef int16_t  Type; };
  template<> struct IntBySize<4, 0> { typedef uint32_t Type; };
  template<> struct IntBySize<4, 1> { typedef int32_t  Type; };
  template<> struct IntBySize<8, 0> { typedef uint64_t Type; };
  template<> struct IntBySize<8, 1> { typedef int64_t  Type; };

  template<typename T, int IS_SIGNED = std::is_signed<T>::value>
  struct Int32Or64 : public IntBySize<sizeof(T) <= 4 ? size_t(4) : sizeof(T), IS_SIGNED> {};
}

//! Cast an integer `x` to either `int32_t` or `int64_t` depending on `T`.
template<typename T>
constexpr typename Internal::Int32Or64<T, 1>::Type asInt(T x) noexcept {
  return (typename Internal::Int32Or64<T, 1>::Type)x;
}

//! Cast an integer `x` to either `uint32_t` or `uint64_t` depending on `T`.
template<typename T>
constexpr typename Internal::Int32Or64<T, 0>::Type asUInt(T x) noexcept {
  return (typename Internal::Int32Or64<T, 0>::Type)x;
}

//! Cast an integer `x` to either `int32_t`, uint32_t`, `int64_t`, or `uint64_t` depending on `T`.
template<typename T>
constexpr typename Internal::Int32Or64<T>::Type asNormalized(T x) noexcept {
  return (typename Internal::Int32Or64<T>::Type)x;
}

//! Cast an integer `x` to a fixed-width type as defined by <stdint.h> depending on `T`.
template<typename T>
constexpr typename Internal::IntBySize<sizeof(T), std::is_signed<T>::value>::Type asStdInt(T x) noexcept {
  return (typename Internal::IntBySize<sizeof(T), std::is_signed<T>::value>::Type)x;
}

// ============================================================================
// [b2d::Support - Bit Manipulation]
// ============================================================================

template<typename T>
constexpr uint32_t bitSizeOf() noexcept { return uint32_t(sizeof(T) * 8u); }

//! Storage used to store a pack of bits - should be the same as machine word.
typedef Internal::IntBySize<sizeof(uintptr_t), 0>::Type BitWord;

//! Number of bits stored in a single `BitWord`.
constexpr uint32_t kBitWordSizeInBits = bitSizeOf<BitWord>();

constexpr size_t bitWordCountFromBitCount(size_t nBits) noexcept { return (nBits + kBitWordSizeInBits - 1) / kBitWordSizeInBits; }

//! Returns `0 - x` in a safe way (no undefined behavior), works for both signed and unsigned numbers.
template<typename T>
constexpr T neg(const T& x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return T(U(0) - U(x));
}

template<typename X, typename Y>
constexpr X applySignMask(const X& x, const Y& mask) noexcept {
  typedef typename std::make_unsigned<X>::type U;
  return X((U(x) ^ U(mask)) - U(mask));
}

template<typename T>
constexpr T allOnes() noexcept { return neg<T>(T(1)); }

//! Returns `x << y` (shift left logical) by explicitly casting `x` to an unsigned type and back.
template<typename X, typename Y>
constexpr X shl(const X& x, const Y& y) noexcept {
  typedef typename std::make_unsigned<X>::type U;
  return X(U(x) << y);
}

//! Returns `x >> y` (shift right logical) by explicitly casting `x` to an unsigned type and back.
template<typename X, typename Y>
constexpr X shr(const X& x, const Y& y) noexcept {
  typedef typename std::make_unsigned<X>::type U;
  return X(U(x) >> y);
}

//! Returns `x >> y` (shift right arithmetic) by explicitly casting `x` to a signed type and back.
template<typename X, typename Y>
constexpr X sar(const X& x, const Y& y) noexcept {
  typedef typename std::make_signed<X>::type S;
  return X(S(x) >> y);
}

//! Returns `x | (x >> y)` - helper used by some bit manipulation helpers.
template<typename X, typename Y>
constexpr X or_shr(const X& x, const Y& y) noexcept { return X(x | shr(x, y)); }

namespace Internal {
  template<typename T>
  B2D_INLINE T rol(T x, unsigned n) noexcept {
    return shl(x, (     n) % unsigned(sizeof(T) * 8u)) |
           shr(x, (0u - n) % unsigned(sizeof(T) * 8u)) ;
  }

  template<typename T>
  B2D_INLINE T ror(T x, unsigned n) noexcept {
    return shr(x, (     n) % unsigned(sizeof(T) * 8u)) |
           shl(x, (0u - n) % unsigned(sizeof(T) * 8u)) ;
  }

  // MSC is unable to emit `rol|ror` instruction when `n` is not constant. Using
  // compiler intrinsics prevents us having constexpr, but increases performance.
  #if B2D_CXX_MSC
  template<> B2D_INLINE uint8_t rol(uint8_t x, unsigned n) noexcept { return uint8_t(_rotl8(x, n)); }
  template<> B2D_INLINE uint8_t ror(uint8_t x, unsigned n) noexcept { return uint8_t(_rotr8(x, n)); }
  template<> B2D_INLINE uint16_t rol(uint16_t x, unsigned n) noexcept { return uint16_t(_rotl16(x, n)); }
  template<> B2D_INLINE uint16_t ror(uint16_t x, unsigned n) noexcept { return uint16_t(_rotr16(x, n)); }
  template<> B2D_INLINE uint32_t rol(uint32_t x, unsigned n) noexcept { return uint32_t(_rotl(x, n)); }
  template<> B2D_INLINE uint32_t ror(uint32_t x, unsigned n) noexcept { return uint32_t(_rotr(x, n)); }
  template<> B2D_INLINE uint64_t rol(uint64_t x, unsigned n) noexcept { return uint64_t(_rotl64(x, n)); }
  template<> B2D_INLINE uint64_t ror(uint64_t x, unsigned n) noexcept { return uint64_t(_rotr64(x, n)); }
  #endif
}

template<typename X, typename Y>
B2D_INLINE X rol(X x, Y n) noexcept { return X(Internal::rol(asUInt(x), unsigned(n))); }

template<typename X, typename Y>
B2D_INLINE X ror(X x, Y n) noexcept { return X(Internal::ror(asUInt(x), unsigned(n))); }

//! Returns `x & -x` - extracts lowest set isolated bit (like BLSI instruction).
template<typename T>
constexpr T blsi(T x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return T(U(x) & neg(U(x)));
}

//! Returns `x & (x - 1)` - resets lowest set bit (like BLSR instruction).
template<typename T>
constexpr T blsr(T x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return T(U(x) & (U(x) - U(1)));
}

//! Generate a trailing bit-mask that has `n` least significant (trailing) bits set.
template<typename T, typename CountT>
constexpr T lsbMask(CountT n) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return (sizeof(U) < sizeof(uintptr_t))
    ? T(U((uintptr_t(1) << n) - uintptr_t(1)))
    // Shifting more bits than the type provides is UNDEFINED BEHAVIOR.
    // In such case we trash the result by ORing it with a mask that has
    // all bits set and discards the UNDEFINED RESULT of the shift.
    : T(((U(1) << n) - U(1u)) | neg(U(n >= CountT(bitSizeOf<T>()))));
}

//! Return a bit-mask that has `x` bit set.
template<typename T, typename Arg>
constexpr T bitMask(Arg x) noexcept { return T(1u) << x; }

//! Return a bit-mask that has `x` bit set (multiple arguments).
template<typename T, typename Arg, typename... ArgsT>
constexpr T bitMask(Arg x, ArgsT... args) noexcept { return T(bitMask<T>(x) | bitMask<T>(args...)); }

//! Get whether `x` has Nth bit set.
template<typename T, typename IndexT>
constexpr bool bitTest(T x, IndexT n) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return (U(x) & (U(1) << n)) != 0;
}

//! Get whether bits specified by `y` are all set in `x`.
template<typename X, typename Y>
constexpr bool bitMatch(X x, Y y) noexcept {
  return (x & y) == y;
}

//! Get whether the `x` is a power of two (only one bit is set).
template<typename T>
constexpr bool isPowerOf2(T x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return x && !(U(x) & (U(x) - U(1)));
}

template<typename T>
static B2D_INLINE bool isConsecutiveBitMask(T x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return x != 0 && (U(x) ^ (U(x) + (U(x) & (~U(x) + 1u)))) >= U(x);
}

namespace Internal {
  constexpr uint8_t fillTrailingBitsImpl(uint8_t x) noexcept { return or_shr(or_shr(or_shr(x, 1), 2), 4); }
  constexpr uint16_t fillTrailingBitsImpl(uint16_t x) noexcept { return or_shr(or_shr(or_shr(or_shr(x, 1), 2), 4), 8); }
  constexpr uint32_t fillTrailingBitsImpl(uint32_t x) noexcept { return or_shr(or_shr(or_shr(or_shr(or_shr(x, 1), 2), 4), 8), 16); }
  constexpr uint64_t fillTrailingBitsImpl(uint64_t x) noexcept { return or_shr(or_shr(or_shr(or_shr(or_shr(or_shr(x, 1), 2), 4), 8), 16), 32); }
}

//! Fill all trailing bits right from the first most significant bit set.
template<typename T>
constexpr T fillTrailingBits(const T& x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return T(Internal::fillTrailingBitsImpl(U(x)));
}

// ============================================================================
// [b2d::Support - CTZ]
// ============================================================================

namespace Internal {
  constexpr uint32_t ctzGenericImpl(uint32_t xAndNegX) noexcept {
    return 31 - ((xAndNegX & 0x0000FFFFu) ? 16 : 0)
              - ((xAndNegX & 0x00FF00FFu) ?  8 : 0)
              - ((xAndNegX & 0x0F0F0F0Fu) ?  4 : 0)
              - ((xAndNegX & 0x33333333u) ?  2 : 0)
              - ((xAndNegX & 0x55555555u) ?  1 : 0);
  }

  constexpr uint32_t ctzGenericImpl(uint64_t xAndNegX) noexcept {
    return 63 - ((xAndNegX & 0x00000000FFFFFFFFu) ? 32 : 0)
              - ((xAndNegX & 0x0000FFFF0000FFFFu) ? 16 : 0)
              - ((xAndNegX & 0x00FF00FF00FF00FFu) ?  8 : 0)
              - ((xAndNegX & 0x0F0F0F0F0F0F0F0Fu) ?  4 : 0)
              - ((xAndNegX & 0x3333333333333333u) ?  2 : 0)
              - ((xAndNegX & 0x5555555555555555u) ?  1 : 0);
  }

  template<typename T>
  constexpr uint32_t ctzGeneric(T x) noexcept {
    return ctzGenericImpl(x & neg(x));
  }

  static B2D_INLINE uint32_t ctz(uint32_t x) noexcept {
    #if B2D_CXX_MSC
    unsigned long i;
    _BitScanForward(&i, x);
    return uint32_t(i);
    #elif B2D_CXX_GNU || B2D_CXX_CLANG
    return uint32_t(__builtin_ctz(x));
    #else
    return ctzGeneric(x);
    #endif
  }

  static B2D_INLINE uint32_t ctz(uint64_t x) noexcept {
    #if B2D_CXX_MSC && (B2D_ARCH_X86 == 64 || B2D_ARCH_ARM == 64)
    unsigned long i;
    _BitScanForward64(&i, x);
    return uint32_t(i);
    #elif B2D_CXX_GNU || B2D_CXX_CLANG
    return uint32_t(__builtin_ctzll(x));
    #else
    return ctzGeneric(x);
    #endif
  }
}

//! Count trailing zeros in `x` (returns a position of a first bit set in `x`).
//!
//! NOTE: The input MUST NOT be zero, otherwise the result is undefined.
template<typename T>
static B2D_INLINE uint32_t ctz(T x) noexcept { return Internal::ctz(asUInt(x)); }

// ============================================================================
// [b2d::Support - Advance]
// ============================================================================

template<typename X, typename Y>
constexpr X advancePtr(X ptr, Y offset) noexcept {
  return (X)( (uintptr_t)(ptr) + (uintptr_t)(intptr_t)offset);
}

// ============================================================================
// [b2d::Support - Alignment]
// ============================================================================

template<typename X, typename Y>
constexpr bool isAligned(X base, Y alignment) noexcept {
  typedef typename Internal::IntBySize<sizeof(X), 0>::Type U;
  return ((U)base % (U)alignment) == 0;
}

template<typename X, typename Y>
constexpr X alignUp(X x, Y alignment) noexcept {
  typedef typename Internal::IntBySize<sizeof(X), 0>::Type U;
  return (X)( ((U)x + ((U)(alignment) - 1u)) & ~((U)(alignment) - 1u) );
}

template<typename X, typename Y>
constexpr X alignDown(X x, Y alignment) noexcept {
  typedef typename Internal::IntBySize<sizeof(X), 0>::Type U;
  return (X)( (U)x & ~((U)(alignment) - 1u) );
}

//! Get zero or a positive difference between `base` and `base` aligned to `alignment`.
template<typename X, typename Y>
constexpr typename Internal::IntBySize<sizeof(X), 0>::Type alignUpDiff(X base, Y alignment) noexcept {
  typedef typename Internal::IntBySize<sizeof(X), 0>::Type U;
  return alignUp(U(base), alignment) - U(base);
}

template<typename T>
constexpr T alignUpPowerOf2(T x) noexcept {
  typedef typename Internal::IntBySize<sizeof(T), 0>::Type U;
  return (T)(fillTrailingBits(U(x) - 1u) + 1u);
}

// ============================================================================
// [b2d::Support - NumGranularized]
// ============================================================================

//! Calculate the number of elements that would be required if `base` is
//! granularized by `granularity`. This function can be used to calculate
//! the number of BitWords to represent N bits, for example.
template<typename X, typename Y>
constexpr X numGranularized(X base, Y granularity) noexcept {
  typedef typename Internal::IntBySize<sizeof(X), 0>::Type U;
  return X((U(base) + U(granularity) - 1) / U(granularity));
}

// ============================================================================
// [b2d::Support - ByteSwap]
// ============================================================================

namespace Internal {
  static B2D_INLINE uint16_t byteswap16(uint16_t x) noexcept {
    #if B2D_CXX_MSC
    return uint32_t(_byteswap_ushort(x));
    #else
    return (x << 8) | (x >> 8);
    #endif
  }

  static B2D_INLINE uint32_t byteswap24(uint32_t x) noexcept {
    return ((x << 16) & 0x00FF0000u) |
           ((x      ) & 0x0000FF00u) |
           ((x >> 16) & 0x000000FFu) ;
  }

  static B2D_INLINE uint32_t byteswap32(uint32_t x) noexcept {
    #if B2D_CXX_MSC
    return uint32_t(_byteswap_ulong(x));
    #elif B2D_CXX_HAS_BUILTIN(__builtin_bswap32, B2D_CXX_GNU >= B2D_CXX_VER(4, 3, 0))
    return __builtin_bswap32(x);
    #else
    return (x << 24) | (x >> 24) | ((x << 8) & 0x00FF0000u) | ((x >> 8) & 0x0000FF00);
    #endif
  }

  static B2D_INLINE uint64_t byteswap64(uint64_t x) noexcept {
    #if B2D_CXX_MSC
    return uint64_t(_byteswap_uint64(x));
    #elif B2D_CXX_HAS_BUILTIN(__builtin_bswap64, B2D_CXX_GNU >= B2D_CXX_VER(4, 3, 0))
    return __builtin_bswap64(x);
    #else
    return ( (uint64_t(byteswap32(uint32_t(x >> 32        )))      ) |
             (uint64_t(byteswap32(uint32_t(x & 0xFFFFFFFFu))) << 32) );
    #endif
  }
}

template<typename T> static B2D_INLINE T byteswap16(T x) noexcept { return T(Internal::byteswap16(uint16_t(x))); }
template<typename T> static B2D_INLINE T byteswap24(T x) noexcept { return T(Internal::byteswap24(uint32_t(x))); }
template<typename T> static B2D_INLINE T byteswap32(T x) noexcept { return T(Internal::byteswap32(uint32_t(x))); }
template<typename T> static B2D_INLINE T byteswap64(T x) noexcept { return T(Internal::byteswap64(uint64_t(x))); }

// ============================================================================
// [b2d::Support - Byte Manipulation]
// ============================================================================

constexpr uint32_t bytepack32_4x8(uint32_t a, uint32_t b, uint32_t c, uint32_t d) noexcept {
  return B2D_ARCH_LE ? (a | (b << 8) | (c << 16) | (d << 24))
                     : (d | (c << 8) | (b << 16) | (a << 24)) ;
}

template<typename T>
constexpr T repeatByteSequence(T x) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return T(U(x) * U(uint64_t(0x0101010101010101u) & allOnes<U>()));
}

// ============================================================================
// [b2d::Support - Clamp]
// ============================================================================

namespace Internal {
  // Internal::clamp<T> - `clampToByte()` and `clampToWord()` implementation.
  template<typename SrcT, typename DstT>
  constexpr DstT clamp(SrcT x, DstT y) noexcept {
    typedef typename std::make_unsigned<SrcT>::type U;
    return U(x) <= U(y)                  ? DstT(x) :
           std::is_unsigned<SrcT>::value ? DstT(y) : DstT(SrcT(y) & SrcT(sar(neg(x), sizeof(SrcT) * 8 - 1)));
  }
}

//! Clamp a value `x` to a byte (unsigned 8-bit type).
template<typename T>
constexpr uint8_t clampToByte(T x) noexcept {
  return Internal::clamp<T, uint8_t>(x, uint8_t(0xFFu));
}

//! Clamp a value `x` to a word (unsigned 16-bit type).
template<typename T>
constexpr uint16_t clampToWord(T x) noexcept {
  return Internal::clamp<T, uint16_t>(x, uint16_t(0xFFFFu));
}

// ============================================================================
// [b2d::Support - UDiv255]
// ============================================================================

//! Integer division by 255, compatible with `(x + (x >> 8)) >> 8` used by SIMD.
constexpr uint32_t udiv255(uint32_t x) noexcept { return ((x + 128) * 257) >> 16; }

// ============================================================================
// [b2d::Support - Safe Arithmetic]
// ============================================================================

typedef unsigned char OverflowFlag;

namespace Internal {
  template<typename T>
  B2D_INLINE T safeAddFallback(T x, T y, OverflowFlag& overflow) noexcept {
    typedef typename std::make_unsigned<T>::type U;

    U result = U(x) + U(y);
    overflow = OverflowFlag(std::is_unsigned<T>::value ? result < U(x) : T((U(x) ^ ~U(y)) & (U(x) ^ result)) < 0);
    return T(result);
  }

  template<typename T>
  B2D_INLINE T safeSubFallback(T x, T y, OverflowFlag& overflow) noexcept {
    typedef typename std::make_unsigned<T>::type U;

    U result = U(x) - U(y);
    overflow = OverflowFlag(std::is_unsigned<T>::value ? result > U(x) : T((U(x) ^ U(y)) & (U(x) ^ result)) < 0);
    return T(result);
  }

  template<typename T>
  B2D_INLINE T safeMulFallback(T x, T y, OverflowFlag& overflow) noexcept {
    typedef typename Internal::IntBySize<sizeof(T) * 2, std::is_signed<T>::value>::Type I;
    typedef typename std::make_unsigned<I>::type U;

    U mask = U(std::numeric_limits<typename std::make_unsigned<T>::type>::max());
    if (std::is_signed<T>::value) {
      U prod = U(I(x)) * U(I(y));
      overflow = OverflowFlag(I(prod) < I(std::numeric_limits<T>::min()) ||
                              I(prod) > I(std::numeric_limits<T>::max()) );
      return T(I(prod & mask));
    }
    else {
      U prod = U(x) * U(y);
      overflow = OverflowFlag((prod & ~mask) != 0);
      return T(prod & mask);
    }
  }

  template<>
  B2D_INLINE int64_t safeMulFallback(int64_t x, int64_t y, OverflowFlag& overflow) noexcept {
    int64_t result = int64_t(uint64_t(x) * uint64_t(y));
    overflow = OverflowFlag(x && (result / x != y));
    return result;
  }

  template<>
  B2D_INLINE uint64_t safeMulFallback(uint64_t x, uint64_t y, OverflowFlag& overflow) noexcept {
    uint64_t result = x * y;
    overflow = OverflowFlag(y != 0 && std::numeric_limits<uint64_t>::max() / y < x);
    return result;
  }

  // These can be specialized.
  template<typename T> B2D_INLINE T safeAdd(T x, T y, OverflowFlag& overflow) noexcept { return safeAddFallback(x, y, overflow); }
  template<typename T> B2D_INLINE T safeSub(T x, T y, OverflowFlag& overflow) noexcept { return safeSubFallback(x, y, overflow); }
  template<typename T> B2D_INLINE T safeMul(T x, T y, OverflowFlag& overflow) noexcept { return safeMulFallback(x, y, overflow); }

  #if B2D_CXX_CLANG >= B2D_CXX_VER(3, 4, 0) || \
      B2D_CXX_GNU   >= B2D_CXX_VER(5, 0, 0)
  #define B2D_SAFE_ARITH_SPECIALIZE(FUNC, T, ALT_T, BUILTIN)                  \
    template<>                                                                \
    B2D_INLINE T FUNC(T x, T y, OverflowFlag& overflow) noexcept {            \
      ALT_T result;                                                           \
      overflow = OverflowFlag(BUILTIN((ALT_T)x, (ALT_T)y, &result));          \
      return T(result);                                                       \
    }
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, int32_t , int               , __builtin_sadd_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, uint32_t, unsigned int      , __builtin_uadd_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, int64_t , long long         , __builtin_saddll_overflow)
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, uint64_t, unsigned long long, __builtin_uaddll_overflow)
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, int32_t , int               , __builtin_ssub_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, uint32_t, unsigned int      , __builtin_usub_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, int64_t , long long         , __builtin_ssubll_overflow)
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, uint64_t, unsigned long long, __builtin_usubll_overflow)
  B2D_SAFE_ARITH_SPECIALIZE(safeMul, int32_t , int               , __builtin_smul_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeMul, uint32_t, unsigned int      , __builtin_umul_overflow  )
  B2D_SAFE_ARITH_SPECIALIZE(safeMul, int64_t , long long         , __builtin_smulll_overflow)
  B2D_SAFE_ARITH_SPECIALIZE(safeMul, uint64_t, unsigned long long, __builtin_umulll_overflow)
  #undef B2D_SAFE_ARITH_SPECIALIZE
  #endif

  // There is a bug in MSVC that makes these specializations unusable, maybe in the future...
  #if 0 && B2D_CXX_MSC
  #define B2D_SAFE_ARITH_SPECIALIZE(FUNC, T, ALT_T, BUILTIN)                  \
    template<>                                                                \
    B2D_INLINE T FUNC(T x, T y, OverflowFlag& overflow) noexcept {            \
      ALT_T result;                                                           \
      overflow = OverflowFlag(BUILTIN(0, (ALT_T)x, (ALT_T)y, &result));       \
      return T(result);                                                       \
    }
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, uint32_t, unsigned int      , _addcarry_u32 )
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, uint32_t, unsigned int      , _subborrow_u32)
  #if ARCH_BITS >= 64
  B2D_SAFE_ARITH_SPECIALIZE(safeAdd, uint64_t, unsigned __int64  , _addcarry_u64 )
  B2D_SAFE_ARITH_SPECIALIZE(safeSub, uint64_t, unsigned __int64  , _subborrow_u64)
  #endif
  #undef B2D_SAFE_ARITH_SPECIALIZE
  #endif
}

template<typename T>
static B2D_INLINE T safeAdd(T x, T y, OverflowFlag* overflow) noexcept {
  auto result = Internal::safeAdd(asStdInt(x), asStdInt(y), *overflow);
  return T(result);
}

template<typename T>
static B2D_INLINE T safeSub(T x, T y, OverflowFlag* overflow) noexcept {
  auto result = Internal::safeSub(asStdInt(x), asStdInt(y), *overflow);
  return T(result);
}

template<typename T>
static B2D_INLINE T safeMul(T x, T y, OverflowFlag* overflow) noexcept {
  auto result = Internal::safeMul(asStdInt(x), asStdInt(y), *overflow);
  return T(result);
}

//! Returns `base + x * y` and sets `overflow` flags on overflow.
template<typename T>
static B2D_INLINE T safeMulAdd(T base, T x, T y, OverflowFlag* overflow) noexcept {
  OverflowFlag ofMul;
  OverflowFlag ofAdd;

  auto prod   = Internal::safeMul(asStdInt(x), asStdInt(y), ofMul);
  auto result = Internal::safeAdd(asStdInt(base), asStdInt(prod), ofAdd);

  *overflow = ofMul | ofAdd;
  return T(result);
}

// ============================================================================
// [b2d::Support - Read / Write]
// ============================================================================

static inline uint32_t readU8(const void* p) noexcept { return uint32_t(static_cast<const uint8_t*>(p)[0]); }
static inline int32_t readI8(const void* p) noexcept { return int32_t(static_cast<const int8_t*>(p)[0]); }

template<uint32_t BO, size_t Alignment>
static inline uint32_t readU16x(const void* p) noexcept {
  if (BO == kByteOrderNative && (kUnalignedAccess16 || Alignment >= 2)) {
    typedef typename Internal::AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
    return uint32_t(static_cast<const U16AlignedToN*>(p)[0]);
  }
  else {
    uint32_t hi = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 1 : 0));
    uint32_t lo = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 0 : 1));
    return shl(hi, 8) | lo;
  }
}

template<uint32_t BO, size_t Alignment>
static inline int32_t readI16x(const void* p) noexcept {
  if (BO == kByteOrderNative && (kUnalignedAccess16 || Alignment >= 2)) {
    typedef typename Internal::AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
    return int32_t(int16_t(static_cast<const U16AlignedToN*>(p)[0]));
  }
  else {
    int32_t hi = readI8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 1 : 0));
    int32_t lo = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 0 : 1));
    return shl(hi, 8) | lo;
  }
}

template<uint32_t BO = kByteOrderNative>
static inline uint32_t readU24u(const void* p) noexcept {
  uint32_t b0 = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 2 : 0));
  uint32_t b1 = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 1 : 1));
  uint32_t b2 = readU8(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 0 : 2));
  return shl(b0, 16) | shl(b1, 8) | b2;
}

template<uint32_t BO, size_t Alignment>
static inline uint32_t readU32x(const void* p) noexcept {
  if (kUnalignedAccess32 || Alignment >= 4) {
    typedef typename Internal::AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
    uint32_t x = static_cast<const U32AlignedToN*>(p)[0];
    return BO == kByteOrderNative ? x : byteswap32(x);
  }
  else {
    uint32_t hi = readU16x<BO, Alignment >= 2 ? size_t(2) : Alignment>(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 2 : 0));
    uint32_t lo = readU16x<BO, Alignment >= 2 ? size_t(2) : Alignment>(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 0 : 2));
    return shl(hi, 16) | lo;
  }
}

template<uint32_t BO, size_t Alignment>
static inline uint64_t readU64x(const void* p) noexcept {
  if (BO == kByteOrderNative && (kUnalignedAccess64 || Alignment >= 8)) {
    typedef typename Internal::AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
    return static_cast<const U64AlignedToN*>(p)[0];
  }
  else {
    uint32_t hi = readU32x<BO, Alignment >= 4 ? size_t(4) : Alignment>(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 4 : 0));
    uint32_t lo = readU32x<BO, Alignment >= 4 ? size_t(4) : Alignment>(static_cast<const uint8_t*>(p) + (BO == kByteOrderLE ? 0 : 4));
    return shl(uint64_t(hi), 32) | lo;
  }
}

template<uint32_t BO, size_t Alignment>
static inline int32_t readI32x(const void* p) noexcept { return int32_t(readU32x<BO, Alignment>(p)); }

template<uint32_t BO, size_t Alignment>
static inline int64_t readI64x(const void* p) noexcept { return int64_t(readU64x<BO, Alignment>(p)); }

template<size_t Alignment> static inline int32_t readI16xLE(const void* p) noexcept { return readI16x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline int32_t readI16xBE(const void* p) noexcept { return readI16x<kByteOrderBE, Alignment>(p); }
template<size_t Alignment> static inline uint32_t readU16xLE(const void* p) noexcept { return readU16x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline uint32_t readU16xBE(const void* p) noexcept { return readU16x<kByteOrderBE, Alignment>(p); }
template<size_t Alignment> static inline int32_t readI32xLE(const void* p) noexcept { return readI32x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline int32_t readI32xBE(const void* p) noexcept { return readI32x<kByteOrderBE, Alignment>(p); }
template<size_t Alignment> static inline uint32_t readU32xLE(const void* p) noexcept { return readU32x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline uint32_t readU32xBE(const void* p) noexcept { return readU32x<kByteOrderBE, Alignment>(p); }
template<size_t Alignment> static inline int64_t readI64xLE(const void* p) noexcept { return readI64x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline int64_t readI64xBE(const void* p) noexcept { return readI64x<kByteOrderBE, Alignment>(p); }
template<size_t Alignment> static inline uint64_t readU64xLE(const void* p) noexcept { return readU64x<kByteOrderLE, Alignment>(p); }
template<size_t Alignment> static inline uint64_t readU64xBE(const void* p) noexcept { return readU64x<kByteOrderBE, Alignment>(p); }

static inline int32_t readI16a(const void* p) noexcept { return readI16x<kByteOrderNative, 2>(p); }
static inline int32_t readI16u(const void* p) noexcept { return readI16x<kByteOrderNative, 1>(p); }
static inline uint32_t readU16a(const void* p) noexcept { return readU16x<kByteOrderNative, 2>(p); }
static inline uint32_t readU16u(const void* p) noexcept { return readU16x<kByteOrderNative, 1>(p); }

static inline int32_t readI16aLE(const void* p) noexcept { return readI16xLE<2>(p); }
static inline int32_t readI16uLE(const void* p) noexcept { return readI16xLE<1>(p); }
static inline uint32_t readU16aLE(const void* p) noexcept { return readU16xLE<2>(p); }
static inline uint32_t readU16uLE(const void* p) noexcept { return readU16xLE<1>(p); }

static inline int32_t readI16aBE(const void* p) noexcept { return readI16xBE<2>(p); }
static inline int32_t readI16uBE(const void* p) noexcept { return readI16xBE<1>(p); }
static inline uint32_t readU16aBE(const void* p) noexcept { return readU16xBE<2>(p); }
static inline uint32_t readU16uBE(const void* p) noexcept { return readU16xBE<1>(p); }

static inline uint32_t readU24uLE(const void* p) noexcept { return readU24u<kByteOrderLE>(p); }
static inline uint32_t readU24uBE(const void* p) noexcept { return readU24u<kByteOrderBE>(p); }

static inline int32_t readI32a(const void* p) noexcept { return readI32x<kByteOrderNative, 4>(p); }
static inline int32_t readI32u(const void* p) noexcept { return readI32x<kByteOrderNative, 1>(p); }
static inline uint32_t readU32a(const void* p) noexcept { return readU32x<kByteOrderNative, 4>(p); }
static inline uint32_t readU32u(const void* p) noexcept { return readU32x<kByteOrderNative, 1>(p); }

static inline int32_t readI32aLE(const void* p) noexcept { return readI32xLE<4>(p); }
static inline int32_t readI32uLE(const void* p) noexcept { return readI32xLE<1>(p); }
static inline uint32_t readU32aLE(const void* p) noexcept { return readU32xLE<4>(p); }
static inline uint32_t readU32uLE(const void* p) noexcept { return readU32xLE<1>(p); }

static inline int32_t readI32aBE(const void* p) noexcept { return readI32xBE<4>(p); }
static inline int32_t readI32uBE(const void* p) noexcept { return readI32xBE<1>(p); }
static inline uint32_t readU32aBE(const void* p) noexcept { return readU32xBE<4>(p); }
static inline uint32_t readU32uBE(const void* p) noexcept { return readU32xBE<1>(p); }

static inline int64_t readI64a(const void* p) noexcept { return readI64x<kByteOrderNative, 8>(p); }
static inline int64_t readI64u(const void* p) noexcept { return readI64x<kByteOrderNative, 1>(p); }
static inline uint64_t readU64a(const void* p) noexcept { return readU64x<kByteOrderNative, 8>(p); }
static inline uint64_t readU64u(const void* p) noexcept { return readU64x<kByteOrderNative, 1>(p); }

static inline int64_t readI64aLE(const void* p) noexcept { return readI64xLE<8>(p); }
static inline int64_t readI64uLE(const void* p) noexcept { return readI64xLE<1>(p); }
static inline uint64_t readU64aLE(const void* p) noexcept { return readU64xLE<8>(p); }
static inline uint64_t readU64uLE(const void* p) noexcept { return readU64xLE<1>(p); }

static inline int64_t readI64aBE(const void* p) noexcept { return readI64xBE<8>(p); }
static inline int64_t readI64uBE(const void* p) noexcept { return readI64xBE<1>(p); }
static inline uint64_t readU64aBE(const void* p) noexcept { return readU64xBE<8>(p); }
static inline uint64_t readU64uBE(const void* p) noexcept { return readU64xBE<1>(p); }

static inline void writeU8(void* p, uint32_t x) noexcept { static_cast<uint8_t*>(p)[0] = uint8_t(x & 0xFFu); }
static inline void writeI8(void* p, int32_t x) noexcept { static_cast<uint8_t*>(p)[0] = uint8_t(x & 0xFF); }

template<uint32_t BO, size_t Alignment>
static inline void writeU16x(void* p, uint32_t x) noexcept {
  if (BO == kByteOrderNative && (kUnalignedAccess16 || Alignment >= 2)) {
    typedef typename Internal::AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
    static_cast<U16AlignedToN*>(p)[0] = uint16_t(x & 0xFFFFu);
  }
  else {
    static_cast<uint8_t*>(p)[0] = uint8_t((x >> (BO == kByteOrderLE ? 0 : 8)) & 0xFFu);
    static_cast<uint8_t*>(p)[1] = uint8_t((x >> (BO == kByteOrderLE ? 8 : 0)) & 0xFFu);
  }
}

template<uint32_t BO = kByteOrderNative>
static inline void writeU24u(void* p, uint32_t v) noexcept {
  static_cast<uint8_t*>(p)[0] = uint8_t((v >> (BO == kByteOrderLE ?  0 : 16)) & 0xFFu);
  static_cast<uint8_t*>(p)[1] = uint8_t((v >> (BO == kByteOrderLE ?  8 :  8)) & 0xFFu);
  static_cast<uint8_t*>(p)[2] = uint8_t((v >> (BO == kByteOrderLE ? 16 :  0)) & 0xFFu);
}

template<uint32_t BO, size_t Alignment>
static inline void writeU32x(void* p, uint32_t x) noexcept {
  if (kUnalignedAccess32 || Alignment >= 4) {
    typedef typename Internal::AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
    static_cast<U32AlignedToN*>(p)[0] = (BO == kByteOrderNative) ? x : Support::byteswap32(x);
  }
  else {
    writeU16x<BO, Alignment >= 2 ? size_t(2) : Alignment>(static_cast<uint8_t*>(p) + 0, x >> (BO == kByteOrderLE ?  0 : 16));
    writeU16x<BO, Alignment >= 2 ? size_t(2) : Alignment>(static_cast<uint8_t*>(p) + 2, x >> (BO == kByteOrderLE ? 16 :  0));
  }
}

template<uint32_t BO, size_t Alignment>
static inline void writeU64x(void* p, uint64_t x) noexcept {
  if (BO == kByteOrderNative && (kUnalignedAccess64 || Alignment >= 8)) {
    typedef typename Internal::AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
    static_cast<U64AlignedToN*>(p)[0] = x;
  }
  else {
    writeU32x<BO, Alignment >= 4 ? size_t(4) : Alignment>(static_cast<uint8_t*>(p) + 0, uint32_t((x >> (BO == kByteOrderLE ?  0 : 32)) & 0xFFFFFFFFu));
    writeU32x<BO, Alignment >= 4 ? size_t(4) : Alignment>(static_cast<uint8_t*>(p) + 4, uint32_t((x >> (BO == kByteOrderLE ? 32 :  0)) & 0xFFFFFFFFu));
  }
}

template<uint32_t BO, size_t Alignment> static inline void writeI16x(void* p, int32_t x) noexcept { writeU16x<BO, Alignment>(p, uint32_t(x)); }
template<uint32_t BO, size_t Alignment> static inline void writeI32x(void* p, int32_t x) noexcept { writeU32x<BO, Alignment>(p, uint32_t(x)); }
template<uint32_t BO, size_t Alignment> static inline void writeI64x(void* p, int64_t x) noexcept { writeU64x<BO, Alignment>(p, uint64_t(x)); }

template<size_t Alignment> static inline void writeI16xLE(void* p, int32_t x) noexcept { writeI16x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeI16xBE(void* p, int32_t x) noexcept { writeI16x<kByteOrderBE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU16xLE(void* p, uint32_t x) noexcept { writeU16x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU16xBE(void* p, uint32_t x) noexcept { writeU16x<kByteOrderBE, Alignment>(p, x); }

template<size_t Alignment> static inline void writeI32xLE(void* p, int32_t x) noexcept { writeI32x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeI32xBE(void* p, int32_t x) noexcept { writeI32x<kByteOrderBE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU32xLE(void* p, uint32_t x) noexcept { writeU32x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU32xBE(void* p, uint32_t x) noexcept { writeU32x<kByteOrderBE, Alignment>(p, x); }

template<size_t Alignment> static inline void writeI64xLE(void* p, int64_t x) noexcept { writeI64x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeI64xBE(void* p, int64_t x) noexcept { writeI64x<kByteOrderBE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU64xLE(void* p, uint64_t x) noexcept { writeU64x<kByteOrderLE, Alignment>(p, x); }
template<size_t Alignment> static inline void writeU64xBE(void* p, uint64_t x) noexcept { writeU64x<kByteOrderBE, Alignment>(p, x); }

static inline void writeI16a(void* p, int32_t x) noexcept { writeI16x<kByteOrderNative, 2>(p, x); }
static inline void writeI16u(void* p, int32_t x) noexcept { writeI16x<kByteOrderNative, 1>(p, x); }
static inline void writeU16a(void* p, uint32_t x) noexcept { writeU16x<kByteOrderNative, 2>(p, x); }
static inline void writeU16u(void* p, uint32_t x) noexcept { writeU16x<kByteOrderNative, 1>(p, x); }

static inline void writeI16aLE(void* p, int32_t x) noexcept { writeI16xLE<2>(p, x); }
static inline void writeI16uLE(void* p, int32_t x) noexcept { writeI16xLE<1>(p, x); }
static inline void writeU16aLE(void* p, uint32_t x) noexcept { writeU16xLE<2>(p, x); }
static inline void writeU16uLE(void* p, uint32_t x) noexcept { writeU16xLE<1>(p, x); }

static inline void writeI16aBE(void* p, int32_t x) noexcept { writeI16xBE<2>(p, x); }
static inline void writeI16uBE(void* p, int32_t x) noexcept { writeI16xBE<1>(p, x); }
static inline void writeU16aBE(void* p, uint32_t x) noexcept { writeU16xBE<2>(p, x); }
static inline void writeU16uBE(void* p, uint32_t x) noexcept { writeU16xBE<1>(p, x); }

static inline void writeU24uLE(void* p, uint32_t v) noexcept { writeU24u<kByteOrderLE>(p, v); }
static inline void writeU24uBE(void* p, uint32_t v) noexcept { writeU24u<kByteOrderBE>(p, v); }

static inline void writeI32a(void* p, int32_t x) noexcept { writeI32x<kByteOrderNative, 4>(p, x); }
static inline void writeI32u(void* p, int32_t x) noexcept { writeI32x<kByteOrderNative, 1>(p, x); }
static inline void writeU32a(void* p, uint32_t x) noexcept { writeU32x<kByteOrderNative, 4>(p, x); }
static inline void writeU32u(void* p, uint32_t x) noexcept { writeU32x<kByteOrderNative, 1>(p, x); }

static inline void writeI32aLE(void* p, int32_t x) noexcept { writeI32xLE<4>(p, x); }
static inline void writeI32uLE(void* p, int32_t x) noexcept { writeI32xLE<1>(p, x); }
static inline void writeU32aLE(void* p, uint32_t x) noexcept { writeU32xLE<4>(p, x); }
static inline void writeU32uLE(void* p, uint32_t x) noexcept { writeU32xLE<1>(p, x); }

static inline void writeI32aBE(void* p, int32_t x) noexcept { writeI32xBE<4>(p, x); }
static inline void writeI32uBE(void* p, int32_t x) noexcept { writeI32xBE<1>(p, x); }
static inline void writeU32aBE(void* p, uint32_t x) noexcept { writeU32xBE<4>(p, x); }
static inline void writeU32uBE(void* p, uint32_t x) noexcept { writeU32xBE<1>(p, x); }

static inline void writeI64a(void* p, int64_t x) noexcept { writeI64x<kByteOrderNative, 8>(p, x); }
static inline void writeI64u(void* p, int64_t x) noexcept { writeI64x<kByteOrderNative, 1>(p, x); }
static inline void writeU64a(void* p, uint64_t x) noexcept { writeU64x<kByteOrderNative, 8>(p, x); }
static inline void writeU64u(void* p, uint64_t x) noexcept { writeU64x<kByteOrderNative, 1>(p, x); }

static inline void writeI64aLE(void* p, int64_t x) noexcept { writeI64xLE<8>(p, x); }
static inline void writeI64uLE(void* p, int64_t x) noexcept { writeI64xLE<1>(p, x); }
static inline void writeU64aLE(void* p, uint64_t x) noexcept { writeU64xLE<8>(p, x); }
static inline void writeU64uLE(void* p, uint64_t x) noexcept { writeU64xLE<1>(p, x); }

static inline void writeI64aBE(void* p, int64_t x) noexcept { writeI64xBE<8>(p, x); }
static inline void writeI64uBE(void* p, int64_t x) noexcept { writeI64xBE<1>(p, x); }
static inline void writeU64aBE(void* p, uint64_t x) noexcept { writeU64xBE<8>(p, x); }
static inline void writeU64uBE(void* p, uint64_t x) noexcept { writeU64xBE<1>(p, x); }

// TODO: Remove.
static inline bool inlinedEq(const void* _a, const void* _b, size_t size) noexcept {
  return std::memcmp(_a, _b, size) == 0;
}

static inline bool inlinedEq1(const void* _a, const void* _b) noexcept {
  return (((uint8_t  *)_a)[0] == ((uint8_t  *)_b)[0]);
}

static inline bool inlinedEq2(const void* _a, const void* _b) noexcept {
  return (((uint16_t *)_a)[0] == ((uint16_t *)_b)[0]);
}

static inline bool inlinedEq4(const void* _a, const void* _b) noexcept {
  return (((uint32_t *)_a)[0] == ((uint32_t *)_b)[0]);
}

static inline bool inlinedEq8(const void* _a, const void* _b) noexcept {
  #if B2D_ARCH_BITS >= 64
    return (((uint64_t *)_a)[0] == ((uint64_t *)_b)[0]);
  #else
    return (((uint32_t *)_a)[0] == ((uint32_t *)_b)[0]) &
           (((uint32_t *)_a)[1] == ((uint32_t *)_b)[1]) ;
  #endif
}

static inline bool inlinedEq16(const void* _a, const void* _b) noexcept {
  #if B2D_ARCH_BITS >= 64
  return (((uint64_t *)_a)[0] == ((uint64_t *)_b)[0]) &
         (((uint64_t *)_a)[1] == ((uint64_t *)_b)[1]) ;
  #elif B2D_ARCH_SSE2
  __m128i xmm0a = _mm_loadu_si128((__m128i*)(_a) + 0);
  __m128i xmm0b = _mm_loadu_si128((__m128i*)(_b) + 0);

  xmm0a = _mm_cmpeq_epi8(xmm0a, xmm0b);
  return _mm_movemask_epi8(xmm0a) == 0xFFFF;
  #else
  return (((uint32_t *)_a)[0] == ((uint32_t *)_b)[0]) &
          (((uint32_t *)_a)[1] == ((uint32_t *)_b)[1]) &
          (((uint32_t *)_a)[2] == ((uint32_t *)_b)[2]) &
          (((uint32_t *)_a)[3] == ((uint32_t *)_b)[3]) ;
  #endif
}

static inline bool inlinedEq32(const void* _a, const void* _b) noexcept {
  #if B2D_ARCH_SSE2
  __m128i xmm0a = _mm_loadu_si128((__m128i*)(_a) + 0);
  __m128i xmm1a = _mm_loadu_si128((__m128i*)(_a) + 1);
  __m128i xmm0b = _mm_loadu_si128((__m128i*)(_b) + 0);
  __m128i xmm1b = _mm_loadu_si128((__m128i*)(_b) + 1);

  xmm0a = _mm_cmpeq_epi8(xmm0a, xmm0b);
  xmm1a = _mm_cmpeq_epi8(xmm1a, xmm1b);
  xmm0a = _mm_and_si128(xmm0a, xmm1a);
  return _mm_movemask_epi8(xmm0a) == 0xFFFF;
  #else
  return inlinedEq16((const uint8_t*)(_a) +  0, (const uint8_t*)(_b) +  0) &
         inlinedEq16((const uint8_t*)(_a) + 16, (const uint8_t*)(_b) + 16) ;
  #endif
}

template<size_t N>
B2D_INLINE bool inlinedEqN(const void* _a, const void* _b) noexcept {
  size_t i;

  const uint8_t* a = reinterpret_cast<const uint8_t*>(_a);
  const uint8_t* b = reinterpret_cast<const uint8_t*>(_b);

  for (i = size_t(N / 32); i; i--) {
    if (!inlinedEq32(a, b))
      return false;

    a += 32;
    b += 32;
  }

  for (i = size_t(N % 32) / sizeof(size_t); i; i--) {
    if (reinterpret_cast<const size_t*>(a)[0] != reinterpret_cast<const size_t*>(b)[0])
      return false;

    a += sizeof(size_t);
    b += sizeof(size_t);
  }

  for (i = size_t(N % 4); i; i--) {
    if (a[0] != b[0])
      return false;
  }

  return true;
}

template<typename T>
B2D_INLINE bool inlinedEqT(const T* _dst, const T* _src) noexcept {
  return inlinedEqN<sizeof(T)>((const void*)(_dst), (const void*)(_src));
}

// ============================================================================
// [b2d::Support - Operators]
// ============================================================================

struct Set    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { B2D_UNUSED(x); return  y; } };
struct SetNot { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { B2D_UNUSED(x); return ~y; } };
struct And    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x &  y; } };
struct AndNot { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x & ~y; } };
struct NotAnd { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return ~x &  y; } };
struct Or     { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x |  y; } };
struct Xor    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x ^  y; } };
struct Add    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x +  y; } };
struct Sub    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return  x -  y; } };
struct Min    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return std::min<T>(x, y); } };
struct Max    { template<typename T> static B2D_INLINE T op(T x, T y) noexcept { return std::max<T>(x, y); } };

// ============================================================================
// [b2d::Support - BitWordIterator]
// ============================================================================

//! Iterates over each bit in a number which is set to 1.
//!
//! Example of use:
//!
//! ```
//! uint32_t bitsToIterate = 0x110F;
//! Support::BitWordIterator<uint32_t> it(bitsToIterate);
//!
//! while (it.hasNext()) {
//!   uint32_t bitIndex = it.next();
//!   std::printf("Bit at %u is set\n", unsigned(bitIndex));
//! }
//! ```
template<typename T>
class BitWordIterator {
public:
  constexpr explicit BitWordIterator(T bitWord) noexcept
    : _bitWord(bitWord) {}

  B2D_INLINE void init(T bitWord) noexcept { _bitWord = bitWord; }
  B2D_INLINE bool hasNext() const noexcept { return _bitWord != 0; }

  B2D_INLINE uint32_t next() noexcept {
    B2D_ASSERT(_bitWord != 0);
    uint32_t index = ctz(_bitWord);
    _bitWord ^= T(1u) << index;
    return index;
  }

  T _bitWord;
};

// ============================================================================
// [b2d::Support - BitVectorOps]
// ============================================================================

namespace Internal {
  template<typename T, class OperatorT, class FullWordOpT>
  static inline void bitVectorOp(T* buf, size_t index, size_t count) noexcept {
    if (count == 0)
      return;

    const size_t kTSizeInBits = bitSizeOf<T>();
    size_t vecIndex = index / kTSizeInBits; // T[]
    size_t bitIndex = index % kTSizeInBits; // T[][]

    buf += vecIndex;

    // The first BitWord requires special handling to preserve bits outside the fill region.
    const T kFillMask = allOnes<T>();
    size_t firstNBits = std::min<size_t>(kTSizeInBits - bitIndex, count);

    buf[0] = OperatorT::op(buf[0], (kFillMask >> (kTSizeInBits - firstNBits)) << bitIndex);
    buf++;
    count -= firstNBits;

    // All bits between the first and last affected BitWords can be just filled.
    while (count >= kTSizeInBits) {
      buf[0] = FullWordOpT::op(buf[0], kFillMask);
      buf++;
      count -= kTSizeInBits;
    }

    // The last BitWord requires special handling as well
    if (count)
      buf[0] = OperatorT::op(buf[0], kFillMask >> (kTSizeInBits - count));
  }
}

//! Set bit in a bit-vector `buf` at `index`.
template<typename T>
static B2D_INLINE bool bitVectorGetBit(T* buf, size_t index) noexcept {
  const size_t kTSizeInBits = bitSizeOf<T>();

  size_t vecIndex = index / kTSizeInBits;
  size_t bitIndex = index % kTSizeInBits;

  return bool((buf[vecIndex] >> bitIndex) & 0x1u);
}

//! Set bit in a bit-vector `buf` at `index` to `value`.
template<typename T>
static B2D_INLINE void bitVectorSetBit(T* buf, size_t index, bool value) noexcept {
  const size_t kTSizeInBits = bitSizeOf<T>();

  size_t vecIndex = index / kTSizeInBits;
  size_t bitIndex = index % kTSizeInBits;

  T bitMask = T(1u) << bitIndex;
  if (value)
    buf[vecIndex] |= bitMask;
  else
    buf[vecIndex] &= ~bitMask;
}

//! Set bit in a bit-vector `buf` at `index` to `value`.
template<typename T>
static B2D_INLINE void bitVectorFlipBit(T* buf, size_t index) noexcept {
  const size_t kTSizeInBits = bitSizeOf<T>();

  size_t vecIndex = index / kTSizeInBits;
  size_t bitIndex = index % kTSizeInBits;

  T bitMask = T(1u) << bitIndex;
  buf[vecIndex] ^= bitMask;
}

//! Fill `count` bits in bit-vector `buf` starting at bit-index `index`.
template<typename T>
static B2D_INLINE void bitVectorFill(T* buf, size_t index, size_t count) noexcept { Internal::bitVectorOp<T, Or, Set>(buf, index, count); }

//! Clear `count` bits in bit-vector `buf` starting at bit-index `index`.
template<typename T>
static B2D_INLINE void bitVectorClear(T* buf, size_t index, size_t count) noexcept { Internal::bitVectorOp<T, AndNot, SetNot>(buf, index, count); }

// ============================================================================
// [b2d::Support - BitVectorIterator]
// ============================================================================

template<typename T>
class BitVectorIterator {
public:
  B2D_INLINE BitVectorIterator(const T* data, size_t numBitWords, size_t start = 0) noexcept {
    init(data, numBitWords, start);
  }

  B2D_INLINE void init(const T* data, size_t numBitWords, size_t start = 0) noexcept {
    const T* ptr = data + (start / bitSizeOf<T>());
    size_t idx = alignDown(start, bitSizeOf<T>());
    size_t end = numBitWords * bitSizeOf<T>();

    T bitWord = T(0);
    if (idx < end) {
      bitWord = *ptr++ & (allOnes<T>() << (start % bitSizeOf<T>()));
      while (!bitWord && (idx += bitSizeOf<T>()) < end)
        bitWord = *ptr++;
    }

    _ptr = ptr;
    _idx = idx;
    _end = end;
    _current = bitWord;
  }

  B2D_INLINE bool hasNext() const noexcept {
    return _current != T(0);
  }

  B2D_INLINE size_t next() noexcept {
    T bitWord = _current;
    B2D_ASSERT(bitWord != T(0));

    uint32_t bit = ctz(bitWord);
    bitWord ^= T(1u) << bit;

    size_t n = _idx + bit;
    while (!bitWord && (_idx += bitSizeOf<T>()) < _end)
      bitWord = *_ptr++;

    _current = bitWord;
    return n;
  }

  B2D_INLINE size_t peekNext() const noexcept {
    B2D_ASSERT(_current != T(0));
    return _idx + ctz(_current);
  }

  const T* _ptr;
  size_t _idx;
  size_t _end;
  T _current;
};

// ============================================================================
// [b2d::Support - BitVectorFlipIterator]
// ============================================================================

template<typename T>
class BitVectorFlipIterator {
public:
  B2D_INLINE BitVectorFlipIterator(const T* data, size_t numBitWords, size_t start = 0, T xorMask = 0) noexcept {
    init(data, numBitWords, start, xorMask);
  }

  B2D_INLINE void init(const T* data, size_t numBitWords, size_t start = 0, T xorMask = 0) noexcept {
    const T* ptr = data + (start / bitSizeOf<T>());
    size_t idx = alignDown(start, bitSizeOf<T>());
    size_t end = numBitWords * bitSizeOf<T>();

    T bitWord = T(0);
    if (idx < end) {
      bitWord = (*ptr++ ^ xorMask) & (allOnes<T>() << (start % bitSizeOf<T>()));
      while (!bitWord && (idx += bitSizeOf<T>()) < end)
        bitWord = *ptr++ ^ xorMask;
    }

    _ptr = ptr;
    _idx = idx;
    _end = end;
    _current = bitWord;
    _xorMask = xorMask;
  }

  B2D_INLINE bool hasNext() const noexcept {
    return _current != T(0);
  }

  B2D_INLINE size_t next() noexcept {
    T bitWord = _current;
    B2D_ASSERT(bitWord != T(0));

    uint32_t bit = ctz(bitWord);
    bitWord ^= T(1u) << bit;

    size_t n = _idx + bit;
    while (!bitWord && (_idx += bitSizeOf<T>()) < _end)
      bitWord = *_ptr++ ^ _xorMask;

    _current = bitWord;
    return n;
  }

  B2D_INLINE size_t nextAndFlip() noexcept {
    T bitWord = _current;
    B2D_ASSERT(bitWord != T(0));

    uint32_t bit = ctz(bitWord);
    bitWord ^= allOnes<T>() << bit;
    _xorMask ^= allOnes<T>();

    size_t n = _idx + bit;
    while (!bitWord && (_idx += bitSizeOf<T>()) < _end)
      bitWord = *_ptr++ ^ _xorMask;

    _current = bitWord;
    return n;
  }

  B2D_INLINE size_t peekNext() const noexcept {
    B2D_ASSERT(_current != T(0));
    return _idx + ctz(_current);
  }

  const T* _ptr;
  size_t _idx;
  size_t _end;
  T _current;
  T _xorMask;
};

// ============================================================================
// [b2d::Support - Sorting]
// ============================================================================

enum SortOrder : uint32_t {
  kSortAscending  = 0,
  kSortDescending = 1
};

//! A helper class that provides comparison of any user-defined type that
//! implements `<` and `>` operators (primitive types are supported as well).
template<uint32_t Order = kSortAscending>
struct Compare {
  template<typename A, typename B>
  B2D_INLINE int operator()(const A& a, const B& b) const noexcept {
    return (Order == kSortAscending) ? (a < b ? -1 : a > b ?  1 : 0)
                                     : (a < b ?  1 : a > b ? -1 : 0);
  }
};

//! Insertion sort.
template<typename T, typename CompareT = Compare<kSortAscending>>
static B2D_INLINE void iSort(T* base, size_t size, const CompareT& cmp = CompareT()) noexcept {
  for (T* pm = base + 1; pm < base + size; pm++)
    for (T* pl = pm; pl > base && cmp(pl[-1], pl[0]) > 0; pl--)
      std::swap(pl[-1], pl[0]);
}

namespace Internal {
  //! Quick-sort implementation.
  template<typename T, class CompareT>
  struct QSortImpl {
    static constexpr size_t kStackSize = 64 * 2;
    static constexpr size_t kISortThreshold = 7;

    // Based on "PDCLib - Public Domain C Library" and rewritten to C++.
    static void sort(T* base, size_t size, const CompareT& cmp) noexcept {
      T* end = base + size;
      T* stack[kStackSize];
      T** stackptr = stack;

      for (;;) {
        if ((size_t)(end - base) > kISortThreshold) {
          // We work from second to last - first will be pivot element.
          T* pi = base + 1;
          T* pj = end - 1;
          std::swap(base[(size_t)(end - base) / 2], base[0]);

          if (cmp(*pi  , *pj  ) > 0) std::swap(*pi  , *pj  );
          if (cmp(*base, *pj  ) > 0) std::swap(*base, *pj  );
          if (cmp(*pi  , *base) > 0) std::swap(*pi  , *base);

          // Now we have the median for pivot element, entering main loop.
          for (;;) {
            while (pi < pj   && cmp(*++pi, *base) < 0) continue; // Move `i` right until `*i >= pivot`.
            while (pj > base && cmp(*--pj, *base) > 0) continue; // Move `j` left  until `*j <= pivot`.

            if (pi > pj) break;
            std::swap(*pi, *pj);
          }

          // Move pivot into correct place.
          std::swap(*base, *pj);

          // Larger subfile base / end to stack, sort smaller.
          if (pj - base > end - pi) {
            // Left is larger.
            *stackptr++ = base;
            *stackptr++ = pj;
            base = pi;
          }
          else {
            // Right is larger.
            *stackptr++ = pi;
            *stackptr++ = end;
            end = pj;
          }
          B2D_ASSERT(stackptr <= stack + kStackSize);
        }
        else {
          iSort(base, (size_t)(end - base), cmp);
          if (stackptr == stack)
            break;
          end = *--stackptr;
          base = *--stackptr;
        }
      }
    }
  };
}

//! Quick sort.
template<typename T, class CompareT = Compare<kSortAscending>>
static inline void qSort(T* base, size_t size, const CompareT& cmp = CompareT()) noexcept {
  Internal::QSortImpl<T, CompareT>::sort(base, size, cmp);
}

// ============================================================================
// [b2d::Support::Temporary]
// ============================================================================

//! Used to pass a temporary buffer to:
//!
//!   - Containers that use user-passed buffer as an initial storage (still can grow).
//!   - Zone allocator that would use the temporary buffer as a first block.
struct Temporary {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  constexpr Temporary(const Temporary& other) noexcept = default;
  constexpr Temporary(void* data, size_t size) noexcept
    : _data(data),
      _size(size) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the storage.
  template<typename T = void>
  constexpr T* data() const noexcept { return static_cast<T*>(_data); }
  //! Get the storage size (capacity).
  constexpr size_t size() const noexcept { return _size; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline Temporary& operator=(const Temporary& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  void* _data;
  size_t _size;
};

} // Support namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_SUPPORT_H
