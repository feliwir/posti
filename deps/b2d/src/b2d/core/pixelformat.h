// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_PIXELFORMAT_H
#define _B2D_CORE_PIXELFORMAT_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::PixelFlags]
// ============================================================================

namespace PixelFlags {
  //! Pixel flags.
  enum Flags : uint32_t {
    kNoFlags              = 0x00000000u, //!< No flags.

    kComponentAlpha       = 0x00000001u, //!< Pixel has alpha component.
    kComponentRGB         = 0x00000002u, //!< Pixel has RGB components.
    kComponentARGB        = 0x00000003u, //!< Pixel has both RGB and alpha components.
    kComponentMask        = 0x00000003u, //!< Mask of all pixel component flags.

    kUnusedBits           = 0x00000010u, //!< Pixel has unused bits/bytes (XRGB32, for example).
    kPremultiplied        = 0x00000020u, //!< Pixel has RGB components premultiplied by Alpha.
    _kSolid               = 0x00000040u  //!< Only used by `Gradient` to mark solid transitions.
  };
}

// ============================================================================
// [b2d::PixelFormat]
// ============================================================================

//! Provides `PixelFormat::Id` enumeration.
namespace PixelFormat {
  //! Describes a pixel format.
  //!
  //! For most use cases you would only need to specify `PixelFormat::Id`.
  enum Id : uint32_t {
    //! Invalid pixel format (used only by empty or uninitialized images).
    kNone = 0,

    //! 32-bit ARGB (8-bit components, native byte-order, premultiplied).
    //!
    //! Pixel layout:
    //!
    //! ```
    //! +----------------+---+---+---+---+
    //! | Memory Bytes   | 0 | 1 | 2 | 3 |
    //! +----------------+---+---+---+---+
    //! | Little Endian  | B | G | R | A |
    //! | Big Endian     | A | R | G | B |
    //! +----------------+---+---+---+---+
    //! ```
    //!
    //! Supported natively by these libraries / APIs:
    //!
    //! ```
    //! +--------------+----------------+-------------------------------------+
    //! | API          | Interface      | Pixel Format                        |
    //! +--------------+----------------+-------------------------------------+
    //! | Cairo        | cairo_image_t  | CAIRO_FORMAT_ARGB32                 |
    //! | OSX/IOS      | CGImageRef     | 8bpp|8bpc|DefaultBO|PremulAlphaLast |
    //! | Qt           | QImage         | QImage::Format_ARGB32_Premultiplied |
    //! | WinAPI       | DIBSECTION     | 32bpp|8bpc|ARGB                     |
    //! +--------------+----------------+-------------------------------------+
    //! ```
    kPRGB32 = 1,

    //! 32-bit (X)RGB (8-bit components, native byte-order, (X)alpha byte ignored).
    //!
    //! Pixel layout:
    //!
    //! ```
    //! +----------------+---+---+---+---+
    //! | Memory Bytes   | 0 | 1 | 2 | 3 |
    //! +----------------+---+---+---+---+
    //! | Little Endian  | B | G | R | X |
    //! | Big Endian     | X | R | G | B |
    //! +----------------+---+---+---+---+
    //! ```
    //!
    //! Supported natively by these libraries / APIs:
    //!
    //! ```
    //! +--------------+----------------+-------------------------------------+
    //! | API          | Interface      | Pixel Format                        |
    //! +--------------+----------------+-------------------------------------+
    //! | Cairo        | cairo_image_t  | CAIRO_FORMAT_RGB24                  |
    //! | OSX/IOS      | CGImageRef     | 32bpp|8bpc|DefaultBO|SkipAlphaLast  |
    //! | Qt           | QImage         | QImage::Format_RGB32                |
    //! | WinAPI       | DIBSECTION     | 32bpp|8bpc|RGB                      |
    //! +--------------+----------------+-------------------------------------+
    //! ```
    kXRGB32 = 2,

    //! 8-bit ALPHA.
    //!
    //! Supported natively by these libraries / APIs:
    //!
    //! ```
    //! +--------------+----------------+-------------------------------------+
    //! | API          | Interface      | Pixel Format                        |
    //! +--------------+----------------+-------------------------------------+
    //! | Cairo        | cairo_image_t  | CAIRO_FORMAT_A8                     |
    //! | OSX/IOS      | CGImageRef     | 8bpp|8bpc|AlphaOnly                 |
    //! | WinAPI       | DIBSECTION     | 8bpp|8bpc                           |
    //! +--------------+----------------+-------------------------------------+
    //! ```
    kA8 = 3,

    //! Count of pixel formats.
    kCount = 4
  };

  static B2D_INLINE bool isValid(uint32_t id) noexcept {
    return id != PixelFormat::kNone && id < PixelFormat::kCount;
  }

  static B2D_INLINE uint32_t bppFromId(uint32_t id) noexcept;
}

// ============================================================================
// [b2d::PixelFormatInfo]
// ============================================================================

//! Pixel format information.
//!
//! In most use cases you would only need to use `PixelFormat`.
struct PixelFormatInfo {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t bpp() const noexcept { return _bpp; }
  B2D_INLINE uint32_t pixelFlags() const noexcept { return _flags; }
  B2D_INLINE uint32_t pixelFormat() const noexcept { return _format; }

  B2D_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }

  B2D_INLINE bool hasRGB() const noexcept { return hasFlag(PixelFlags::kComponentRGB); }
  B2D_INLINE bool hasARGB() const noexcept { return (_flags & PixelFlags::kComponentMask) == PixelFlags::kComponentARGB; }
  B2D_INLINE bool hasAlpha() const noexcept { return hasFlag(PixelFlags::kComponentAlpha); }

  B2D_INLINE bool isRGBOnly() const noexcept { return (_flags & PixelFlags::kComponentMask) == PixelFlags::kComponentRGB; }
  B2D_INLINE bool isAlphaOnly() const noexcept { return (_flags & PixelFlags::kComponentMask) == PixelFlags::kComponentAlpha; }

  B2D_INLINE bool hasUnusedBits() const noexcept { return hasFlag(PixelFlags::kUnusedBits); }
  B2D_INLINE bool isPremultiplied() const noexcept { return hasFlag(PixelFlags::kPremultiplied); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE bool operator==(const PixelFormatInfo& other) const noexcept { return _packed == other._packed; }
  B2D_INLINE bool operator!=(const PixelFormatInfo& other) const noexcept { return _packed != other._packed; }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static B2D_API const PixelFormatInfo table[PixelFormat::kCount];

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _format;
      uint8_t _bpp;
      uint16_t _flags;
    };
    uint32_t _packed;
  };
};

static B2D_INLINE uint32_t PixelFormat::bppFromId(uint32_t id) noexcept {
  B2D_ASSERT(id < PixelFormat::kCount);
  return PixelFormatInfo::table[id].bpp();
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_PIXELFORMAT_H
