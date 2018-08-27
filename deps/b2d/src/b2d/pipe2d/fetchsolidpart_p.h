// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHSOLIDPART_P_H
#define _B2D_PIPE2D_FETCHSOLIDPART_P_H

// [Dependencies]
#include "./fetchpart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchSolidPart]
// ============================================================================

//! Pipeline solid-fetch part.
class FetchSolidPart : public FetchPart {
public:
  B2D_NONCOPYABLE(FetchSolidPart)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchSolidPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline bool isTransparent() const noexcept { return _isTransparent; }
  inline void setTransparent(bool value) noexcept { _isTransparent = value; }

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void _initPart(x86::Gp& x, x86::Gp& y) noexcept override;
  void _finiPart() noexcept override;

  // --------------------------------------------------------------------------
  // [InitSolidFlags]
  // --------------------------------------------------------------------------

  //! Injects code at the beginning of the pipeline that is required to prepare
  //! the requested variables that will be used by a special compositor that can
  //! composite the destination with solid pixels. Multiple calls to `prepareSolid()`
  //! are allowed and this feature is used to setup variables required by various
  //! parts of the pipeline.
  //!
  //! NOTE: Initialization means code injection, calling `prepareSolid()` will
  //! not emit any code at the current position, it will instead inject code to
  //! the position saved by `init()`.
  void initSolidFlags(uint32_t flags) noexcept;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void fetch1(PixelARGB& p, uint32_t flags) noexcept override;
  void fetch4(PixelARGB& p, uint32_t flags) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Source pixel, expanded to the whole register if necessary.
  PixelARGB _pixel;
  //! If the solid color is always transparent (set if clear operator is used).
  bool _isTransparent;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHSOLIDPART_P_H
