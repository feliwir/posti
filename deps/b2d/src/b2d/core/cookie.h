// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_COOKIE_H
#define _B2D_CORE_COOKIE_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Cookie]
// ============================================================================

//! Cookie is a class that holds an arbitrary 128-bit value that can be used
//! to match other cookies. The first value `data0` is usually a cookie origin,
//! which can be created by a static function `Cookie::generateUnique64BitId()`,
//! and the second value `data1` is usually a payload associated with that origin.
//!
//! Cookies can be used by `Context2D::save()` and `Context2D::restore()` functions.
class Cookie {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  constexpr Cookie() noexcept : _data { 0, 0 } {}
  constexpr Cookie(const Cookie& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0, 0); }
  B2D_INLINE void reset(uint64_t data0, uint64_t data1) noexcept {
    _data[0] = data0;
    _data[1] = data1;
  }

  constexpr bool empty() const noexcept { return (_data[0] | _data[1]) == 0; }

  constexpr uint64_t data0() const noexcept { return _data[0]; }
  constexpr uint64_t data1() const noexcept { return _data[1]; }

  constexpr bool equals(const Cookie& other) const noexcept {
    return (_data[0] == other._data[0]) & (_data[1] == other._data[1]);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Cookie& operator=(const Cookie& other) noexcept = default;

  constexpr bool operator==(const Cookie& other) const noexcept { return  equals(other); }
  constexpr bool operator!=(const Cookie& other) const noexcept { return !equals(other); }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static B2D_API uint64_t generateUnique64BitId() noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint64_t _data[2];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_COOKIE_H
