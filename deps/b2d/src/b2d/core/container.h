// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_CONTAINER_H
#define _B2D_CORE_CONTAINER_H

// [Dependencies]
#include "../core/any.h"
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ContainerOp]
// ============================================================================

//! Container operation.
enum ContainerOp : uint32_t {
  //! Replace operation.
  //!
  //! When this operation mode is used it reserves size that is only needed to
  //! hold the required data. If the container is not shared and larger space
  //! than required is allocated it keeps it as is (it won't reallocate it to
  //! a smaller size).
  kContainerOpReplace = 0,

  //! Append operation (consecutive).
  //!
  //! When this operation mode is used it tries to prepare container for a
  //! consecutive appending by reserving much more memory than required. Every
  //! container should have a method called `compact()` that can be used to
  //! shrink the data allocated into the minimum possible space.
  //!
  //! Consider using `kContainerOpConcat` if this behavior is unwanted.
  kContainerOpAppend = 1,

  //! Append operation (concatenate).
  //!
  //! When this operation mode is used it finalizes the container size to its
  //! current size plus the required size for the append operation. The data of
  //! the container is always reallocated in case that the extra reserved memory
  //! is so large. Using this mode is just an optimized way of performing
  //! `append()` followed by `compact()`.
  //!
  //! Consider using `kContainerOpAppend` if this behavior is unwanted.
  kContainerOpConcat = 2
};

// ============================================================================
// [b2d::bDataGrow]
// ============================================================================

// TODO: Move to support

// TODO: Would be better:
//   grow(size_t implSize, size_t typeSize, size_t before, size_t after);
B2D_API size_t bDataGrow(size_t headerSize, size_t before, size_t after, size_t szT);

//! Get a theoretical maximum size of a container.
//!
//! This function is provided for convenience. Blend always uses theoretical
//! limits to prevent arithmetic overflows, but as the name implies, even if
//! theoretical limit is fine it is not guaranteed that the operating system
//! will be able to allocate a continuous block of a given size.
static B2D_INLINE size_t bDataMaxSize(size_t headerSize, size_t elementSize) {
  return (std::numeric_limits<size_t>::max() / 2 - headerSize) / elementSize;
}

// ============================================================================
// [b2d::Range]
// ============================================================================

//! Range is a small data structure that contains start and end indexes. It's
//! used it to specify a range in indexed containers like `Array`, `Gradient`,
//! `Path2D` or others.
//!
//! NOTE: Indexes used by `Range` are, like all other indexes used by Blend2D,
//! unsigned integers of `size_t` type.
struct Range {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create an uninitialized range.
  inline Range() noexcept = default;

  //! Create a copy of `other` range.
  constexpr Range(const Range& other) noexcept = default;

  //! Create a range from `start` and `end` indexes.
  constexpr explicit Range(size_t start, size_t end = Globals::kInvalidIndex) noexcept
    : _start(start),
      _end(end) {}

  static constexpr Range everything() noexcept { return Range { 0, Globals::kInvalidIndex }; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Reset range by setting both start and end indexes to `start` and `end`.
  B2D_INLINE void reset(size_t start, size_t end = Globals::kInvalidIndex) noexcept {
    _start = start;
    _end = end;
  }

  //! Get whether the range is valid (`_start` must be lesser than `_end`).
  constexpr bool isValid() const noexcept { return _start < _end; }

  //! Get the start position.
  constexpr size_t start() const noexcept { return _start; }
  //! Get the end position.
  constexpr size_t end() const noexcept { return _end; }
  //! Get the range size or zero if the range is empty/invalid.
  B2D_INLINE size_t size() const noexcept { return _end - Math::min<size_t>(_start, _end); }

  //! Calculate range size without checking for correct data.
  //!
  //! This function is used internally in places where we know that the size
  //! is zero or larger. There is assertion so if this method is used inproperly
  //! then assertion-failure is shown.
  B2D_INLINE size_t _size() const noexcept {
    B2D_ASSERT(_start <= _end);
    return _end - _start;
  }

  //! Set start index.
  B2D_INLINE void setStart(size_t start) noexcept { _start = start; }
  //! Set end index.
  B2D_INLINE void setEnd(size_t end) noexcept { _end = end; }

  // --------------------------------------------------------------------------
  // [Fit]
  // --------------------------------------------------------------------------

  B2D_INLINE bool fit(size_t size, size_t& rIdx, size_t& rEnd) const noexcept {
    rIdx = _start;
    rEnd = Math::min(_end, size);
    return rIdx < rEnd;
  }

  B2D_INLINE bool eq(const Range& other) const noexcept {
    return (_start == other._start) &
           (_end   == other._end  ) ;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Range& operator=(const Range& other) noexcept = default;

  B2D_INLINE bool operator==(const Range& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Range& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _start;
  size_t _end;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_CONTAINER_H
