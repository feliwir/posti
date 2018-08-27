// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_GLYPHOUTLINEDECODER_H
#define _B2D_TEXT_GLYPHOUTLINEDECODER_H

// [Dependencies]
#include "../core/matrix2d.h"
#include "../core/membuffer.h"
#include "../core/path2d.h"
#include "../core/wrap.h"
#include "../text/font.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/glyphrun.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::GlyphOutlineSinkInfo]
// ============================================================================

//! Information passed to a sink by `GlyphOutlineDecoder`.
struct GlyphOutlineSinkInfo {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE size_t glyphIndex() const noexcept { return _glyphIndex; }
  B2D_INLINE size_t contourCount() const noexcept { return _contourCount; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _glyphIndex;
  size_t _contourCount;
};

// ============================================================================
// [b2d::GlyphOutlineDecoder]
// ============================================================================

//! Glyph outline decoder is responsible for decoding glyph or glyph-run outlines
//! from a `Font`. It's a short-lived object that cannot outlive the font passed
//! to `createFromFont()`.
//!
//! This is an abstract interface that supports outlines defined by both TrueType
//! and OpenType formats. It's transparent to the user so there is no need to
//! distinguish between these two.
class GlyphOutlineDecoder {
public:
  B2D_NONCOPYABLE(GlyphOutlineDecoder)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API GlyphOutlineDecoder() noexcept;
  B2D_API ~GlyphOutlineDecoder() noexcept;

  // --------------------------------------------------------------------------
  // [Create / Reset]
  // --------------------------------------------------------------------------

  //! Create a `GlyphOutlineDecoder` from the given `font`.
  B2D_API Error createFromFont(const Font& font) noexcept;

  //! Reset the decoder, called implicitly by `GlyphOutlineDecoder` destructor.
  B2D_API void reset() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Returns true if this `GlyphOutlineDecoder` is empty, thus uninitialized.
  B2D_INLINE bool empty() const noexcept { return _fontFace.isNone(); }

  //! Get the associated `FontFace`.
  B2D_INLINE const FontFace& fontFace() const noexcept { return _fontFace; }

  //! Get the associated `FontData`.
  B2D_INLINE FontData& fontData() noexcept { return _fontData; }
  //! \overload
  B2D_INLINE const FontData& fontData() const noexcept { return _fontData; }

  // --------------------------------------------------------------------------
  // [Internals]
  // --------------------------------------------------------------------------

  B2D_INLINE const FontFaceFuncs& funcs() const noexcept { return _fontFace.impl()->_funcs; }

  // --------------------------------------------------------------------------
  // [Decode]
  // --------------------------------------------------------------------------

  //! \internal
  B2D_API Error _decodeGlyph(uint32_t glyphId, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept;

  //! \internal
  B2D_API Error _decodeGlyphRun(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept;

  //! Decode a single glyph and append the result to `out` path.
  B2D_INLINE Error decodeGlyph(uint32_t glyphId, const Matrix2D& userMatrix, Path2D& out) noexcept {
    return _decodeGlyph(glyphId, userMatrix, out, nullptr, nullptr);
  }

  //! Decode a single glyph, append the result to `out` path, and call `sink`
  //! during decoding.
  //!
  //! This function takes a `sink` function with `closure` that is used to flush
  //! one or more contour during decoding. If the `sink` was passed then it will
  //! always be called at least once if the decoding is successful. If the decoder
  //! fails for any reason it returns immediately and won't call `sink` anymore,
  //! but it cannot "undo" previous calls to `sink`.
  //!
  //! NOTE: The decoder never resets the path, so if the `sink` wants to consume
  //! the contours the path contains it must reset the path before returning to
  //! the decoder. Otherwise the decoder would append additional contours to the
  //! path.
  B2D_INLINE Error decodeGlyph(uint32_t glyphId, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept {
    return _decodeGlyph(glyphId, userMatrix, out, sink, closure);
  }

  //! Decode the given `glyphRun` and append the result to `out` path.
  B2D_INLINE Error decodeGlyphRun(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out) noexcept {
    return _decodeGlyphRun(glyphRun, userMatrix, out, nullptr, nullptr);
  }

  //! Decode the given `glyphRun`, append the result to `out` path, and call `sink` during decoding.
  B2D_INLINE Error decodeGlyphRun(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept {
    return _decodeGlyphRun(glyphRun, userMatrix, out, sink, closure);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FontFace _fontFace;                    //!< FontFace used by `font` passed to `createFromFont()`.
  FontData _fontData;                    //!< FontData required during the decoding.
  FontMatrix _fontMatrix;                //!< Font matrix taken from `font` by `createFromFont()`.
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_GLYPHOUTLINEDECODER_H
