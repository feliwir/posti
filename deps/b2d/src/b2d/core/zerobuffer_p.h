// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZEROBUFFER_P_H
#define _B2D_CORE_ZEROBUFFER_P_H

// [Dependencies]
#include "../core/zeroallocator_p.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ZeroBuffer]
// ============================================================================

//! \internal
//!
//! Memory buffer that is initially zeroed and that must be zeroed upon release.
class ZeroBuffer {
public:
  B2D_NONCOPYABLE(ZeroBuffer)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ZeroBuffer() noexcept
    : _ptr(nullptr),
      _size(0) {}

  B2D_INLINE ZeroBuffer(ZeroBuffer&& other) noexcept
    : _ptr(other._ptr),
      _size(other._size) {
    other._ptr = nullptr;
    other._size = 0;
  }

  B2D_INLINE ~ZeroBuffer() noexcept {
    if (_ptr)
      ZeroAllocator::release(_ptr, _size);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint8_t* ptr() const noexcept { return _ptr; }
  B2D_INLINE size_t size() const noexcept { return _size; }

  // --------------------------------------------------------------------------
  // [Allocation]
  // --------------------------------------------------------------------------

  B2D_INLINE Error ensure(size_t requiredSize) noexcept {
    if (requiredSize <= _size)
      return kErrorOk;

    _ptr = static_cast<uint8_t*>(ZeroAllocator::realloc(_ptr, _size, requiredSize, _size));
    return _ptr ? kErrorOk : DebugUtils::errored(kErrorNoMemory);
  }

  B2D_INLINE void release() noexcept {
    if (_ptr) {
      ZeroAllocator::release(_ptr, _size);
      _ptr = nullptr;
      _size = 0;
    }
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Pointer to a buffer obtained through `ZeroAllocator`.
  uint8_t* _ptr;
  //! Size of the buffer.
  size_t _size;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZEROBUFFER_P_H
