// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_PATHFLATTEN_H
#define _B2D_CORE_PATHFLATTEN_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Flattener]
// ============================================================================

//! Flattener.
struct Flattener {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE explicit Flattener(double flatness) noexcept
    : _flatness(flatness),
      _flatnessSq(Math::pow2(flatness)) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE double flatness() const noexcept { return _flatness; }

  B2D_INLINE void setFlatness(double flatness) noexcept {
    _flatness = flatness;
    _flatnessSq = Math::pow2(flatness);
  }

  // --------------------------------------------------------------------------
  // [Flatten]
  // --------------------------------------------------------------------------

  B2D_API Error flattenPath(Path2D& dst, const Path2D& src) const;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _flatness;
  double _flatnessSq;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_PATHFLATTEN_H
