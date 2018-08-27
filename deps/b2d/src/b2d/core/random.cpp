// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math.h"
#include "../core/random.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Random - Reset / Rewind]
// ============================================================================

void Random::reset(uint64_t seed) noexcept {
  // The number is arbitrary, it means nothing.
  constexpr uint64_t kZeroSeed = 0x1F0A2BE71D163FA0u;

  // Generate the state data by using splitmix64.
  _state._seed = seed;
  for (uint32_t i = 0; i < 2; i++) {
    seed += 0x9E3779B97F4A7C15u;
    uint64_t x = seed;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9u;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBu;
    x = (x ^ (x >> 31));
    _state._data[i] = x != 0 ? x : kZeroSeed;
  }
}

// ============================================================================
// [b2d::Random - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_random) {
  // Number of iterations for tests that use loop.
  enum { kCount = 1000000 };

  // Test whether the SIMD implementation matches the C++ one.
  #if B2D_SIMD_I
  {
    Random a, b;

    SIMD::I128 bLo = b.nextUInt64AsI128();
    SIMD::I128 bHi = SIMD::vswizi32<2, 3, 0, 1>(bLo);

    uint64_t aVal = a.nextUInt64();
    uint64_t bVal = (uint64_t(SIMD::vcvti128i32(bLo))      ) +
                    (uint64_t(SIMD::vcvti128i32(bHi)) << 32) ;

    EXPECT(aVal == bVal);
  }
  #endif

  // Test whether the random 32-bit integer is the HI part of the 64-bit one.
  {
    Random a, b;
    EXPECT(uint32_t(a.nextUInt64() >> 32) == b.nextUInt32());
  }

  // Test whether returned doubles satisfy [0..1) condition.
  {
    // Supply a low-entropy seed on purpose.
    Random rnd(3u);

    uint32_t below = 0;
    uint32_t above = 0;

    for (uint32_t i = 0; i < kCount; i++) {
      double x = rnd.nextDouble();
      below += x <  0.5;
      above += x >= 0.5;
      EXPECT(x >= 0.0 && x < 1.0);
    }
    INFO("Random numbers at [0.0, 0.5): %u of %u", below, kCount);
    INFO("Random numbers at [0.5, 1.0): %u of %u", above, kCount);
  }
}
#endif

B2D_END_NAMESPACE
