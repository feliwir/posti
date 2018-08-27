// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/regioninternal_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Region - IsBoxListSorted / IsRectListSorted]
// ============================================================================

bool bRegionIsBoxListSorted(const IntBox* data, size_t size) {
  if (size <= 1)
    return true;

  int y0 = data[0].y0();
  int y1 = data[0].y1();
  int xb = data[0].x1();

  // Detect invalid box.
  if (xb >= data[0].x1() || y0 >= y1)
    return false;

  for (size_t i = 1; i < size; i++) {
    // Detect invalid box.
    if (data[i].x0() >= data[i].x1())
      return false;

    // Detect next band.
    if (data[i].y0() != y0 || data[i].y1() != y1) {
      // Detect invalid box.
      if (data[i].y0() >= data[i].y1())
        return false;

      // Detect invalid position (non-sorted).
      if (data[i].y0() < y1)
        return false;

      // Ok, prepare for a new band.
      y0 = data[i].y0();
      y1 = data[i].y1();
      xb = data[i].x1();
    }
    else {
      // Detect whether the current band advances the last one.
      if (data[i].x0() < xb)
        return false;
      xb = data[i].x1();
    }
  }

  return true;
}

bool bRegionIsRectListSorted(const IntRect* data, size_t size) {
  if (size <= 1)
    return true;

  int y = data[0].y();
  int h = data[0].h();
  int x = data[0].x() + data[0].w();

  // Detect invalid box.
  if (data[0].w() <= 0 || data[0].h() <= 0)
    return false;

  for (size_t i = 1; i < size; i++) {
    // Detect invalid box.
    if (data[i].w() <= 0)
      return false;

    // Detect next band.
    if (data[i].y() != y || data[i].h() != h) {
      // Detect invalid box.
      if (data[i].h() <= 0)
        return false;

      // Detect invalid position (non-sorted).
      if (data[i].y() < y + h)
        return false;

      // Ok, prepare for a new band.
      y = data[i].y();
      h = data[i].h();
      x = data[i].x() + data[i].w();
    }
    else {
      // Detect whether the current band advances the last one.
      if (data[i].x() < x)
        return false;
      x = data[i].x() + data[i].w();
    }
  }

  return true;
}

// ============================================================================
// [b2d::Region - GetClosestBox]
// ============================================================================

const IntBox* bRegionGetClosestBox(const IntBox* data, size_t size, int y) {
  B2D_ASSERT(data != nullptr);
  B2D_ASSERT(size > 0);

  if (size <= 16) {
    for (size_t i = 0; i < size; i++) {
      if (data[i].y0() >= y)
        return &data[i];
    }
    return nullptr;
  }
  else {
    const IntBox* box;

    if (data[0].y0() >= y && data[0].y1() < y)
      return data;
    if (data[size - 1].y1() <= y)
      return nullptr;

    for (size_t i = size; i != 0; i >>= 1) {
      box = data + (i >> 1);

      // Try match.
      if (y >= box->y0()) {
        if (y < box->y1())
          break;
        // Otherwise move left.
      }
      else if (box->y1() <= y) {
        // Move right.
        data = box + 1;
        i--;
      }
      // otherwise move left.
    }

    while (box[-1].y1() == box[0].y1())
      box--;
    return box;
  }
}

B2D_END_NAMESPACE
