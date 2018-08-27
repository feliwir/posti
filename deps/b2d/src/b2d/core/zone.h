// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZONE_H
#define _B2D_CORE_ZONE_H

// [Dependencies]
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Zone]
// ============================================================================

//! Zone memory allocator.
//!
//! Zone is an incremental memory allocator that allocates memory by simply
//! incrementing a pointer. It allocates blocks of memory by using standard
//! C library `malloc/free`, but divides these blocks into smaller chunks
//! requested by calling `Zone::alloc()` and friends.
//!
//! Zone memory allocators are designed to allocate data of short lifetime.
//!
//! NOTE: It's not recommended to use `Zone` to allocate larger data structures
//! than the initial `blockSize` passed to `Zone()` constructor. The block size
//! should be always greater than the maximum `size` passed to `alloc()`. Zone
//! is designed to handle such cases, but it may allocate new block for each
//! call to `alloc()` that exceeds the default block size.
class Zone {
public:
  B2D_NONCOPYABLE(Zone)

  //! \internal
  //!
  //! A single block of memory managed by `Zone`.
  struct Block {
    B2D_INLINE uint8_t* data() const noexcept {
      return const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this) + sizeof(*this));
    }

    Block* prev;                         //!< Link to the previous block.
    Block* next;                         //!< Link to the next block.
    size_t size;                         //!< Size of the block.
  };

  //! \internal
  //!
  //! Saved state, used by `Zone::saveState()` and `Zone::restoreState()`.
  struct State {
    uint8_t* ptr;                        //! Current pointer.
    uint8_t* end;                        //! End pointer.
    Block* block;                        //! Current block.
  };

  enum Limits : size_t {
    kBlockSize = sizeof(Block),
    kBlockOverhead = Globals::kMemAllocOverhead + kBlockSize,

    kMinBlockSize = 64, // The number is ridiculously small, but still possible.
    kMaxBlockSize = size_t(1) << (sizeof(size_t) * 8 - 4 - 1),
    kMinAlignment = 1,
    kMaxAlignment = 64
  };

  static B2D_API const Block _zeroBlock;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new Zone.
  //!
  //! The `blockSize` parameter describes the default size of the block. If the
  //! `size` parameter passed to `alloc()` is greater than the default size
  //! `Zone` will allocate and use a larger block, but it will not change the
  //! default `blockSize`.
  //!
  //! It's not required, but it's good practice to set `blockSize` to a
  //! reasonable value that depends on the usage of `Zone`. Greater block sizes
  //! are generally safer and perform better than unreasonably low block sizes.
  B2D_INLINE explicit Zone(size_t blockSize, size_t blockAlignment = 1) noexcept {
    _init(blockSize, blockAlignment, nullptr);
  }

  B2D_INLINE Zone(size_t blockSize, size_t blockAlignment, const Support::Temporary& temporary) noexcept {
    _init(blockSize, blockAlignment, &temporary);
  }

  //! Move an existing `Zone`.
  //!
  //! NOTE: You cannot move an existing `ZoneTmp` as it uses embedded storage.
  //! Attempting to move `ZoneTmp` would result in assertion failure in debug
  //! mode and undefined behavior in release mode.
  B2D_INLINE Zone(Zone&& other) noexcept
    : _ptr(other._ptr),
      _end(other._end),
      _block(other._block),
      _packedData(other._packedData) {
    B2D_ASSERT(!other.isTemporary());
    other._block = const_cast<Block*>(&_zeroBlock);
    other._ptr = other._block->data();
    other._end = other._block->data();
  }

  //! Destroy the `Zone` instance.
  //!
  //! This will destroy the `Zone` instance and release all blocks of memory
  //! allocated by it. It performs implicit `reset(Globals::kResetHard)`.
  B2D_INLINE ~Zone() noexcept { reset(Globals::kResetHard); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  B2D_API void _init(size_t blockSize, size_t blockAlignment, const Support::Temporary* temporary) noexcept;

  //! Reset the `Zone` invalidating all blocks allocated.
  //!
  //! See `Globals::ResetPolicy` for more details.
  B2D_API void reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether this `Zone` is actually a `ZoneTmp` that uses temporary memory.
  B2D_INLINE bool isTemporary() const noexcept { return _isTemporary != 0; }

  //! Get the default block size.
  B2D_INLINE size_t blockSize() const noexcept { return _blockSize; }
  //! Get the default block alignment.
  B2D_INLINE size_t blockAlignment() const noexcept { return size_t(1) << _blockAlignmentShift; }
  //! Get remaining size of the current block.
  B2D_INLINE size_t remainingSize() const noexcept { return (size_t)(_end - _ptr); }

  //! Get the current zone cursor (dangerous).
  //!
  //! This is a function that can be used to get exclusive access to the current
  //! block's memory buffer.
  template<typename T = uint8_t>
  B2D_INLINE T* ptr() noexcept { return reinterpret_cast<T*>(_ptr); }
  //! Get the end of the current zone block, only useful if you use `ptr()`.
  template<typename T = uint8_t>
  B2D_INLINE T* end() noexcept { return reinterpret_cast<T*>(_end); }

  // NOTE: The following two functions `setPtr()` and `setEnd()` can be used
  // to perform manual memory allocation in case that an incremental allocation
  // is needed - for example you build some data structure without knowing the
  // final size. This is used for example by AnalyticRasterizer to build list
  // of edges.

  //! Set the current zone pointer to `ptr` (must be within the current block).
  template<typename T>
  B2D_INLINE void setPtr(T* ptr) noexcept {
    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    B2D_ASSERT(p >= _ptr && p <= _end);
    _ptr = p;
  }

  //! Set the end zone pointer to `end` (must be within the current block).
  template<typename T>
  B2D_INLINE void setEnd(T* end) noexcept {
    uint8_t* p = reinterpret_cast<uint8_t*>(end);
    B2D_ASSERT(p >= _ptr && p <= _end);
    _end = p;
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  //! Align the current pointer to `alignment`.
  B2D_INLINE void align(size_t alignment) noexcept {
    _ptr = std::min(Support::alignUp(_ptr, alignment), _end);
  }

  //! Ensure the remaining size is at least equal or greater than `size`.
  //!
  //! NOTE: This function doesn't respect any alignment. If you need to ensure
  //! there is enough room for an aligned allocation you need to call `align()`
  //! before calling `ensure()`.
  B2D_INLINE Error ensure(size_t size) noexcept {
    if (size <= remainingSize())
      return kErrorOk;
    else
      return _alloc(0, 1) ? kErrorOk : DebugUtils::errored(kErrorNoMemory);
  }

  B2D_INLINE void _assignBlock(Block* block) noexcept {
    size_t alignment = blockAlignment();
    _ptr = Support::alignUp(block->data(), alignment);
    _end = Support::alignDown(block->data() + block->size, alignment);
    _block = block;
  }

  B2D_INLINE void _assignZeroBlock() noexcept {
    Block* block = const_cast<Block*>(&_zeroBlock);
    _ptr = block->data();
    _end = block->data();
    _block = block;
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  //! Allocate the requested memory specified by `size`.
  //!
  //! Pointer returned is valid until the `Zone` instance is destroyed or reset
  //! by calling `reset()`. If you plan to make an instance of C++ from the
  //! given pointer use placement `new` and `delete` operators:
  //!
  //! ~~~
  //! using namespace b2d;
  //!
  //! class Object { ... };
  //!
  //! // Create Zone with default block size of approximately 65536 bytes.
  //! Zone zone(65536 - Zone::kBlockOverhead);
  //!
  //! // Create your objects using zone object allocating, for example:
  //! Object* obj = static_cast<Object*>( zone.alloc(sizeof(Object)) );
  //
  //! if (!obj) {
  //!   // Handle out of memory error.
  //! }
  //!
  //! // Placement `new` and `delete` operators can be used to instantiate it.
  //! new(obj) Object();
  //!
  //! // ... lifetime of your objects ...
  //!
  //! // To destroy the instance (if required).
  //! obj->~Object();
  //!
  //! // Reset or destroy `Zone`.
  //! zone.reset();
  //! ~~~
  B2D_INLINE void* alloc(size_t size) noexcept {
    if (B2D_UNLIKELY(size > remainingSize()))
      return _alloc(size, 1);

    uint8_t* ptr = _ptr;
    _ptr += size;
    return static_cast<void*>(ptr);
  }

  //! Allocate the requested memory specified by `size` and `alignment`.
  //!
  //! Performs the same operation as `Zone::alloc(size)` with `alignment` applied.
  B2D_INLINE void* alloc(size_t size, size_t alignment) noexcept {
    B2D_ASSERT(Support::isPowerOf2(alignment));
    uint8_t* ptr = Support::alignUp(_ptr, alignment);

    if (size > (size_t)(_end - ptr))
      return _alloc(size, alignment);

    _ptr = ptr + size;
    return static_cast<void*>(ptr);
  }

  //! Allocate the requested memory specified by `size` without doing any checks.
  //!
  //! Can only be called if `remainingSize()` returns size at least equal to `size`.
  B2D_INLINE void* allocNoCheck(size_t size) noexcept {
    B2D_ASSERT(remainingSize() >= size);

    uint8_t* ptr = _ptr;
    _ptr += size;
    return static_cast<void*>(ptr);
  }

  //! Allocate the requested memory specified by `size` and `alignment` without doing any checks.
  //!
  //! Performs the same operation as `Zone::allocNoCheck(size)` with `alignment` applied.
  B2D_INLINE void* allocNoCheck(size_t size, size_t alignment) noexcept {
    B2D_ASSERT(Support::isPowerOf2(alignment));

    uint8_t* ptr = Support::alignUp(_ptr, alignment);
    B2D_ASSERT(size <= (size_t)(_end - ptr));

    _ptr = ptr + size;
    return static_cast<void*>(ptr);
  }

  //! Allocate the requested memory specified by `size` and `alignment` and zero
  //! it before returning its pointer.
  //!
  //! See \ref alloc() for more details.
  B2D_API void* allocZeroed(size_t size, size_t alignment = 1) noexcept;

  //! Like `alloc()`, but the return pointer is casted to `T*`.
  template<typename T>
  B2D_INLINE T* allocT(size_t size = sizeof(T), size_t alignment = alignof(T)) noexcept {
    return static_cast<T*>(alloc(size, alignment));
  }

  //! Like `allocNoCheck()`, but the return pointer is casted to `T*`.
  template<typename T>
  B2D_INLINE T* allocNoCheckT(size_t size = sizeof(T), size_t alignment = alignof(T)) noexcept {
    return static_cast<T*>(allocNoCheck(size, alignment));
  }

  //! Like `allocZeroed()`, but the return pointer is casted to `T*`.
  template<typename T>
  B2D_INLINE T* allocZeroedT(size_t size = sizeof(T), size_t alignment = alignof(T)) noexcept {
    return static_cast<T*>(allocZeroed(size, alignment));
  }

  //! Like `new(std::nothrow) T(...)`, but allocated by `Zone`.
  template<typename T>
  B2D_INLINE T* newT() noexcept {
    void* p = alloc(sizeof(T), alignof(T));
    if (B2D_UNLIKELY(!p))
      return nullptr;
    return new(p) T();
  }

  //! Like `new(std::nothrow) T(...)`, but allocated by `Zone`.
  template<typename T, typename... ArgsT>
  B2D_INLINE T* newT(ArgsT&&... args) noexcept {
    void* p = alloc(sizeof(T), alignof(T));
    if (B2D_UNLIKELY(!p))
      return nullptr;
    return new(p) T(std::forward<ArgsT>(args)...);
  }

  //! \internal
  B2D_API void* _alloc(size_t size, size_t alignment) noexcept;

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Store the current state to `state`.
  B2D_INLINE void saveState(State* state) noexcept {
    state->ptr = _ptr;
    state->end = _end;
    state->block = _block;
  }

  //! Restore the state of `Zone` to the previously saved `State`.
  B2D_INLINE void restoreState(State* state) noexcept {
    Block* block = state->block;
    _ptr = state->ptr;
    _end = state->end;
    _block = block;
  }

  // --------------------------------------------------------------------------
  // [Swap]
  // --------------------------------------------------------------------------

  B2D_INLINE void swapWith(Zone& other) noexcept {
    // This could lead to a disaster.
    B2D_ASSERT(!this->isTemporary());
    B2D_ASSERT(!other.isTemporary());

    std::swap(_ptr, other._ptr);
    std::swap(_end, other._end);
    std::swap(_block, other._block);
    std::swap(_packedData, other._packedData);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t* _ptr;                         //!< Pointer in the current block.
  uint8_t* _end;                         //!< End of the current block.
  Block* _block;                         //!< Current block.

  #if B2D_ARCH_BITS >= 64
  union {
    struct {
      size_t _blockSize : 60;            //!< Default block size.
      size_t _isTemporary : 1;           //!< First block is temporary (ZoneTmp).
      size_t _blockAlignmentShift : 3;   //!< Block alignment (1 << alignment).
    };
    size_t _packedData;
  };
  #else
  union {
    struct {
      size_t _blockSize : 28;            //!< Default block size.
      size_t _isTemporary : 1;           //!< First block is temporary (ZoneTmp).
      size_t _blockAlignmentShift : 3;   //!< Block alignment (1 << alignment).
    };
    size_t _packedData;
  };
  #endif
};

// ============================================================================
// [b2d::ZoneTmp]
// ============================================================================

template<size_t N>
class ZoneTmp : public Zone {
public:
  B2D_NONCOPYABLE(ZoneTmp<N>)

  B2D_INLINE explicit ZoneTmp(size_t blockSize, size_t blockAlignment = 1) noexcept
    : Zone(blockSize, blockAlignment, Support::Temporary(_storage.data, N)) {}

  struct Storage {
    char data[N];
  } _storage;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZONE_H
