// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/container.h"
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::bDataGrow]
// ============================================================================

size_t bDataGrow(size_t headerSize, size_t before, size_t after, size_t szT) {
  // Threshold for excessive growing. If size of data in memory is larger
  // than the threshold, grow will be constant instead of doubling the size.
  const size_t minThreshold = 128;
  const size_t maxThreshold = 1024 * 1024 * 8;

  B2D_ASSERT(before <= after);

  size_t beforeSize = headerSize + before * szT;
  size_t afterSize = headerSize + after * szT;
  size_t optimal;

  if (afterSize < minThreshold) {
    optimal = minThreshold;
  }
  else if (beforeSize < maxThreshold && afterSize < maxThreshold) {
    optimal = beforeSize;
    while (optimal < afterSize)
      optimal += ((optimal + 2) >> 1);
  }
  else {
    optimal = Math::max(beforeSize + maxThreshold, afterSize);
  }

  optimal = (optimal + 15) & ~15;
  optimal = ((optimal - headerSize) / szT);

  B2D_ASSERT(optimal >= after);
  return optimal;
}

B2D_END_NAMESPACE
