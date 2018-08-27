// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHPART_P_H
#define _B2D_PIPE2D_FETCHPART_P_H

// [Dependencies]
#include "./pipehelpers_p.h"
#include "./pipepart_p.h"
#include "./pipepixel_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchPart]
// ============================================================================

//! Pipeline fetch part.
class FetchPart : public PipePart {
public:
  B2D_NONCOPYABLE(FetchPart)

  enum : uint32_t {
    kUnlimitedMaxPixels = 64
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get fetch type.
  inline uint32_t fetchType() const noexcept { return _fetchType; }

  //! Get whether the fetch-type equals `ft`.
  inline bool isFetchType(uint32_t ft) const noexcept { return _fetchType == ft; }
  //! Get whether the fetch-type is between `first..last`, inclusive.
  inline bool isFetchType(uint32_t first, uint32_t last) const noexcept { return _fetchType >= first && _fetchType <= last; }

  //! Get whether the fetch-type is solid.
  inline bool isSolid() const noexcept { return isFetchType(kFetchTypeSolid); }

  //! Get whether the fetch-type is gradient.
  inline bool isGradient() const noexcept { return isFetchType(_kFetchTypeGradientFirst, _kFetchTypeGradientLast); }
  //! Get whether the fetch-type is linear gradient.
  inline bool isLinearGradient() const noexcept { return isFetchType(_kFetchTypeGradientLinearFirst, _kFetchTypeGradientLinearLast); }
  //! Get whether the fetch-type is radial gradient.
  inline bool isRadialGradient() const noexcept { return isFetchType(_kFetchTypeGradientRadialFirst, _kFetchTypeGradientRadialLast); }
  //! Get whether the fetch-type is conical gradient.
  inline bool isConicalGradient() const noexcept { return isFetchType(_kFetchTypeGradientConicalFirst, _kFetchTypeGradientConicalLast); }

  //! Get whether the fetch-type is texture.
  inline bool isTexture() const noexcept { return isFetchType(_kFetchTypeTextureFirst, _kFetchTypeTextureLast); }
  //! Get whether the fetch is the destination (special type).
  inline bool isPixelPtr() const noexcept { return isFetchType(kFetchTypePixelPtr); }

  //! Get source pixel format.
  inline uint32_t pixelFormat() const noexcept { return _pixelFormat; }
  //! Get source pixel format information.
  inline PixelFormatInfo pixelFormatInfo() const noexcept { return PixelFormatInfo::table[_pixelFormat]; }

  //! Get source bytes-per-pixel (only used when `isPattern()` is true).
  inline uint32_t bpp() const noexcept { return _bpp; }

  //! Get the maximum pixels the fetch part can fetch at a time.
  inline uint32_t maxPixels() const noexcept { return _maxPixels; }

  //! Get whether the fetched pixels contain RGB channels.
  inline bool hasRGB() const noexcept { return _hasRGB; }
  //! Get whether the fetched pixels contain Alpha channel.
  inline bool hasAlpha() const noexcept { return _hasAlpha; }

  //! Get whether the fetch is currently initialized for a rectangular fill.
  inline bool isRectFill() const noexcept { return _isRectFill; }
  //! Get pixel granularity passed to `FetchPath::init()`.
  inline uint32_t pixelGranularity() const noexcept { return _pixelGranularity; }

  inline bool isComplexFetch() const noexcept { return _isComplexFetch; }
  inline void setComplexFetch(bool value) noexcept { _isComplexFetch = value; }

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void init(x86::Gp& x, x86::Gp& y, uint32_t pixelGranularity) noexcept;
  void fini() noexcept;

  virtual void _initPart(x86::Gp& x, x86::Gp& y) noexcept;
  virtual void _finiPart() noexcept;

  // --------------------------------------------------------------------------
  // [Advance]
  // --------------------------------------------------------------------------

  //! Advance the current y coordinate by one pixel.
  virtual void advanceY() noexcept;

  //! Initialize the current horizontal cursor of the current scanline to `x`.
  //!
  //! NOTE: This initializer is generally called once per scanline to setup the
  //! current position by initializing it to `x`. The position is then advanced
  //! automatically by pixel fetchers and by `advanceX()`, which is used when
  //! there is a gap in the scanline that has to be skipped.
  virtual void startAtX(x86::Gp& x) noexcept;

  //! Advance the current x coordinate by `diff` pixels, the final x position
  //! after advance will be `x`. The fetcher can decide whether to use `x`,
  //! `diff`, or both.
  virtual void advanceX(x86::Gp& x, x86::Gp& diff) noexcept;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  //! Must be called before `fetch1()`.
  virtual void prefetch1() noexcept;

  //! Load 1 pixel to XMM register(s) in `p` and advance by 1.
  virtual void fetch1(PixelARGB& p, uint32_t flags) noexcept = 0;

  //! Called as a prolog before fetching multiple fixels at once. This must be
  //! called before any loop that would call `fetch4()` or `fetch8()` unless the
  //! fetcher is in a vector mode because of `pixelGranularity`.
  virtual void enterN() noexcept;

  //! Called as an epilog after fetching multiple fixels at once. This must be
  //! called after a loop that uses `fetch4()` or `fetch8()` unless the fetcher
  //! is in a vector mode because of `pixelGranularity`.
  virtual void leaveN() noexcept;

  //! Must be called before a loop that calls `fetch4()` or `fetch8()`. In some
  //! cases there will be some instructions placed between `prefetch` and `fetch`,
  //! which means that if the fetcher requires an expensive operation that has
  //! greater latency then it would be better to place that code into the prefetch
  //! area.
  virtual void prefetchN() noexcept;

  //! Cancels the effect of `prefetchN()` and also automatic prefetch that happens
  //! inside `fetch4()` or `fetch8()`. Must be called after a loop that calls
  //! `fetch4()`, `fetch8()`, or immediately after `prefetchN()` if no loop has
  //! been entered.
  virtual void postfetchN() noexcept;

  //! Fetch 4 pixels to XMM register(s) in `p` and advance by 4.
  virtual void fetch4(PixelARGB& p, uint32_t flags) noexcept = 0;

  //! Fetch 8 pixels to XMM register(s) in `p` and advance by 8.
  //!
  //! NOTE: The default implementation uses `fetch4()` twice.
  virtual void fetch8(PixelARGB& p, uint32_t flags) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _fetchType;                   //!< Fetch type.
  uint32_t _fetchExtra;                  //!< Fetch extra (different meaning for each fetch type).

  uint8_t _pixelFormat;                  //!< Source pixel format, see `PixelFormat::Id`.
  uint8_t _bpp;                          //!< Source bytes-per-pixel (only required by pattern fetcher).
  uint8_t _maxPixels;                    //!< Maximum pixel step that the fetcher can fetch at a time (0=unlimited).
  uint8_t _pixelGranularity;             //!< Pixel granularity passed to init().

  bool _hasRGB;                          //!< If the fetched pixels contain RGB channels.
  bool _hasAlpha;                        //!< If the fetched pixels contain alpha channel.

  bool _isRectFill;                      //!< Fetcher is in a rectangle fill mode, set and cleared by `init...()`.
  bool _isComplexFetch;                  //!< If the fetch-type is complex (used to limit the maximum number of pixels).
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHPART_P_H
