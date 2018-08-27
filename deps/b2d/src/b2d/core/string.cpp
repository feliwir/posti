// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/string.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Utf8String]
// ============================================================================

Error Utf8String::_create(const char* data, size_t size) noexcept {
  char* dst = nullptr;

  // Null terminated string without `size` specified.
  if (size == Globals::kNullTerminated)
    size = std::strlen(data);

  if (isLarge()) {
    if (size <= _large.capacity) {
      dst = _large.data;
      _large.size = size;
    }
    else {
      size_t capacityPlusOne = Support::alignUp(size + 1, 32);
      if (B2D_UNLIKELY(capacityPlusOne < size))
        return DebugUtils::errored(kErrorNoMemory);

      dst = static_cast<char*>(Allocator::systemAlloc(capacityPlusOne));
      if (B2D_UNLIKELY(!dst))
        return DebugUtils::errored(kErrorNoMemory);

      if (!isExternal())
        Allocator::systemRelease(_large.data);

      _large.data = dst;
      _large.size = size;
      _large.capacity = capacityPlusOne - 1;
      _small.sizeOrTag = kTagLarge;
    }
  }
  else {
    if (size <= kSSOCapacity) {
      B2D_ASSERT(size < 0xFFu);

      dst = _small.data;
      _small.sizeOrTag = uint8_t(size);
    }
    else {
      dst = static_cast<char*>(Allocator::systemAlloc(size + 1));
      if (B2D_UNLIKELY(!dst))
        return DebugUtils::errored(kErrorNoMemory);

      _large.data = dst;
      _large.size = size;
      _large.capacity = size;
      _small.sizeOrTag = kTagLarge;
    }
  }

  // Optionally copy data from `data` and null-terminate.
  if (data && size) {
    // NOTE: It's better to use `memmove()`. If, for any reason, somebody uses
    // this function to substring the same string it would work as expected.
    std::memmove(dst, data, size);
  }

  dst[size] = '\0';
  return kErrorOk;
}

Error Utf8String::reset(uint32_t resetPolicy) noexcept {
  if (isLarge()) {
    if (resetPolicy == Globals::kResetHard) {
      if (!isExternal())
        Allocator::systemRelease(_large.data);
      _resetToEmbedded();
    }
    else {
      _large.data[0] = '\0';
      _large.size = 0;
    }
  }
  else {
    _resetToEmbedded();
  }

  return kErrorOk;
}

Error Utf8String::truncate(size_t newSize) noexcept {
  if (isLarge()) {
    if (newSize < _large.size) {
      _large.data[newSize] = '\0';
      _large.size = newSize;
    }
  }
  else {
    if (newSize < _small.sizeOrTag) {
      _small.data[newSize] = '\0';
      _small.sizeOrTag = uint8_t(newSize);
    }
  }

  return kErrorOk;
}

B2D_END_NAMESPACE
