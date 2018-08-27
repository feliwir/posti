// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_MATH_INTEGRATE_H
#define _B2D_CORE_MATH_INTEGRATE_H

// [Dependencies]
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

namespace Math {

// ============================================================================
// [b2d::Math - Integrate]
// ============================================================================

//! Integrate the given function `func` at interval `interval`.
B2D_API Error integrate(double& dst, EvaluateFunc func, void* funcData, double tMin, double tMax, int nSteps) noexcept;

} // Math namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_MATH_INTEGRATE_H
