// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ALLOCATOR_H
#define _B2D_CORE_ALLOCATOR_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Allocator]
// ============================================================================

//! Memory allocator.
class Allocator {
public:
  B2D_NONCOPYABLE(Allocator)

  B2D_API Allocator() noexcept;
  B2D_API virtual ~Allocator() noexcept;

  // --------------------------------------------------------------------------
  // [Alloc / Release]
  // --------------------------------------------------------------------------

  //! Try to allocate a memory chunk of a given `size` and return pointer to it
  //! on success. On failure `nullptr` is returned.
  B2D_INLINE void* alloc(size_t size) noexcept {
    size_t sizeAllocated;
    return _alloc(size, sizeAllocated);
  }

  //! Try to allocate a memory chunk of a given `size` and return pointer to it
  //! on success. On failure `nullptr` is returned.
  //!
  //! This is an overloaded function that would also report back the exact size
  //! of the allocated memory through `sizeAllocated`. System allocators would
  //! always report the same `size` as passed to `alloc()`, however, custom
  //! allocators and memory pools can round the size and report greater size
  //! allocated than requested. It's up to the caller if it can take advantage
  //! of the size reported.
  B2D_INLINE void* alloc(size_t size, size_t& sizeAllocated) noexcept {
    return _alloc(size, sizeAllocated);
  }

  //! Reallocate the given chunk `ptr` of `oldSize` into a new location of `newSize`.
  B2D_INLINE void* realloc(size_t* ptr, size_t oldSize, size_t newSize) noexcept {
    size_t sizeAllocated;
    return _realloc(ptr, oldSize, newSize, sizeAllocated);
  }

  //! \overload
  B2D_INLINE void* realloc(size_t* ptr, size_t oldSize, size_t newSize, size_t& sizeAllocated) noexcept {
    return _realloc(ptr, oldSize, newSize, sizeAllocated);
  }

  //! Release previously allocated memory through `alloc()` and `realloc()`.
  //!
  //! NOTE: The `size` passed to release must be equal to the `size` passed to
  //! `alloc()` or `realloc()`, or equal to the size it returned via `sizeAllocated`.
  B2D_INLINE void release(void* ptr, size_t size) noexcept {
    return _release(ptr, size);
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual Allocator* addRef() noexcept;
  B2D_API virtual void deref() noexcept;

  virtual void* _alloc(size_t size, size_t& sizeAllocated) noexcept = 0;
  virtual void* _realloc(size_t* ptr, size_t oldSize, size_t newSize, size_t& sizeAllocated) noexcept = 0;
  virtual void _release(void* ptr, size_t size) noexcept = 0;

  // --------------------------------------------------------------------------
  // [System]
  // --------------------------------------------------------------------------

  //! Static function compatible with `Allocator::host()->alloc()`.
  static B2D_API void* systemAlloc(size_t size) noexcept;
  //! Static function compatible with `Allocator::host()->realloc()`.
  static B2D_API void* systemRealloc(void* p, size_t size) noexcept;
  //! Static function compatible with `Allocator::host()->release()`.
  static B2D_API void systemRelease(void* p) noexcept;

  //! \internal
  static B2D_API Allocator* _host;
  //! Get a host allocator that internally uses `std::malloc()` and friends.
  static B2D_INLINE Allocator* host() noexcept { return _host; }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ALLOCATOR_H
