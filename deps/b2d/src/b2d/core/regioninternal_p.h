// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_REGIONINTERNAL_P_H
#define _B2D_CORE_REGIONINTERNAL_P_H

// [Dependencies]
#include "../core/geomtypes.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::RegionUtil - IsBoxListSorted / IsRectListSorted]
// ============================================================================

//! \internal
bool bRegionIsBoxListSorted(const IntBox* data, size_t size);

//! \internal
bool bRegionIsRectListSorted(const IntRect* data, size_t size);

// ============================================================================
// [b2d::RegionUtil - GetClosestBox]
// ============================================================================

//! \internal
//!
//! Binary search for the first rectangle usable for a scanline @a y.
//!
//! @param data Array of YX sorted rectangles.
//! @param size Count of rectangles.
//! @param y The Y position to match
//!
//! If there are no rectangles which matches the Y coordinate and the last
//! @c rect.y1 is lower or equal to @a y, @c NULL is returned. If there
//! is rectangle where @c rect.y0 >= @a y then the first rectangle in the list
//! is returned.
//!
//! The @a base and @a size parameters come in the most cases from @c Region
//! instance, but can be constructed manually.
const IntBox* bRegionGetClosestBox(const IntBox* data, size_t size, int y);

// ============================================================================
// [b2d::RegionUtil - GetEndBand]
// ============================================================================

//! \internal
//!
//! Get the end band of current horizontal rectangle list.
static B2D_INLINE const IntBox* bRegionGetEndBand(const IntBox* data, const IntBox* end) {
  const IntBox* cur = data;
  int y0 = data[0]._y0;

  while (++cur != end && cur[0]._y0 == y0)
    continue;
  return cur;
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_REGIONINTERNAL_P_H
