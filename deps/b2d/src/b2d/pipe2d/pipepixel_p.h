// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPEPIXEL_P_H
#define _B2D_PIPE2D_PIPEPIXEL_P_H

// [Dependencies]
#include "./pipehelpers_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::PixelARGB]
// ============================================================================

//! [A]RGB32 pixel representation.
//!
//! Convention used to define and process pixel components:
//!
//!   - Prefixes:
//!     - "p"  - packed pixel(s) or component(s).
//!     - "u"  - unpacked pixel(s) or component(s).
//!
//!   - Components:
//!     - "c"  - Pixel components (ARGB).
//!     - "a"  - Pixel alpha values (A).
//!     - "ia" - Inverted pixel alpha values (IA).
//!     - "m"  - Mask (not part of the pixel itself, comes from a FillPart).
//!     - "im" - Mask (not part of the pixel itself, comes from a FillPart).
struct PixelARGB {
  //! Pixel flags.
  enum Flags {
    kPC          = 0x00000001u,          //!< Packed ARGB32 components stored in `pc`.
    kUC          = 0x00000002u,          //!< Unpacked ARGB32 components stored in `uc`.
    kUA          = 0x00000004u,          //!< Unpacked ALPHA8 stored in `ua`.
    kUIA         = 0x00000008u,          //!< Unpacked+Inverted ALPHA8 stored in `uia`.
    kAny         = 0x0000000Fu,          //!< Any of PC|UC|UA|UIA.

    kLastPartial = 0x40000000u,          //!< Last fetch in this scanline - `N-1` pixels is ok.
    kImmutable   = 0x80000000u           //!< Fetch read-only, registers won't be modified.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline PixelARGB() noexcept { reset(); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept {
    pc.reset();
    uc.reset();
    ua.reset();
    uia.reset();
    immutable = false;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  VecArray pc;                           //!< Packed ARGB32 pixel(s), maximum 8.
  VecArray uc;                           //!< Unpacked ARGB32 pixel(s), maximum 8.
  VecArray ua;                           //!< Unpacked/Expanded ARGB32 alpha components, maximum 8.
  VecArray uia;                          //!< Unpacked/Expanded ARGB32 inverted alpha components, maximum 8.
  bool immutable;                        //!< True if all members are immutable (solid fills).
};

// ============================================================================
// [b2d::Pipe2D::SolidPixelARGB]
// ============================================================================

struct SolidPixelARGB {
  inline SolidPixelARGB() noexcept { reset(); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept {
    px.reset();
    ux.reset();
    py.reset();
    uy.reset();
    m.reset();
    im.reset();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  x86::Vec px;                             //!< Packed pre-processed components, shown as "X" in equations.
  x86::Vec py;                             //!< Packed pre-processed components, shown as "Y" in equations.
  x86::Vec ux;                             //!< Unpacked pre-processed components, shown as "X" in equations.
  x86::Vec uy;                             //!< Unpacked pre-processed components, shown as "Y" in equations.
  x86::Vec m;                              //!< Const mask [0...256].
  x86::Vec im;                             //!< Inverted mask [0...256].
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPEPIXEL_P_H
