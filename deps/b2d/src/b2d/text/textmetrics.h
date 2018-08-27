// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_TEXTMETRICS_H
#define _B2D_TEXT_TEXTMETRICS_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::TextMetrics]
// ============================================================================

struct TextMetrics {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _advance.reset();
    _boundingBox.reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Size& advance() const noexcept { return _advance; }
  B2D_INLINE double advanceWidth() const noexcept { return _advance.width(); }
  B2D_INLINE double advanceHeight() const noexcept { return _advance.height(); }

  B2D_INLINE const Box& boundingBox() const noexcept { return _boundingBox; }
  B2D_INLINE double width() const noexcept { return _boundingBox.width(); }
  B2D_INLINE double height() const noexcept { return _boundingBox.height(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Size _advance;
  Box _boundingBox;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_TEXTMETRICS_H
