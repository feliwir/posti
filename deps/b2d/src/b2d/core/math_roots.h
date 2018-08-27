// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_MATH_ROOTS_H
#define _B2D_CORE_MATH_ROOTS_H

// [Dependencies]
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

namespace Math {

// ============================================================================
// [b2d::Math - Roots]
// ============================================================================

//! Solve a linear polynomial `Ax + B = 0` and store the result in `dst`.
//!
//! Returns the number of roots found within [tMin, tMax] - `0` to `1`.
static B2D_INLINE size_t linearRoots(
  double* dst,
  double a, double b,
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept {

  double x = -b / a;
  dst[0] = x;
  return size_t(x >= tMin && x <= tMax);
}

//! \overload
static B2D_INLINE size_t linearRoots(
  double* dst,
  const double poly[2],
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept {

  return linearRoots(dst, poly[0], poly[1], tMin, tMax);
}

//! Solve a quadratic polynomial `Ax^2 + Bx + C = 0` and store the result in `dst`.
//!
//! Returns the number of roots found within [tMin, tMax] - `0` to `2`.
//!
//! Resources:
//!   - http://stackoverflow.com/questions/4503849/quadratic-equation-in-ada/4504415#4504415
//!   - http://people.csail.mit.edu/bkph/articles/Quadratics.pdf
//!
//! The standard equation:
//!
//!   ```
//!   x0 = (-b + sqrt(delta)) / 2a
//!   x1 = (-b - sqrt(delta)) / 2a
//!   ```
//!
//! When 4*a*c < b*b, computing x0 involves subtracting close numbers, and makes
//! you lose accuracy, so use the following instead:
//!
//!   ```
//!   x0 = 2c / (-b - sqrt(delta))
//!   x1 = 2c / (-b + sqrt(delta))
//!   ```
//!
//! Which yields a better x0, but whose x1 has the same problem as x0 had above.
//! The correct way to compute the roots is therefore:
//!
//!   ```
//!   q  = -0.5 * (b + sign(b) * sqrt(delta))
//!   x0 = q / a
//!   x1 = c / q
//!   ```
//!
//! NOTE: This is a branchless version designed to be easily inlineable.
static B2D_INLINE size_t quadRoots(
  double* dst,
  double a, double b, double c,
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept {

  double d = max(b * b - 4.0 * a * c, 0.0);
  double s = std::sqrt(d);
  double q = -0.5 * (b + std::copysign(s, b));

  double t0 = q / a;
  double t1 = c / q;

  double x0 = min(t0, t1);
  double x1 = max(t1, t0);

  dst[0] = x0;
  size_t count = size_t((x0 >= tMin) & (x0 <= tMax));

  dst[count] = x1;
  count += size_t((x1 > x0) & (x1 >= tMin) && (x1 <= tMax));

  return count;
}

//! \overload
static B2D_INLINE size_t quadRoots(
  double* dst,
  const double poly[3],
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept {

  return quadRoots(dst, poly[0], poly[1], poly[2], tMin, tMax);
}

//! Solve a cubic polynomial and store the result in `dst`.
//!
//! Returns the number of roots found within [tMin, tMax] - `0` to `3`.
B2D_API size_t cubicRoots(
  double* dst,
  const double* poly,
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept;

//! \overload
static B2D_INLINE size_t cubicRoots(
  double* dst,
  double a,
  double b,
  double c,
  double d,
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept {

  double poly[4] = { a, b, c, d };
  return cubicRoots(dst, poly, tMin, tMax);
}

//! Solve an Nth degree polynomial and store the result in `dst`.
//!
//! Returns the number of roots found [tMin, tMax] - `0` to `n - 1`.
B2D_API size_t polyRoots(
  double* dst,
  const double* poly, int n,
  double tMin = std::numeric_limits<double>::lowest(),
  double tMax = std::numeric_limits<double>::max()) noexcept;

} // Math namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_MATH_ROOTS_H
