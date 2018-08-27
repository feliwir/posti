// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHGRADIENTPART_P_H
#define _B2D_PIPE2D_FETCHGRADIENTPART_P_H

// [Dependencies]
#include "./fetchpart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchGradientPart]
// ============================================================================

//! Base class for all gradient fetch parts.
class FetchGradientPart : public FetchPart {
public:
  B2D_NONCOPYABLE(FetchGradientPart)

  struct CommonRegs {
    x86::Gp table;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchGradientPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get gradient extend.
  inline uint32_t extend() const noexcept { return _extend; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _extend;
};

// ============================================================================
// [b2d::Pipe2D::FetchGradientLinearPart]
// ============================================================================

//! Linear gradient fetch part.
class FetchGradientLinearPart : public FetchGradientPart {
public:
  B2D_NONCOPYABLE(FetchGradientLinearPart)

  struct LinearRegs : public CommonRegs {
    x86::Xmm pt;
    x86::Xmm dt;
    x86::Xmm dt2;
    x86::Xmm py;
    x86::Xmm dy;
    x86::Xmm rep;
    x86::Xmm msk;
    x86::Xmm vIdx;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchGradientLinearPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline bool isPad() const noexcept { return !_isRoR; }
  inline bool isRoR() const noexcept { return  _isRoR; }

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void _initPart(x86::Gp& x, x86::Gp& y) noexcept override;
  void _finiPart() noexcept override;

  // --------------------------------------------------------------------------
  // [Advance]
  // --------------------------------------------------------------------------

  void advanceY() noexcept override;
  void startAtX(x86::Gp& x) noexcept override;
  void advanceX(x86::Gp& x, x86::Gp& diff) noexcept override;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void prefetch1() noexcept override;
  void fetch1(PixelARGB& p, uint32_t flags) noexcept override;

  void enterN() noexcept override;
  void leaveN() noexcept override;
  void prefetchN() noexcept override;
  void postfetchN() noexcept override;

  void fetch4(PixelARGB& p, uint32_t flags) noexcept override;
  void fetch8(PixelARGB& p, uint32_t flags) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Wrap<LinearRegs> f;
  bool _isRoR;
};

// ============================================================================
// [b2d::Pipe2D::FetchGradientRadialPart]
// ============================================================================

//! Radial gradient fetch part.
class FetchGradientRadialPart : public FetchGradientPart {
public:
  B2D_NONCOPYABLE(FetchGradientRadialPart)

  // `d`   - determinant.
  // `dd`  - determinant delta.
  // `ddd` - determinant-delta delta.
  struct RadialRegs : public CommonRegs {
    x86::Xmm xx_xy;
    x86::Xmm yx_yy;

    x86::Xmm ax_ay;
    x86::Xmm fx_fy;
    x86::Xmm da_ba;

    x86::Xmm d_b;
    x86::Xmm dd_bd;
    x86::Xmm ddx_ddy;

    x86::Xmm px_py;
    x86::Xmm scale;
    x86::Xmm ddd;
    x86::Xmm value;

    x86::Gp maxi;
    x86::Xmm vmaxi; // Maximum table index, basically `precision - 1` (mask).
    x86::Xmm vmaxf; // Like `vmaxi`, but converted to `float`.

    // These are only used by `radialFetch4()` and restored by `radialLeaveN()`.
    x86::Xmm d_b_prev;
    x86::Xmm dd_bd_prev;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchGradientRadialPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void _initPart(x86::Gp& x, x86::Gp& y) noexcept override;
  void _finiPart() noexcept override;

  // --------------------------------------------------------------------------
  // [Advance]
  // --------------------------------------------------------------------------

  void advanceY() noexcept override;
  void startAtX(x86::Gp& x) noexcept override;
  void advanceX(x86::Gp& x, x86::Gp& diff) noexcept override;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void prefetch1() noexcept override;
  void fetch1(PixelARGB& p, uint32_t flags) noexcept override;

  void prefetchN() noexcept override;
  void postfetchN() noexcept override;
  void fetch4(PixelARGB& p, uint32_t flags) noexcept override;

  void precalc(x86::Xmm& px_py) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Wrap<RadialRegs> f;
};

// ============================================================================
// [b2d::Pipe2D::FetchGradientConicalPart]
// ============================================================================

//! Conical gradient fetch part.
class FetchGradientConicalPart : public FetchGradientPart {
public:
  B2D_NONCOPYABLE(FetchGradientConicalPart)

  struct ConicalRegs : public CommonRegs {
    x86::Xmm xx_xy;
    x86::Xmm yx_yy;

    x86::Xmm hx_hy;
    x86::Xmm px_py;

    x86::Gp consts;

    // 4+ pixels.
    x86::Xmm xx4_xy4;
    x86::Xmm xx_0123;
    x86::Xmm xy_0123;

    // Temporary.
    x86::Xmm x0, x1, x2, x3, x4, x5;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchGradientConicalPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void _initPart(x86::Gp& x, x86::Gp& y) noexcept override;
  void _finiPart() noexcept override;

  // --------------------------------------------------------------------------
  // [Advance]
  // --------------------------------------------------------------------------

  void advanceY() noexcept override;
  void startAtX(x86::Gp& x) noexcept override;
  void advanceX(x86::Gp& x, x86::Gp& diff) noexcept override;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void fetch1(PixelARGB& p, uint32_t flags) noexcept override;

  void prefetchN() noexcept override;
  void fetch4(PixelARGB& p, uint32_t flags) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Wrap<ConicalRegs> f;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHGRADIENTPART_P_H
