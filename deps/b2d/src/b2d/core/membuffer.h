// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_MEMBUFFER_H
#define _B2D_CORE_MEMBUFFER_H

// [Dependencies]
#include "../core/allocator.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::MemBuffer]
// ============================================================================

//! Memory buffer.
//!
//! Memory buffer is a helper class which holds pointer to an allocated memory
//! block, which will be released automatically by `MemBuffer` destructor or by
//! `reset()` call.
class MemBuffer {
public:
  B2D_NONCOPYABLE(MemBuffer)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE MemBuffer() noexcept
    : _mem(nullptr),
      _buf(nullptr),
      _capacity(0) {}

  B2D_INLINE ~MemBuffer() noexcept {
    _reset();
  }

protected:
  B2D_INLINE MemBuffer(void* mem, void* buf, size_t capacity) noexcept
    : _mem(mem),
      _buf(buf),
      _capacity(capacity) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

public:
  B2D_INLINE void* get() const noexcept { return _mem; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  // --------------------------------------------------------------------------
  // [Alloc / Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void* alloc(size_t size) noexcept {
    if (size <= _capacity)
      return _mem;

    if (_mem != _buf)
      Allocator::systemRelease(_mem);

    _mem = Allocator::systemAlloc(size);
    _capacity = size;

    return _mem;
  }

  B2D_INLINE void _reset() noexcept {
    if (_mem != _buf)
      Allocator::systemRelease(_mem);
  }

  B2D_INLINE void reset() noexcept {
    _reset();

    _mem = nullptr;
    _capacity = 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  void* _mem;
  void* _buf;
  size_t _capacity;
};

// ============================================================================
// [b2d::MemBufferTmp<>]
// ============================================================================

//! Memory buffer (temporary).
//!
//! This template is for fast routines that need to use memory  allocated on
//! the stack, but the memory requirement is not known at compile time. The
//! number of bytes allocated on the stack is described by `N` parameter.
template<size_t N>
class MemBufferTmp : public MemBuffer {
public:
  B2D_NONCOPYABLE(MemBufferTmp<N>)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE MemBufferTmp() noexcept
    : MemBuffer(_storage, _storage, N) {}

  B2D_INLINE ~MemBufferTmp() noexcept {}

  // --------------------------------------------------------------------------
  // [Alloc / Reset]
  // --------------------------------------------------------------------------

  using MemBuffer::alloc;

  B2D_INLINE void reset() noexcept {
    _reset();
    _mem = _buf;
    _capacity = N;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _storage[N];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_MEMBUFFER_H
