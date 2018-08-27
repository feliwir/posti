// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERSPAN_P_H
#define _B2D_RASTERENGINE_RASTERSPAN_P_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterSpan]
// ============================================================================

//! \internal
//!
//! Span - definition of mask, size, and c-mask or v-mask.
//!
//! Spans are always related with scanline, so information about Y position is
//! not part of the span. The const mask is encoded in mask pointer so you should
//! always check if span is CMask (@c isConst() method) or vmask (@c isVariant()
//! method). There are asserts so you should be warned that you are using span
//! incorrectly.
//!
//! Note that you can base another class on @c SWSpan to extend its
//! functionality. The core idea is that @c SWSpan is used across the API
//! so you don't need to define more classes to work with spans.
struct RasterSpan {
  //! Span type.
  enum Type : uint32_t {
    // --------------------------------------------------------------------------
    // Do not change these constants without checking SWSpan itself. There
    // are some optimizations that are based on the values of these constants.
    // --------------------------------------------------------------------------

    //! Span is a const-mask (0-255 or 0-65535 depending on target).
    kTypeCMask = 0,
    //! Span is a variant-mask (0-255).
    kTypeVMaskA8 = 1,

    //! The count of span types.
    kTypeCount = 2
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Whether the span is valid (used by asserts).
  B2D_INLINE bool isValid() const {
    return (_x0 < _x1) && (_mask != nullptr);
  }

  //! Get span type.
  B2D_INLINE uint32_t type() const {
    return _type;
  }

  //! Set span type.
  B2D_INLINE void setType(uint32_t type) {
    B2D_ASSERT(type < kTypeCount);
    _type = type;
  }

  //! Set span start/end position.
  B2D_INLINE void setPosition(int x0, int x1) {
    // Disallow invalid position.
    B2D_ASSERT(x0 >= 0);
    B2D_ASSERT(x1 >= 0);
    B2D_ASSERT(x0 < x1);

    _x0 = x0;
    _x1 = x1;
  }

  //! Set span start/end position and type.
  B2D_INLINE void setPositionAndType(int x0, int x1, uint32_t type) {
    // Disallow invalid position.
    B2D_ASSERT(x0 >= 0);
    B2D_ASSERT(x1 >= 0);
    B2D_ASSERT(x0 < x1);

    _type = type;
    _x0 = uint32_t(unsigned(x0));
    _x1 = uint32_t(unsigned(x1));
  }

  //! Get span start position, inclusive (x0).
  B2D_INLINE int x0() const {
    return int(_x0);
  }

  //! Set span start position.
  B2D_INLINE void setX0(int x0) {
    B2D_ASSERT(x0 >= 0);
    _x0 = uint32_t(unsigned(x0));
  }

  //! Set span start position and type.
  B2D_INLINE void setX0AndType(int x0, uint32_t type) {
    B2D_ASSERT(x0 >= 0);
    _x0 = uint32_t(unsigned(x0));
    _type = type;
  }

  //! Get span end position, exclusive (x1).
  B2D_INLINE int x1() const {
    return int(_x1);
  }

  //! Set span end position.
  B2D_INLINE void setX1(int x1) {
    B2D_ASSERT(x1 >= 0);
    _x1 = uint32_t(unsigned(x1));
  }

  //! Get span size, computed as <code>x1 - x0</code>.
  B2D_INLINE int size() const {
    return int(_x1 - _x0);
  }

  // --------------------------------------------------------------------------
  // [Type Checks]
  // --------------------------------------------------------------------------

  //! Get whether the span is a const-mask.
  B2D_INLINE bool isCMask() const { return _type == kTypeCMask; }
  //! Get whether the span is a variant-mask.
  B2D_INLINE bool isVMask() const { return _type != kTypeCMask; }

  //! Get whether the span is a A8-Glyph.
  B2D_INLINE bool isVMaskA8() const { return _type == kTypeVMaskA8; }

  // --------------------------------------------------------------------------
  // [Generic Interface]
  // --------------------------------------------------------------------------

  //! Get the generic mask as pointer.
  B2D_INLINE void* _maskPtr() const { return _mask; }
  //! Get the generic mask as integer.
  B2D_INLINE uintptr_t _maskInt() const { return _mask_uint; }

  //! Set the generic mask pointer to `mask`.
  B2D_INLINE void _setMaskPtr(void* mask) { _mask = mask; }
  //! Set the generic mask integer to `mask`.
  B2D_INLINE void _setMaskInt(uintptr_t mask) { _mask_uint = mask; }

  // --------------------------------------------------------------------------
  // [CMask-Interface]
  // --------------------------------------------------------------------------

  //! Get the c-mask value.
  B2D_INLINE uint32_t cMask() const {
    B2D_ASSERT(isCMask());
    return uint32_t(_mask_uint);
  }

  // --------------------------------------------------------------------------
  // [VMask-Interface]
  // --------------------------------------------------------------------------

  //! Get the c-mask pointer.
  B2D_INLINE uint8_t* vMask() const {
    B2D_ASSERT(isVMask());
    return reinterpret_cast<uint8_t*>(_mask);
  }

  //! Set the c-mask pointer.
  B2D_INLINE void setVMask(uint8_t* mask) {
    B2D_ASSERT(isVMask());
    _mask = reinterpret_cast<void*>(mask);
  }

  // --------------------------------------------------------------------------
  // [Next]
  // --------------------------------------------------------------------------

  //! Get the next span.
  B2D_INLINE SWSpan* next() const { return _next; }
  //! Set the next span.
  B2D_INLINE void setNext(SWSpan* next) { _next = next; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Start of span (first valid pixel, inclusive).
  uint32_t _x0 : 29;
  //! Type of span.
  uint32_t _type : 3;
  //! End of span (last valid pixel, exclusive).
  uint32_t _x1;

  //! CMask value or pointer to VMask.
  //!
  //! Use CMask/VMask getters & setters to access or modify this value.
  union {
    void* _mask;
    uintptr_t _mask_uint;
  };

  //! Next span in a span-chain or @c nullptr if this is the last span.
  SWSpan* _next;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERSPAN_P_H
