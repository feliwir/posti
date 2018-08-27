// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHPIXELPTRPART_P_H
#define _B2D_PIPE2D_FETCHPIXELPTRPART_P_H

// [Dependencies]
#include "./fetchpart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchPixelPtrPart]
// ============================================================================

//! Pipeline fetch pixel-pointer part.
class FetchPixelPtrPart : public FetchPart {
public:
  B2D_NONCOPYABLE(FetchPixelPtrPart)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FetchPixelPtrPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept;

  // --------------------------------------------------------------------------
  // [Initialization]
  // --------------------------------------------------------------------------

  //! Initialize the pixel pointer to `p`.
  inline void initPtr(const x86::Gp& p) noexcept { _ptr = p; }
  //! Get the pixel-pointer.
  inline x86::Gp& ptr() noexcept { return _ptr; }

  //! Get pixel-pointer alignment.
  inline uint32_t ptrAlignment() const noexcept { return _ptrAlignment; }
  //! Set pixel-pointer alignment.
  inline void setPtrAlignment(uint32_t alignment) noexcept { _ptrAlignment = uint8_t(alignment); }

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void fetch1(PixelARGB& p, uint32_t flags) noexcept override;
  void fetch4(PixelARGB& p, uint32_t flags) noexcept override;
  void fetch8(PixelARGB& p, uint32_t flags) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  x86::Gp _ptr;                          //!< Pixel pointer.
  uint8_t _ptrAlignment;                 //!< Pixel pointer alignment (updated by FillPart|CompOpPart).
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHPIXELPTRPART_P_H
