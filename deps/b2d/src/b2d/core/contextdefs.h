// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_CONTEXTDEFS_H
#define _B2D_CORE_CONTEXTDEFS_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::kGeomArg]
// ============================================================================

// TODO: Rename, move...
enum kGeomArg {
  kGeomArgNone = 0,

  kGeomArgLine,
  kGeomArgArc,

  kGeomArgBox,
  kGeomArgIntBox,

  kGeomArgRect,
  kGeomArgIntRect,

  kGeomArgRound,
  kGeomArgCircle,
  kGeomArgEllipse,
  kGeomArgChord,
  kGeomArgPie,
  kGeomArgTriangle,

  kGeomArgPolyline,
  kGeomArgIntPolyline,

  kGeomArgPolygon,
  kGeomArgIntPolygon,

  kGeomArgArrayBox,
  kGeomArgArrayIntBox,

  kGeomArgArrayRect,
  kGeomArgArrayIntRect,

  kGeomArgRegion,
  kGeomArgPath2D,

  kGeomArgCount
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_CONTEXTDEFS_H
