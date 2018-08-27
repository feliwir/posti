// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/runtime.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Allocator - Globals]
// ============================================================================

Allocator* Allocator::_host;

// ============================================================================
// [b2d::Allocator - Base]
// ============================================================================

Allocator::Allocator() noexcept {}
Allocator::~Allocator() noexcept {}

Allocator* Allocator::addRef() noexcept { return this; }
void Allocator::deref() noexcept {}

// ============================================================================
// [b2d::Allocator - System]
// ============================================================================

void* Allocator::systemAlloc(size_t size) noexcept {
  // Allowed by C/C++ standard, disallowed in Blend2D.
  B2D_ASSERT(size > 0);

  return std::malloc(size);
}

void* Allocator::systemRealloc(void* p, size_t size) noexcept {
  // Allowed by C/C++ standard, disallowed in Blend2D.
  B2D_ASSERT(size > 0);

  return std::realloc(p, size);
}

void Allocator::systemRelease(void* p) noexcept {
  // Never try to release a NULL pointer.
  B2D_ASSERT(p != nullptr);

  std::free(p);
}

class HostAllocator final : public Allocator {
public:
  void* _alloc(size_t size, size_t& sizeAllocated) noexcept override;
  void* _realloc(size_t* ptr, size_t oldSize, size_t newSize, size_t& sizeAllocated) noexcept override;
  void _release(void* ptr, size_t size) noexcept override;
};

void* HostAllocator::_alloc(size_t size, size_t& sizeAllocated) noexcept {
  // Not allowed by Blend2D, it's a bug in Blend2D if `size == 0`.
  B2D_ASSERT(size > 0);

  void* result = std::malloc(size);
  sizeAllocated = result ? size : size_t(0);
  return result;
}

void* HostAllocator::_realloc(size_t* ptr, size_t oldSize, size_t newSize, size_t& sizeAllocated) noexcept {
  if (oldSize == 0)
    return alloc(newSize, sizeAllocated);

  void* result = std::realloc(ptr, newSize);
  sizeAllocated = result ? newSize : oldSize;
  return result;
}

void HostAllocator::_release(void* ptr, size_t size) noexcept {
  // Not allowed by Blend2D, it's a bug in Blend2D if `ptr` is null.
  B2D_ASSERT(ptr != nullptr);

  // Not used by HostAllocator.
  B2D_UNUSED(size);

  std::free(ptr);
}

// ===========================================================================
// [b2d::Allocator - Runtime Handlers]
// ===========================================================================

static Wrap<HostAllocator> gHostAllocator;

void AllocatorOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);
  Allocator::_host = gHostAllocator.init();
}

B2D_END_NAMESPACE
