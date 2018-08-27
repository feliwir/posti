// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERGLOBALS_P_H
#define _B2D_RASTERENGINE_RASTERGLOBALS_P_H

// [Dependencies]
#include "../core/gradient.h"
#include "../core/matrix2d.h"
#include "../core/pattern.h"
#include "../pipe2d/compopinfo_p.h"
#include "../pipe2d/piperuntime_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class RasterWorker;

// ============================================================================
// [b2d::RasterEngine::Globals]
// ============================================================================

//! How many pixels are represented by a single bit of a `BitWord.
//!
//! This is a hardcoded value as it's required by both rasterizer and compositor.
//! Before establishing `4` the values [4, 8, 16, 32] were tested. Candidates
//! were `4` and `8` where `8` sometimes surpassed `4` in specific workloads,
//! but `4` was stable across all tests.
//!
//! In general increasing `kPixelsPerOneBit` would result in less memory
//! consumed by bit vectors, but would increase the work compositors have to
//! do to process cells produced by analytic rasterizer.
static constexpr uint32_t kPixelsPerOneBit = 4;

// TODO: Refactor this.
namespace SWGlobals {
  static B2D_INLINE uint32_t pixelFormatFromComponentFlags(uint32_t componentFlags) noexcept {
    return componentFlags == PixelFlags::kComponentRGB ? PixelFormat::kXRGB32
                                                       : PixelFormat::kPRGB32;
  }
}

// ============================================================================
// [b2d::RasterEngine::A8Consts]
// ============================================================================

//! 8-bit alpha contstants.
enum A8Consts : uint32_t {
  kA8Shift = 8,                    // 8.
  kA8Scale = 1 << kA8Shift,        // 256.
  kA8Mask  = kA8Scale - 1          // 255.
};

// ============================================================================
// [b2d::RasterEngine::FixedPoint]
// ============================================================================

//! Parametrized fixed-point used by Render engine.
//!
//! Usually represents a fixed point representation.
template<typename T>
struct FixedPoint {
  B2D_INLINE void set(T x, T y) noexcept {
    _x = x;
    _y = y;
  }

  constexpr T x() const noexcept { return _x; }
  constexpr T y() const noexcept { return _y; }

  T _x;
  T _y;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERGLOBALS_P_H
