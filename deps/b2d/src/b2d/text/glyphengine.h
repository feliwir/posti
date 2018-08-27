// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_GLYPHENGINE_H
#define _B2D_TEXT_GLYPHENGINE_H

// [Dependencies]
#include "../core/wrap.h"
#include "../text/font.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/glyphitem.h"
#include "../text/glyphrun.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::GlyphMappingState]
// ============================================================================

//! Character to glyph mapping state.
struct GlyphMappingState {
  B2D_INLINE void reset() noexcept {
    _glyphCount = 0;
    _undefinedCount = 0;
    _undefinedFirst = Globals::kInvalidIndex;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE size_t glyphCount() const noexcept { return _glyphCount; }
  B2D_INLINE size_t undefinedCount() const noexcept { return _undefinedCount; }
  B2D_INLINE size_t undefinedFirst() const noexcept { return _undefinedFirst; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _glyphCount;                    //!< Number of glyphs or glyph-items out.
  size_t _undefinedCount;                //!< Numbef of undefined glyphs (chars that have no mapping).
  size_t _undefinedFirst;                //!< Index of the first undefined glyph.
};

// ============================================================================
// [b2d::GlyphEngine]
// ============================================================================

//! Glyph engine is a lightweight and usually short-lived object that provides
//! low-level access to glyph processing of the underlying `FontFace`. The main
//! objective of this class is to provide a simple way to lock `FontData` and
//! to provide simple wrappers around `FontFace` selected implementations of
//! various font features.
class GlyphEngine {
public:
  B2D_NONCOPYABLE(GlyphEngine)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API GlyphEngine() noexcept;
  B2D_API ~GlyphEngine() noexcept;

  // --------------------------------------------------------------------------
  // [Reset / Create]
  // --------------------------------------------------------------------------

  //! Reset the engine, called implicitly by `GlyphEngine` destructor.
  B2D_API void reset() noexcept;

  //! Create a `GlyphEngine` from the given `font`.
  B2D_API Error createFromFont(const Font& font) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether this class was properly initialized and can be used.
  B2D_INLINE bool empty() const noexcept { return _font.isNone(); }

  //! Get the associated `Font`.
  B2D_INLINE const Font& font() const noexcept { return _font; }

  //! Get the associated `FontFace`.
  B2D_INLINE const FontFace& fontFace() const noexcept { return _fontFace(); }

  //! Get the associated `FontData`.
  B2D_INLINE FontData& fontData() noexcept { return _fontData; }
  //! \overload
  B2D_INLINE const FontData& fontData() const noexcept { return _fontData; }

  //! \internal
  //!
  //! Get functions provided by `FontFace`.
  B2D_INLINE const FontFaceFuncs& funcs() const noexcept { return _fontFace->impl()->_funcs; }

  // --------------------------------------------------------------------------
  // [Character To Glyph Mapping]
  // --------------------------------------------------------------------------

  B2D_INLINE Error mapGlyphItem(GlyphItem& item) const noexcept {
    GlyphMappingState dummyState;
    return funcs().mapGlyphItems(this, &item, 1, &dummyState);
  }

  B2D_INLINE Error mapGlyphItems(GlyphItem* items, size_t count, GlyphMappingState& state) const noexcept {
    return funcs().mapGlyphItems(this, items, count, &state);
  }

  // --------------------------------------------------------------------------
  // [Glyph Bounding Box]
  // --------------------------------------------------------------------------

  B2D_INLINE Error getGlyphBoundingBoxes(const GlyphId* glyphIds, IntBox* glyphBoxes, size_t count) const noexcept {
    return funcs().getGlyphBoundingBoxes(this, glyphIds, sizeof(GlyphId), glyphBoxes, count);
  }

  B2D_INLINE Error getGlyphBoundingBoxes(const GlyphItem* glyphItems, IntBox* glyphBoxes, size_t count) const noexcept {
    return funcs().getGlyphBoundingBoxes(this, &glyphItems->_glyphId, sizeof(GlyphItem), glyphBoxes, count);
  }

  // --------------------------------------------------------------------------
  // [Glyph Positioning]
  // --------------------------------------------------------------------------

  B2D_INLINE Error getGlyphAdvances(const GlyphItem* glyphItems, Point* glyphOffsets, size_t count) const noexcept {
    return funcs().getGlyphAdvances(this, glyphItems, glyphOffsets, count);
  }

  B2D_INLINE Error applyPairAdjustment(GlyphItem* glyphItems, Point* glyphOffsets, size_t count) const noexcept {
    return funcs().applyPairAdjustment(this, glyphItems, glyphOffsets, count);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Font _font;                            //!< A font which should have a valid `FontFace` and other descriptors.
  Weak<FontFace> _fontFace;              //!< A weak reference to `FontFace` only used as a shortcut in inline functions.
  FontData _fontData;                    //!< FontData used to populate `_tables[]` required for decoding.
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_GLYPHENGINE_H
