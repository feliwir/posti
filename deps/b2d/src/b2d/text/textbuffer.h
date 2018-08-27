// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_TEXTSHAPER_H
#define _B2D_TEXT_TEXTSHAPER_H

// [Dependencies]
#include "../core/allocator.h"
#include "../text/font.h"
#include "../text/fontface.h"
#include "../text/glyphitem.h"
#include "../text/glyphrun.h"
#include "../text/textrun.h"
#include "../text/textmetrics.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::TextBuffer]
// ============================================================================

//! A text buffer to shape or to convert to glyphs.
class TextBuffer {
public:
  B2D_NONCOPYABLE(TextBuffer)

  enum Flags : uint32_t {
    kFlagGlyphIds         = 0x00000001u, //!< Buffer contains GlyphIds and not code points.
    kFlagGlyphAdvances    = 0x00000002u, //!< Buffer provides glyph advances.
    kFlagHasBoundingBox   = 0x08000000u, //!< Buffer has a calculated bounding box.
    kFlagInvalidChars     = 0x40000000u, //!< String was created from input that has errors.
    kFlagUndefinedChars   = 0x80000000u  //!< String has characters that weren't mapped to glyphs.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE TextBuffer(Allocator* allocator = Allocator::host()) noexcept
    : _allocator(allocator),
      _flags(0),
      _capacity(0),
      _items(nullptr),
      _offsets(nullptr),
      _glyphRun {} {}
  B2D_INLINE ~TextBuffer() noexcept { reset(Globals::kResetHard); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the associated allocator (weak).
  B2D_INLINE Allocator* allocator() const noexcept { return _allocator; }

  //! Get whether the buffer is empty.
  B2D_INLINE bool empty() const noexcept { return size() == 0; }
  //! Get the number of glyph items stored in the buffer.
  B2D_INLINE uint32_t size() const noexcept { return _glyphRun.size(); }
  //! Get the current buffer capacity.
  B2D_INLINE uint32_t capacity() const noexcept { return _capacity; }
  //! Get buffer flags, see `Flags`.
  B2D_INLINE uint32_t flags() const noexcept { return _flags; }

  //! Get whether the input string contained invalid characters (unicode encoding errors).
  B2D_INLINE bool hasInvalidChars() const noexcept { return (_flags & kFlagInvalidChars) != 0; }
  //! Get whether the input string contained undefined characters that weren't mapped properly to glyphs.
  B2D_INLINE bool hasUndefinedChars() const noexcept { return (_flags & kFlagUndefinedChars) != 0; }

  //! Get whether this buffer contains code points.
  B2D_INLINE bool hasCodePoints() const noexcept { return (_flags & kFlagGlyphIds) == 0; }
  //! Get whether this buffer contains glyph ids.
  B2D_INLINE bool hasGlyphIds() const noexcept { return (_flags & kFlagGlyphIds) != 0; }

  //! Get glyph items.
  B2D_INLINE GlyphItem* items() noexcept { return _items; }
  //! \overload
  B2D_INLINE const GlyphItem* items() const noexcept { return _items; }

  //! Get glyph offsets.
  B2D_INLINE Point* offsets() noexcept { return _offsets; }
  //! \overload
  B2D_INLINE const Point* offsets() const noexcept { return _offsets; }

  //! Get a `GlyphRun` relevant to the current status of `TextBuffer`.
  B2D_INLINE const GlyphRun& glyphRun() const noexcept { return _glyphRun; }

  // --------------------------------------------------------------------------
  // [Storage]
  // --------------------------------------------------------------------------

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;
  B2D_API Error prepare(uint32_t capacity) noexcept;

  // --------------------------------------------------------------------------
  // [Text]
  // --------------------------------------------------------------------------

  B2D_API Error setUtf8Text(const char* data, size_t size = Globals::kNullTerminated) noexcept;
  B2D_API Error setUtf16Text(const uint16_t* data, size_t size = Globals::kNullTerminated) noexcept;
  B2D_API Error setUtf32Text(const uint32_t* data, size_t size = Globals::kNullTerminated) noexcept;

  B2D_INLINE Error setUtf8Text(const uint8_t* data, size_t size = Globals::kNullTerminated) noexcept {
    return setUtf8Text(reinterpret_cast<const char*>(data), size);
  }

  B2D_INLINE Error setWideText(const wchar_t* data, size_t size = Globals::kNullTerminated) noexcept {
    if (sizeof(wchar_t) == sizeof(uint32_t))
      return setUtf32Text(reinterpret_cast<const uint32_t*>(data), size);
    else if (sizeof(wchar_t) == sizeof(uint16_t))
      return setUtf16Text(reinterpret_cast<const uint16_t*>(data), size);
    else // Not sure this would ever be triggered.
      return setUtf8Text(reinterpret_cast<const char*>(data), size);
  }

  // --------------------------------------------------------------------------
  // [Processing]
  // --------------------------------------------------------------------------

  B2D_API Error shape(const GlyphEngine& engine) noexcept;

  B2D_API Error mapToGlyphs(const GlyphEngine& engine) noexcept;
  B2D_API Error positionGlyphs(const GlyphEngine& engine, uint32_t positioningFlags = 0xFFFFFFFFU) noexcept;

  // --------------------------------------------------------------------------
  // [Measure]
  // --------------------------------------------------------------------------

  B2D_API Error measureText(const GlyphEngine& engine, TextMetrics& out) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Allocator* _allocator;                 //!< Allocator.
  uint32_t _flags;                       //!< Buffer flags.
  uint32_t _capacity;                    //!< Buffer capacity (GlyphItems).
  GlyphItem* _items;                     //!< Glyph items (or text code-points if not yet mapped).
  Point* _offsets;                       //!< Glyph offsets.
  GlyphRun _glyphRun;                    //!< Glyph run.
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_TEXTSHAPER_H
