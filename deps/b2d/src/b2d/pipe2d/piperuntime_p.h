// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPERUNTIME_P_H
#define _B2D_PIPE2D_PIPERUNTIME_P_H

// [Dependencies]
#include "./functioncache_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::PipeRuntime]
// ============================================================================

class PipeRuntime {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  PipeRuntime() noexcept;
  ~PipeRuntime() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline uint32_t optLevel() const noexcept { return _optLevel; }
  inline void setOptLevel(uint32_t optLevel) noexcept { _optLevel = optLevel; }

  inline uint32_t maxPixels() const noexcept { return _maxPixels; }
  inline void setMaxPixelStep(uint32_t value) noexcept { _maxPixels = value; }

  // --------------------------------------------------------------------------
  // [Pipelines]
  // --------------------------------------------------------------------------

  inline FillFunc getFunction(uint32_t signature) noexcept {
    FillFunc func = (FillFunc)_cache.get(signature);
    return func ? func : _compileAndStore(signature);
  }

  FillFunc _compileAndStore(uint32_t signature) noexcept;
  FillFunc _compileFunction(uint32_t signature) noexcept;

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static Wrap<PipeRuntime> _global;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  asmjit::JitRuntime _runtime;           //!< JIT runtime (stores JIT functions).
  Pipe2D::FunctionCache _cache;          //!< Function cache (caches JIT functions).
  size_t _pipelineCount;                 //!< Count of cached pipelines.

  uint32_t _optLevel;                    //!< Maximum optimization level, 0 if not set (debug).
  uint32_t _maxPixels;                   //!< Maximum pixels at a time, 0 if no limit (debug).

  #ifndef ASMJIT_DISABLE_LOGGING
  asmjit::FileLogger _logger;
  bool _enableLogger;
  #endif
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPERUNTIME_P_H
