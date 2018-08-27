// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTMETRICS_H
#define _B2D_TEXT_FONTMETRICS_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontDesignMetrics]
// ============================================================================

//! Font design metrics.
//!
//! Design metrics is information that `FontFace` collected directly from the
//! font data. It means that all fields are RAW units that weren't transformed
//! in any way.
//!
//! When a new `Font` instance is created, it would automatically calculate
//! `FontMetrics` based on `FontDesignMetrics` and other font properties like
//! size, stretch, etc...
struct FontDesignMetrics {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { std::memset(this, 0, sizeof(*this));}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr int ascent() const noexcept { return int(_ascent[TextOrientation::kHorizontal]); }
  constexpr int descent() const noexcept { return int(_descent[TextOrientation::kHorizontal]); }
  constexpr int lineGap() const noexcept { return int(_lineGap); }
  constexpr int xHeight() const noexcept { return int(_xHeight); }
  constexpr int capHeight() const noexcept { return int(_capHeight); }

  constexpr int vertAscent() const noexcept { return int(_ascent[TextOrientation::kVertical]); }
  constexpr int vertDescent() const noexcept { return int(_descent[TextOrientation::kVertical]); }

  constexpr int minLeftSideBearing() const noexcept { return _minLSB[TextOrientation::kHorizontal]; }
  constexpr int minRightSideBearing() const noexcept { return _minTSB[TextOrientation::kHorizontal]; }
  constexpr int maxAdvanceWidth() const noexcept { return _maxAdvance[TextOrientation::kHorizontal]; }

  constexpr int minTopSideBearing() const noexcept { return _minLSB[TextOrientation::kVertical]; }
  constexpr int minBottomSideBearing() const noexcept { return _minTSB[TextOrientation::kVertical]; }
  constexpr int maxAdvanceHeight() const noexcept { return _maxAdvance[TextOrientation::kVertical]; }

  constexpr int underlinePosition() const noexcept { return int(_underlinePosition); }
  constexpr int underlineThickness() const noexcept { return int(_underlineThickness); }
  constexpr int strikethroughPosition() const noexcept { return int(_strikethroughPosition); }
  constexpr int strikethroughThickness() const noexcept { return int(_strikethroughThickness); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint16_t _ascent[2];                   //!< Horizontal and vertical ascent.
  uint16_t _descent[2];                  //!< Horizontal and vertical descent.
  int16_t _lineGap;                      //!< Line gap.
  uint16_t _xHeight;                     //!< Height of 'x' character.
  uint16_t _capHeight;                   //!< Height of a capitalized character.

  int16_t _minLSB[2];                    //!< Minimum leading side bearing (horizontal & vertical).
  int16_t _minTSB[2];                    //!< Minimum trailing side bearing (horizontal & vertical).
  uint16_t _maxAdvance[2];               //!< Maximum advance width (horizontal) and height (vertical).

  int16_t _underlinePosition;
  uint16_t _underlineThickness;
  int16_t _strikethroughPosition;
  uint16_t _strikethroughThickness;
};

// ============================================================================
// [b2d::FontMetrics]
// ============================================================================

//! Font metrics.
//!
//! Font metrics is a scaled `FontDesignMetrics` based on `Font` size and other
//! options.
struct FontMetrics {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { std::memset(this, 0, sizeof(*this));}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr float ascent() const noexcept { return _ascent[TextOrientation::kHorizontal]; }
  constexpr float descent() const noexcept { return _descent[TextOrientation::kHorizontal]; }
  constexpr float lineGap() const noexcept { return _lineGap; }
  constexpr float xHeight() const noexcept { return _xHeight; }
  constexpr float capHeight() const noexcept { return _capHeight; }

  constexpr float vertAscent() const noexcept { return _ascent[TextOrientation::kVertical]; }
  constexpr float vertDescent() const noexcept { return _descent[TextOrientation::kVertical]; }

  constexpr float underlinePosition() const noexcept { return _underlinePosition; }
  constexpr float underlineThickness() const noexcept { return _underlineThickness; }
  constexpr float strikethroughPosition() const noexcept { return _strikethroughPosition; }
  constexpr float strikethroughThickness() const noexcept { return _strikethroughThickness; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  float _ascent[2];
  float _descent[2];
  float _lineGap;
  float _xHeight;
  float _capHeight;

  float _underlinePosition;
  float _underlineThickness;
  float _strikethroughPosition;
  float _strikethroughThickness;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTMETRICS_H
