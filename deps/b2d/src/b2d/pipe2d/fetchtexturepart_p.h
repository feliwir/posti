// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHTEXTUREPART_P_H
#define _B2D_PIPE2D_FETCHTEXTUREPART_P_H

// [Dependencies]
#include "./fetchpart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchTexturePart]
// ============================================================================

//! Base class for all texture fetch parts.
class FetchTexturePart : public FetchPart {
public:
  B2D_NONCOPYABLE(FetchTexturePart)

  //! Common registers (used by all fetch types).
  struct CommonRegs {
    x86::Gp w;                           //!< Texture width (32-bit).
    x86::Gp h;                           //!< Texture height (32-bit).
    x86::Gp srctop;                      //!< Texture pixels (pointer to the first scanline).
    x86::Gp stride;                      //!< Texture stride.
    x86::Gp strideOrig;                  //!< Texture stride (original value, used by TextureSimple only).
    x86::Gp srcp0;                       //!< Pointer to the previous scanline and/or pixel (fractional).
    x86::Gp srcp1;                       //!< Pointer to the current scanline and/or pixel (aligned).
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchTexturePart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether the fetch-type is simple texture {axis-aligned or axis-unaligned}.
  inline bool isSimple() const noexcept { return isFetchType(_kFetchTypeTextureSimpleFirst, _kFetchTypeTextureSimpleLast); }
  //! Get whether the fetch-type is an affine pattern style.
  inline bool isAffine() const noexcept { return isFetchType(_kFetchTypeTextureAffineFirst, _kFetchTypeTextureAffineLast); }
};

// ============================================================================
// [b2d::Pipe2D::FetchTextureSimplePart]
// ============================================================================

//! Simple texture fetch part.
//!
//! Simple texture fetch doesn't do scaling or affine transformations, however,
//! can perform fractional pixel translation described as Fx and Fy values.
class FetchTextureSimplePart : public FetchTexturePart {
public:
  B2D_NONCOPYABLE(FetchTextureSimplePart)

  //! Aligned and fractional blits.
  struct SimpleRegs : public CommonRegs {
    x86::Gp x;                           //!< X position.
    x86::Gp y;                           //!< Y position (counter, decreases to zero).

    x86::Gp rx;                          //!< X repeat/reflect.
    x86::Gp ry;                          //!< Y repeat/reflect.

    x86::Gp xPadded;                     //!< X padded to [0-W) range.
    x86::Gp xOrigin;                     //!< X origin, assigned to `x` at the beginning of each scanline.
    x86::Gp xRestart;                    //!< X restart (used by scalar implementation, points to either -W or 0).

    x86::Xmm pixL;                       //!< Last loaded pixel (or combined pixel) of the first  (srcp0) scanline.

    x86::Xmm wb_wb;
    x86::Xmm wd_wd;
    x86::Xmm wa_wb;
    x86::Xmm wc_wd;

    // Only used by fetchN.
    x86::Xmm xVec4;                      //!< X position vector     `[  x, x+1, x+2, x+3]`.
    x86::Xmm xSet4;                      //!< X setup vector        `[  0,   1,   2,   3]`.
    x86::Xmm xInc4;                      //!< X increment vector    `[  4,   4,   4,   4]`.
    x86::Xmm xNrm4;                      //!< X normalize vector.
    x86::Xmm xMax4;                      //!< X maximum vector      `[max, max, max, max]`.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchTextureSimplePart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether the fetch-type is axis-aligned blit (no extend modes, no overflows)
  inline bool isAABlit() const noexcept { return isFetchType(kFetchTypeTextureAABlit); }
  //! Get whether the fetch-type is axis-aligned texture.
  inline bool isAATexture() const noexcept { return isFetchType(_kFetchTypeTextureAAFirst, _kFetchTypeTextureAALast); }
  //! Get whether the fetch-type is a "FracBi" pattern style.
  inline bool isAUTexture() const noexcept { return isFetchType(_kFetchTypeTextureAUFirst, _kFetchTypeTextureAULast); }
  //! Get whether the fetch-type is a "FracBiX" pattern style.
  inline bool isFxTexture() const noexcept { return isFetchType(_kFetchTypeTextureFxFirst, _kFetchTypeTextureFxLast); }
  //! Get whether the fetch-type is a "FracBiY" pattern style.
  inline bool isFyTexture() const noexcept { return isFetchType(_kFetchTypeTextureFyFirst, _kFetchTypeTextureFyLast); }
  //! Get whether the fetch-type is a "FracBiXY" pattern style.
  inline bool isFxFyTexture() const noexcept { return isFetchType(_kFetchTypeTextureFxFyFirst, _kFetchTypeTextureFxFyLast); }

