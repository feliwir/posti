// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_GLYPHRUN_H
#define _B2D_TEXT_GLYPHRUN_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/support.h"
#include "../text/glyphitem.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::GlyphRun]
// ============================================================================

//! GlyphRun describes a set of consecutive glyphs and their offsets.
//!
//! GlyphRun should only be used to pass glyph IDs and their offsets to the
//! rendering context. The purpose of GlyphRun is to allow to render glyphs
//! which could be shaped by various shaping engines (Blend2D, Harfbuzz, etc).
//!
//! GlyphRun allows to render glyphs that are part of a bigger structure (for
//! example `hb_glyph_info`), raw array like uint16_t[], etc. Glyph positions
//! at the moment use Blend2D's `Point`, but it's possible to extend the data
//! type in the future.
//!
//! There are currently 3 offset modes:
//!
//!   - Absolute offsets specified through `kFlagOffsetsAreAbsolute` flag. In
//!     such case each offset describes an absolute position of the relevant
//!     glyph.
//!
//!   - Relative offsets specified through `kFlagOffsetsAreRelative` flag. In
//!     such case each offset describes glyph advance after the glyph was sent
//!     to `GlyphOutlineSinkFunc`.
//!
//!   - No offsets (no offset flag specified). In this case the positioning of
//!     each glyph relies on `GlyphOutlineSinkFunc` function or glyphs are not
//!     positioned at all. If you need to render only one glyph, for example,
//!     you don't need to specify offsets if the user-matrix contains correct
//!     translation part.

//! There are currently 3 offset units:
//!
//!   - User-space offsets specified through `kFlagUserSpaceOffsets` flag. Each
//!     offset in such case is in user-space units and would be transformed by
//!     user matrix (`fontMatrix` would be ignored in such case).
//!
//!   - Design space offsets specified through `kFlagUserSpaceOffsets` flag.
//!     Each offset is specified in font design-units. This means that it will
//!     be transformed by a combination of user-matrix and design-matrix.
//!
//!   - No units specified. In this case offsets are considered RAW units and
//!     will be used without any transformation.
struct GlyphRun {
  enum Flags : uint32_t {
    kFlagOffsetsAreAbsolute   = 0x0001u, //!< Each offset is in absolute units.
    kFlagOffsetsAreAdvances   = 0x0002u, //!< Each offset is a glyph advance.
    kFlagUserSpaceOffsets     = 0x0004u, //!< Offsets are in user-space units.
    kFlagDesignSpaceOffsets   = 0x0008u  //!< Offsets are in design-space units.
  };

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _flags = 0;
    _glyphIdAdvance = 0;
    _glyphOffsetAdvance = 0;
    _size = 0;
    _glyphIdData = nullptr;
    _glyphOffsetData = nullptr;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _size == 0; }

  B2D_INLINE uint32_t flags() const noexcept { return _flags; }
  B2D_INLINE void setFlags(uint32_t flags) noexcept { _flags = uint16_t(flags); }
  B2D_INLINE void addFlags(uint32_t flags) noexcept { _flags = uint16_t(_flags | flags); }
  B2D_INLINE void clearFlags(uint32_t flags) noexcept { _flags = uint16_t(_flags & ~flags); }
  B2D_INLINE void resetFlags() noexcept { _flags = uint16_t(0); }

  B2D_INLINE uint32_t size() const noexcept { return _size; }
  B2D_INLINE void setSize(uint32_t size) noexcept { _size = size; }
  B2D_INLINE void resetSize() noexcept { _size = 0; }

  template<typename T = void>
  B2D_INLINE T* glyphIdData() const noexcept { return static_cast<T*>(_glyphIdData); }

  template<typename T = void>
  B2D_INLINE T* glyphOffsetData() const noexcept { return static_cast<T*>(_glyphOffsetData); }

  B2D_INLINE intptr_t glyphIdAdvance() const noexcept { return _glyphIdAdvance; }
  B2D_INLINE intptr_t glyphOffsetAdvance() const noexcept { return _glyphOffsetAdvance; }

  B2D_INLINE void setGlyphIds(const GlyphId* glyphIds) noexcept { setGlyphIdData(glyphIds, intptr_t(sizeof(uint16_t))); }
  B2D_INLINE void setGlyphItems(const GlyphItem* glyphItems) noexcept { setGlyphIdData(&glyphItems->_glyphId, intptr_t(sizeof(GlyphItem))); }

  B2D_INLINE void setGlyphIdData(const void* glyphIdData, intptr_t glyphIdAdvance) noexcept {
    _glyphIdData = const_cast<void*>(glyphIdData);
    _glyphIdAdvance = int8_t(glyphIdAdvance);
  }

  B2D_INLINE void resetGlyphIdData() noexcept {
    _glyphIdData = nullptr;
    _glyphIdAdvance = 0;
  }

  B2D_INLINE void setGlyphOffsets(const Point* glyphOffsets) noexcept {
    setGlyphOffsetData(glyphOffsets, sizeof(Point));
  }

  B2D_INLINE void setGlyphOffsetData(const void* glyphOffsetData, intptr_t glyphOffsetAdvance) noexcept {
    _glyphOffsetData = const_cast<void*>(glyphOffsetData);
    _glyphOffsetAdvance = int8_t(glyphOffsetAdvance);
  }

  B2D_INLINE void resetGlyphOffsetData() noexcept {
    _glyphOffsetData = nullptr;
    _glyphOffsetAdvance = 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint16_t _flags;
  int8_t _glyphIdAdvance;
  int8_t _glyphOffsetAdvance;
  uint32_t _size;

  void* _glyphIdData;
  void* _glyphOffsetData;
};

