// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_TEXTRUN_H
#define _B2D_TEXT_TEXTRUN_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::TextRun]
// ============================================================================

class TextRun {
public:
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t flags() const noexcept { return _flags; }
  B2D_INLINE uint32_t size() const noexcept { return _size; }
  B2D_INLINE const char* text() const noexcept { return _text; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _flags;
  uint32_t _size;
  const char* _text;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_TEXTRUN_H
