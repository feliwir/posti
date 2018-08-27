// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_EDGESTORAGE_P_H
#define _B2D_RASTERENGINE_EDGESTORAGE_P_H

// [Dependencies]
#include "../core/support.h"
#include "../rasterengine/rasterglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::EdgeVector]
// ============================================================================

// NOTE: This struct must be aligned to 8 bytes and pts[] array as well. That's
// the main reason we have a different representation for both 32-bit and 64-bit
// targets.
template<typename CoordT>
struct alignas(8) EdgeVector {
  static constexpr uint32_t minSizeOf() noexcept {
    return uint32_t(sizeof(EdgeVector<CoordT>) + sizeof(FixedPoint<CoordT>));
  }

#if B2D_ARCH_BITS >= 64
  EdgeVector<CoordT>* next;
  uint64_t signBit : 1;
  uint64_t count : 63;
  FixedPoint<CoordT> pts[1];
#else
  EdgeVector<CoordT>* next;
  uint32_t signBit : 1;
  uint32_t count : 31;
  FixedPoint<CoordT> pts[1];
#endif
};

// ============================================================================
// [b2d::EdgeStorage]
// ============================================================================

template<typename CoordT>
class EdgeStorage {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE EdgeStorage() noexcept { reset(); }
  B2D_INLINE EdgeStorage(const EdgeStorage& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _bandCount = 0;
    _bandCapacity = 0;
    _bandHeight = 0;
    _fixedBandHeightShift = 0;
    _bandEdges = 0;
    resetBoundingBox();
  }

  B2D_INLINE void clear() noexcept {
    if (!empty()) {
      uint32_t bandStart = (unsigned(_boundingBox.y0()    ) >> _fixedBandHeightShift);
      uint32_t bandEnd   = (unsigned(_boundingBox.y1() - 1) >> _fixedBandHeightShift) + 1;

      for (uint32_t i = bandStart; i < bandEnd; i++)
        _bandEdges[i] = nullptr;
      resetBoundingBox();
    }
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept {
    return _boundingBox._y0 == std::numeric_limits<int>::max();
  }

  B2D_INLINE uint32_t bandCount() const noexcept { return _bandCount; }
  B2D_INLINE uint32_t bandCapacity() const noexcept { return _bandCapacity; }
  B2D_INLINE uint32_t bandHeight() const noexcept { return _bandHeight; }
  B2D_INLINE uint32_t fixedBandHeightShift() const noexcept { return _fixedBandHeightShift; }

  B2D_INLINE EdgeVector<CoordT>** bandEdges() const noexcept { return _bandEdges; }
  B2D_INLINE const IntBox& boundingBox() const noexcept { return _boundingBox; }

  B2D_INLINE void setBandEdges(EdgeVector<CoordT>** edges, uint32_t capacity) noexcept {
    _bandEdges = edges;
    _bandCapacity = capacity;
  }

  B2D_INLINE void setBandHeight(uint32_t bandHeightInPixels) noexcept {
    _bandHeight = bandHeightInPixels;
    _fixedBandHeightShift = Support::ctz(bandHeightInPixels) + kA8Shift;
  }

  B2D_INLINE void resetBoundingBox() noexcept {
    _boundingBox.reset(std::numeric_limits<int>::max(),
                       std::numeric_limits<int>::max(),
                       std::numeric_limits<int>::min(),
                       std::numeric_limits<int>::min());
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _bandCount;                   //!< Length of `_bandEdges` array.
  uint32_t _bandCapacity;                //!< Capacity of `_bandEdges` array.
  uint32_t _bandHeight;                  //!< Height of a single band (in pixels).
  uint32_t _fixedBandHeightShift;        //!< Shift to get a bandId from a fixed-point y coordinate.
  EdgeVector<CoordT>** _bandEdges;       //!< Edges per each band (only used if banding is enabled).
  IntBox _boundingBox;                   //!< Bounding box in fixed-point.
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_EDGESTORAGE_P_H
