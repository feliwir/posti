// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_UNIQUEIDGENERATOR_H
#define _B2D_CORE_UNIQUEIDGENERATOR_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::UniqueIdGenerator]
// ============================================================================

//! \internal
//!
//! A context that can be used to generate unique 64-bit IDs in a thread-safe
//! manner. It uses atomic operations to make the generation as fast as possible
//! and provides an implementation for both 32-bit and 64-bit hosts.
//!
//! The implementation choses a different startegy between 32-bit and 64-bit hosts.
//! On a 64-bit host the implementation always returns sequential IDs starting
//! from 1, on 32-bit host the implementation would always return a number which
//! is higher than the previous one, but it doesn't have to be sequential.
struct UniqueIdGenerator {
  #if B2D_ARCH_BITS < 64

  inline void reset() noexcept {
    _hi = 0;
    _lo = 0;
  }

  inline uint64_t next() noexcept {
    // This implementation doesn't always return an incrementing value as it's
    // not the point. The requirement is to never return the same value, so it
    // sacrifices one bit in `_lo` counter that would tell us to increment `_hi`
    // counter and try again.
    constexpr uint32_t kThresholdLo32 = 0x80000000u;

    for (;;) {
      uint32_t hiValue = _hi.load();
      uint32_t loValue = ++_lo;

      // This MUST support even cases when the thread executing this function
      // right now is terminated. When we reach the threshold we increment
      // `_hi`, which would contain a new HIGH value that will be used
      // immediately, then we remove the threshold mark from LOW value and try
      // to get a new LOW and HIGH values to return.
      if (B2D_UNLIKELY(loValue & kThresholdLo32)) {
        _hi++;

        // If the thread is interrupted here we only incremented the HIGH value.
        // In this case another thread that might call `generateUnique64BitId()`
        // would end up right here trying to clear `kThresholdLo32` from LOW
        // value as well, which is fine.
        _lo.fetch_and(uint32_t(~kThresholdLo32));
        continue;
      }

      return (uint64_t(hiValue) << 32) | loValue;
    }
  }

  std::atomic<uint32_t> _hi;
  std::atomic<uint32_t> _lo;

  #else

  inline void reset() noexcept { _counter = 0; }
  inline uint64_t next() noexcept { return ++_counter; }

  std::atomic<uint64_t> _counter;

  #endif
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_UNIQUEIDGENERATOR_H
