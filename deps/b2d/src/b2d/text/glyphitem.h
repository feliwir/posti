// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_GLYPHITEM_H
#define _B2D_TEXT_GLYPHITEM_H

// [Dependencies]
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::GlyphItem]
// ============================================================================

struct GlyphItem {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _codePoint = 0;
    _cluster = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t codePoint() const noexcept { return _codePoint; }
  B2D_INLINE uint32_t glyphId() const noexcept { return _glyphId; }
  B2D_INLINE uint32_t glyphFlags() const noexcept { return _glyphFlags; }
  B2D_INLINE uint32_t cluster() const noexcept { return _cluster; }

  B2D_INLINE void setCodePoint(uint32_t codePoint) noexcept { _codePoint = codePoint; }
  B2D_INLINE void setGlyphId(GlyphId glyphId) noexcept { _glyphId = glyphId; }
  B2D_INLINE void setGlyphFlags(uint32_t flags) noexcept { _glyphFlags = uint16_t(flags); }
  B2D_INLINE void setCluster(uint32_t cluster) noexcept { _cluster = cluster; }

  B2D_INLINE void setGlyphIdAndFlags(GlyphId glyphId, uint32_t flags) noexcept {
    _glyphId = glyphId;
    _glyphFlags = uint16_t(flags);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint32_t _codePoint;
    struct {
      GlyphId _glyphId;
      uint16_t _glyphFlags;
    };
  };

  uint32_t _cluster;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_GLYPHITEM_H