// ============================================================================
// [b2d::GlyphRunIterator]
// ============================================================================

//! A helper to iterate over a `GlyphRun`.
//!
//! Takes into consideration glyph-id advance and glyph-offset advance.
//!
//! Example:
//!
//! ```
//! void inspectGlyphRun(const GlyphRun& glyphRun) noexcept {
//!   GlyphRunIterator it(glyphRun);
//!   if (it.hasOffsets()) {
//!     while (!it.atEnd()) {
//!       GlyphId glyphId = it.glyphId();
//!       Point offset = it.glyphOffset();
//!
//!       // Do something with `glyphId` and `offset`.
//!
//!       it.advance();
//!     }
//!   }
//!   else {
//!     while (!it.atEnd()) {
//!       GlyphId glyphId = it.glyphId();
//!
//!       // Do something with `glyphId`.
//!
//!       it.advance();
//!     }
//!   }
//! }
//! ```
class GlyphRunIterator {
public:
  B2D_INLINE GlyphRunIterator() noexcept { reset(); }
  B2D_INLINE explicit GlyphRunIterator(const GlyphRun& glyphRun) noexcept { reset(glyphRun); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _index = 0;
    _size = 0;
    _glyphIdData = nullptr;
    _glyphIdAdvance = 0;
    _glyphOffsetData = nullptr;
    _glyphOffsetAdvance = 0;
  }

  B2D_INLINE void reset(const GlyphRun& glyphRun) noexcept {
    _index = 0;
    _size = glyphRun.size();
    _glyphIdData = glyphRun.glyphIdData();
    _glyphIdAdvance = glyphRun.glyphIdAdvance();
    _glyphOffsetData = glyphRun.glyphOffsetData();
    _glyphOffsetAdvance = glyphRun.glyphOffsetAdvance();
  }

  // --------------------------------------------------------------------------
  // [Iterator]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _size == 0; }
  B2D_INLINE bool atEnd() noexcept { return _index == _size; }
  B2D_INLINE size_t index() const noexcept { return _index; }

  B2D_INLINE GlyphId glyphId() const noexcept { return *static_cast<const uint16_t*>(_glyphIdData); }

  B2D_INLINE bool hasOffsets() const noexcept { return _glyphOffsetData != nullptr; }
  B2D_INLINE Point glyphOffset() const noexcept { return *static_cast<const Point*>(_glyphOffsetData); }

  B2D_INLINE void advance() noexcept {
    B2D_ASSERT(_index < _size);
    _index++;

    _glyphIdData = Support::advancePtr(_glyphIdData, _glyphIdAdvance);
    _glyphOffsetData = Support::advancePtr(_glyphOffsetData, _glyphOffsetAdvance);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _index;
  size_t _size;

  void* _glyphIdData;
  intptr_t _glyphIdAdvance;

  void* _glyphOffsetData;
  intptr_t _glyphOffsetAdvance;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_GLYPHRUN_H
