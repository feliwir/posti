// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_STRING_H
#define _B2D_CORE_STRING_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Utf8String]
// ============================================================================

//! A simple non-reference counted string that uses small string optimization (SSO).
//!
//! This string has 3 allocation possibilities:
//!
//!   1. Small    - embedded buffer is used for up to `kSSOCapacity` characters.
//!                 This should handle most small strings to avoid dynamic memory
//!                 allocation.
//!
//!   2. Large    - string that doesn't fit into an embedded buffer (or string
//!                 that was truncated from a larger buffer) and is owned by Blend2D.
//!                 When you destroy the string Blend2D would automatically release
//!                 the large buffer.
//!
//!   3. External - like Large (2), however, the large buffer is not owned by
//!                 Blend2D and won't be released when the string is destroyed or
//!                 reallocated. This is mostly useful for working with larger
//!                 temporary strings allocated on stack or with immutable strings.
class Utf8String {
public:
  B2D_NONCOPYABLE(Utf8String)

  typedef uintptr_t MachineWord;

  enum : uint32_t {
    kLayoutSize = uint32_t(sizeof(MachineWord)) * 4u,
    kSSOCapacity = kLayoutSize - 2
  };

  enum Tag : uint8_t {
    // Designed to have the 'sign' bit set for large/exteral strings.
    kTagLarge    = 0x80u, // External string that is owned by Blend2D.
    kTagExternal = 0xFFu  // External string that is not owned and Blend2D won't release it.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Utf8String() noexcept {
    _resetToEmbedded();
  }

  B2D_INLINE Utf8String(Utf8String&& other) noexcept {
    for (size_t i = 0; i < B2D_ARRAY_SIZE(_rawWords); i++)
      _rawWords[i] = other._rawWords[i];
    other._resetToEmbedded();
  }

  B2D_INLINE ~Utf8String() noexcept {
    reset(Globals::kResetHard);
  }

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

protected:
  //! Resets string to embedded and makes it empty (zero length, zero first char)
  //!
  //! NOTE: This is always called internally after an external buffer was released.
  B2D_INLINE void _resetToEmbedded() noexcept {
    // 4 machine words, optimizing compilers should use SIMD stores.
    for (size_t i = 0; i < B2D_ARRAY_SIZE(_rawWords); i++)
      _rawWords[i] = 0;
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

public:
  //! Reset the string size to zero and null terminate.
  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isLarge() const noexcept { return _small.sizeOrTag >= kTagLarge; }
  B2D_INLINE bool isExternal() const noexcept { return _small.sizeOrTag == kTagExternal; }

  B2D_INLINE bool empty() const noexcept { return size() == 0; }
  B2D_INLINE size_t size() const noexcept { return isLarge() ? size_t(_large.size) : size_t(_small.sizeOrTag); }
  B2D_INLINE size_t capacity() const noexcept { return isLarge() ? _large.capacity : size_t(kSSOCapacity); }

  B2D_INLINE char* data() noexcept { return isLarge() ? _large.data : _small.data; }
  B2D_INLINE const char* data() const noexcept { return isLarge() ? _large.data : _small.data; }

  B2D_INLINE char* end() noexcept { return data() + size(); }
  B2D_INLINE const char* end() const noexcept { return data() + size(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API Error _create(const char* data, size_t size) noexcept;

  B2D_INLINE Error assign(const char* data, size_t size = Globals::kNullTerminated) noexcept {
    return _create(data, size);
  }

  B2D_INLINE Error resize(size_t newSize) noexcept {
    return _create(nullptr, newSize);
  }

  B2D_API Error truncate(size_t newSize) noexcept;

  B2D_INLINE bool eq(const char* other) const noexcept {
    return std::strcmp(data(), other) == 0;
  }

  B2D_INLINE bool eq(const char* other, size_t size) const noexcept {
    return this->size() == size && std::memcmp(data(), other, size) == 0;
  }

  B2D_INLINE bool eq(const Utf8String& other) const noexcept {
    return eq(other.data(), other.size());
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE bool operator==(const char* other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const char* other) const noexcept { return !eq(other); }

  B2D_INLINE bool operator==(const Utf8String& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Utf8String& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  struct Small {
    char data[kSSOCapacity + 1u];
    uint8_t sizeOrTag;
  };

  struct Large {
    char* data;
    size_t capacity;
    size_t size;
  };

  union {
    Small _small;
    Large _large;
    MachineWord _rawWords[kLayoutSize / sizeof(MachineWord)];
  };
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_STRING_H