  //! Get whether the fetch is pattern style that has fractional `x` or `x & y`.
  inline bool hasFracX() const noexcept { return isFxTexture() || isFxFyTexture(); }
  //! Get whether the fetch is pattern style that has fractional `y` or `x & y`.
  inline bool hasFracY() const noexcept { return isFyTexture() || isFxFyTexture(); }

  //! Get extend-x mode.
  inline uint32_t extendX() const noexcept { return _extendX; }

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

  void advanceXByOne() noexcept;
  void repeatOrReflectX() noexcept;
  void prefetchAccX() noexcept;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  // NOTE: We don't do prefetch here. Since the prefetch we need is the same
  // for `prefetch1()` and `prefetchN()` we always prefetch by `prefetchAccX()`
  // during `startAtX()` and `advanceX()`.

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

  uint8_t _extendX;
  Wrap<SimpleRegs> f;
};

// ============================================================================
// [b2d::Pipe2D::FetchTextureAffinePart]
// ============================================================================

//! Affine texture fetch part.
class FetchTextureAffinePart : public FetchTexturePart {
public:
  B2D_NONCOPYABLE(FetchTextureAffinePart)

  struct AffineRegs : public CommonRegs {
    x86::Xmm xx_xy;                      //!< Horizontal X/Y increments.
    x86::Xmm yx_yy;                      //!< Vertical X/Y increments.
    x86::Xmm tx_ty;
    x86::Xmm px_py;
    x86::Xmm ox_oy;
    x86::Xmm rx_ry;                      //!< Normalization after `px_py` gets out of bounds.
    x86::Xmm qx_qy;                      //!< Like `px_py` but one pixel ahead [fetch4].
    x86::Xmm xx2_xy2;                    //!< Advance twice (like `xx_xy`, but doubled) [fetch4].
    x86::Xmm minx_miny;                  //!< Pad minimum coords.
    x86::Xmm maxx_maxy;                  //!< Pad maximum coords.
    x86::Xmm tw_th;                      //!< Texture width and height as doubles.

    x86::Xmm vIdx;                       //!< Vector of texture indexes.
    x86::Xmm vAddrMul;                   //!< Vector containing multipliers for Y/X pairs.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchTextureAffinePart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline bool isAffineNn() const noexcept { return isFetchType(kFetchTypeTextureAffineNnAny) | isFetchType(kFetchTypeTextureAffineNnOpt); }
  inline bool isAffineBi() const noexcept { return isFetchType(kFetchTypeTextureAffineBiAny) | isFetchType(kFetchTypeTextureAffineBiOpt); }
  inline bool isOptimized() const noexcept { return isFetchType(kFetchTypeTextureAffineNnOpt) | isFetchType(kFetchTypeTextureAffineBiOpt); }

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

  void advancePxPy(x86::Xmm& px_py, const x86::Gp& i) noexcept;
  void normalizePxPy(x86::Xmm& px_py) noexcept;

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

  void clampVIdxMin32(x86::Xmm& dst, const x86::Xmm& a, const x86::Xmm& b) noexcept;
  void clampVIdxMax32(x86::Xmm& dst, const x86::Xmm& a, const x86::Xmm& b) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Wrap<AffineRegs> f;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHTEXTUREPART_P_H
