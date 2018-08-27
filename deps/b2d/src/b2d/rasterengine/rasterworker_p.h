// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERWORKER_P_H
#define _B2D_RASTERENGINE_RASTERWORKER_P_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/image.h"
#include "../core/path2d.h"
#include "../core/region.h"
#include "../core/zerobuffer_p.h"
#include "../core/zone.h"
#include "../pipe2d/piperuntime_p.h"
#include "../rasterengine/edgestorage_p.h"
#include "../rasterengine/rasterglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class RasterContext2DImpl;

// ============================================================================
// [b2d::RasterEngine::RasterWorker]
// ============================================================================

class RasterWorker {
public:
  B2D_NONCOPYABLE(RasterWorker)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  RasterWorker(RasterContext2DImpl* context) noexcept;
  ~RasterWorker() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  Error ensureEdgeStorage() noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  RasterContext2DImpl* _context;             //!< Link to 2D rendering context.
  Pipe2D::ContextData _ctxData;          //!< Context data.

  uint8_t _clipMode;                     //!< Clip mode.
  uint8_t _reserved[3];                  //!< Reserved.
  uint32_t _fullAlpha;                   //!< Full alpha value (256 or 65536).

  ImageBuffer _buffer;                   //!< Buffer.
  Path2D _tmpPath[3];                    //!< Temporary paths.

  Zone _workerZone;                      //!< Worker zone memory.
  ZeroBuffer _zeroMem;                   //!< Zeroed memory for analytic rasterizer.
  EdgeStorage<int> _edgeStorage;         //!< Worker edge storage (for next command).
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERWORKER_P_H
