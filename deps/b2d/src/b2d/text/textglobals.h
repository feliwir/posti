// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_TEXTGLOBALS_H
#define _B2D_TEXT_TEXTGLOBALS_H

// [Dependencies]
#include "../core/unicode.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

// Core classes.
class Path2D;

// Font & text related.
class Font;
class FontImpl;

class FontData;
class FontDataImpl;

class FontFace;
class FontFaceImpl;

class FontLoader;
class FontLoaderImpl;

class GlyphEngine;
class GlyphOutlineDecoder;

struct GlyphItem;
struct GlyphMappingState;
struct GlyphOutlineSinkInfo;

struct FontMetrics;
struct FontDesignMetrics;
struct TextMetrics;

// ============================================================================
// [Callbacks]
// ============================================================================

//! Optional callback that can be used to consume outline contours during decoding.
typedef Error (B2D_CDECL* GlyphOutlineSinkFunc)(Path2D& path, const GlyphOutlineSinkInfo& info, void* closure);

// ============================================================================
// [b2d::GlyphId]
// ============================================================================

//! Glyph ID - Real index to a glyph stored in a FontFace.
//!
//! Glyph ID is always unsigned 16-bit integer as used by TrueType and OpenType
//! fonts. There are some libraries that use 32-bit integers for Glyph indexes,
//! but values above 65535 are never used in practice as font's generally do
//! not have the ability to index more than 65535 glyphs (excluding null glyph).
typedef uint16_t GlyphId;

// ============================================================================
// [b2d::TextDirection]
// ============================================================================

struct TextDirection {
  enum Mode : uint32_t {
    kLTR = 0,
    kRTL = 1
  };

  static constexpr uint32_t fromLevel(uint32_t level) noexcept { return level & 0x1; }
};

// ============================================================================
// [b2d::TextOrientation]
// ============================================================================

struct TextOrientation {
  enum Mode : uint32_t {
    kHorizontal = 0,
    kVertical   = 1
  };
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_TEXTGLOBALS_H
