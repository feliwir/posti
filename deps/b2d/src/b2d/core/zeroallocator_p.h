// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZEROALLOCATOR_P_H
#define _B2D_CORE_ZEROALLOCATOR_P_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ZeroAllocator - Public API]
// ============================================================================

//! \internal
namespace ZeroAllocator {
  void* alloc(size_t size, size_t& allocatedSize) noexcept;
  void* realloc(void* oldPtr, size_t oldSize, size_t newSize, size_t& allocatedSize) noexcept;
  void release(void* ptr, size_t size) noexcept;
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZEROALLOCATOR_P_H
