// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_CARRAY_H
#define _B2D_CORE_CARRAY_H

// [Dependencies]
#include "../core/build.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

template<typename T>
struct CArray {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE CArray() noexcept
    : _data(nullptr),
      _size(0) {}

  B2D_INLINE CArray(const CArray& other) noexcept
    : _data(other._data),
      _size(other._size) {}

  B2D_INLINE CArray(T* data, size_t size) noexcept
    : _data(data),
      _size(size) {}

  B2D_INLINE CArray(const T* data, size_t size) noexcept
    : _data(const_cast<T*>(data)),
      _size(size) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _data = nullptr;
    _size = 0;
  }

  B2D_INLINE void reset(T* data, size_t size) noexcept {
    _data = data;
    _size = size;
  }

  B2D_INLINE void reset(const T* data, size_t size) noexcept {
    _data = const_cast<T*>(data);
    _size = size;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE T* data() noexcept { return _data; }
  B2D_INLINE const T* data() const noexcept { return _data; }

  B2D_INLINE void setData(T* data) noexcept { _data = data; }
  B2D_INLINE void setData(const T* data) noexcept { _data = const_cast<T*>(data); }

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE void setSize(size_t size) noexcept { _size = size; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE CArray<T>& operator=(const CArray<T>& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  T* _data;
  size_t _size;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_CARRAY_H
