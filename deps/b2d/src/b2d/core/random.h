// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_RANDOM_H
#define _B2D_CORE_RANDOM_H

// [Dependencies]
#include "../core/globals.h"
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Random]
// ============================================================================

//! Simple pseudo random number generator.
//!
//! The current implementation uses a PRNG called `XORSHIFT+`, which has `64`
//! bit seed, `128` bits of state, and full period `2^128-1`.
//!
//! Based on a paper by Sebastiano Vigna:
//!   http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf
class Random {
public:
  // The constants used are the constants suggested as `23/18/5`.
  enum Steps : uint32_t {
    kStep1_SHL = 23,
    kStep2_SHR = 18,
    kStep3_SHR = 5
  };

  struct State {
    inline uint64_t seed() const noexcept { return _seed; }

    inline bool eq(const State& other) const noexcept {
      return _seed    == other._seed    &&
             _data[0] == other._data[0] &&
             _data[1] == other._data[1] ;
    }

    inline bool operator==(const State& other) const noexcept { return  eq(other); }
    inline bool operator!=(const State& other) const noexcept { return !eq(other); }

    uint64_t _seed;
    uint64_t _data[2];
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Random(uint64_t seed = 0) noexcept { reset(seed); }

  inline Random(const Random& other) noexcept = default;
  inline Random& operator=(const Random& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Get the initial seed of the PRNG.
  inline uint64_t initialSeed() const noexcept { return _state._seed; }

  //! Get the current PRNG state, including its seed.
  inline const State& state() const noexcept { return _state; }
  //! Set the current PRNG state, including its seed.
  inline void setState(const State& state) noexcept { _state = state; }

  //! Reset the PRNG and initialize it to the given `seed`.
  B2D_API void reset(uint64_t seed = 0) noexcept;
  //! Rewind the PRNG so it starts from the beginning.
  inline void rewind() noexcept { reset(_state._seed); }

  #if B2D_SIMD_I
  // High-performance SIMD implementation. Better utilizes CPU in 32-bit mode
  // and it's a better candidate for `nextDouble()` in general on X86 as it
  // returns a SIMD register, which is easier to convert to `double`.
  B2D_INLINE SIMD::I128 nextUInt64AsI128() noexcept {
    SIMD::I128 x = SIMD::vloadi128_64(&_state._data[0]);
    SIMD::I128 y = SIMD::vloadi128_64(&_state._data[1]);

    x = SIMD::vxor(x, SIMD::vslli64<kStep1_SHL>(x));
    y = SIMD::vxor(y, SIMD::vsrli64<kStep3_SHR>(y));
    x = SIMD::vxor(x, SIMD::vsrli64<kStep2_SHR>(x));
    x = SIMD::vxor(x, y);

    SIMD::vstorei64(&_state._data[0], y);
    SIMD::vstorei64(&_state._data[1], x);

    return SIMD::vaddi64(x, y);
  }
  #endif

  #if B2D_SIMD_I && B2D_ARCH_BITS == 32
  B2D_INLINE uint64_t nextUInt64() noexcept {
    return SIMD::vcvti128u64(nextUInt64AsI128());
  }
  #else
  B2D_INLINE uint64_t nextUInt64() noexcept {
    uint64_t x = _state._data[0];
    uint64_t y = _state._data[1];

    x ^= x << kStep1_SHL;
    y ^= y >> kStep3_SHR;
    x ^= x >> kStep2_SHR;
    x ^= y;

    _state._data[0] = y;
    _state._data[1] = x;

    return x + y;
  }
  #endif

  B2D_INLINE uint32_t nextUInt32() noexcept {
    #if B2D_SIMD_I && B2D_ARCH_BITS == 32
    return SIMD::vcvti128u32(SIMD::vswizi32<2, 3, 0, 1>(nextUInt64AsI128()));
    #else
    return uint32_t(nextUInt64() >> 32);
    #endif
  }

  B2D_INLINE double nextDouble() noexcept {
    // Number of bits needed to shift right to extract mantissa.
    enum { kMantissaShift = 64 - 52 };

    // Construct a double at range [1..2) and normalize to [0..1).
    #if B2D_SIMD_I

    static constexpr SIMD::Const128<uint64_t> kExponentMask = {{ 0x3FF0000000000000u, 0x3FF0000000000000u }};
    SIMD::I128 x = SIMD::vor(SIMD::vsrli64<kMantissaShift>(nextUInt64AsI128()), kExponentMask.i128);
    return SIMD::vcvtd128d64(SIMD::vcast<SIMD::D128>(x)) - 1.0;

    #else

    uint64_t kExponentMask = 0x3FF0000000000000u;
    uint64_t u = (nextUInt64() >> kMantissaShift) | kExponentMask;
    return DoubleBits::fromUInt(u).d - 1.0;

    #endif
  }

  inline bool eq(const Random& other) const noexcept { return _state.eq(other._state); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline bool operator==(const Random& other) const noexcept { return  eq(other); }
  inline bool operator!=(const Random& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  State _state;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_RANDOM_H
