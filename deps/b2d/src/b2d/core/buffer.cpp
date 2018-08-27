// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/buffer.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Buffer - Null]
// ============================================================================

static Wrap<BufferImpl> BufferImplNull;

// ============================================================================
// [b2d::Buffer - Impl]
// ============================================================================

BufferImpl::BufferImpl(size_t capacity, uint8_t* externalData) noexcept
  : ObjectImpl(externalData ? Any::kTypeIdBuffer | Any::kFlagIsExternal : Any::kTypeIdBuffer),
    _size(0),
    _capacity(capacity),
    _externalData(externalData) {}

BufferImpl::~BufferImpl() noexcept {}

// ============================================================================
// [b2d::Buffer - Reset]
// ============================================================================

Error Buffer::reset(uint32_t resetPolicy) noexcept {
  if (impl()->isShared() || impl()->isImmutable() || resetPolicy == Globals::kResetHard)
    return AnyInternal::replaceImpl(this, none().impl());

  impl()->_size = 0;
  return kErrorOk;
}

// ============================================================================
// [b2d::Buffer - Reserve]
// ============================================================================

Error Buffer::reserve(size_t maxSize) noexcept {
  BufferImpl* thisI = impl();

  uint8_t* srcData = thisI->data();
  size_t capacity = thisI->capacity();

  if (maxSize > Buffer::kMaxSize)
    return DebugUtils::errored(kErrorNoMemory);

  if (!thisI->isShared() && !thisI->isImmutable() && maxSize <= capacity)
    return kErrorOk;

  size_t size = thisI->size();
  maxSize = Math::max(size, maxSize);

  capacity = BufferImpl::capacityForSize(maxSize);
  BufferImpl* newI = BufferImpl::newInternal(capacity);

  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  newI->_capacity = capacity;
  newI->_size = size;

  if (size)
    std::memcpy(newI->internalData(), srcData, size);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Buffer - Modify]
// ============================================================================

uint8_t* Buffer::modify(uint32_t op, size_t maxSize) noexcept {
  BufferImpl* thisI = impl();

  uint8_t* srcData = thisI->data();
  size_t capacity = thisI->capacity();

  if (op == kContainerOpReplace) {
    if (maxSize > Buffer::kMaxSize)
      return nullptr;

    if (!thisI->isShared() && !thisI->isImmutable() && maxSize <= capacity) {
      thisI->_size = maxSize;
      return srcData;
    }

    capacity = BufferImpl::capacityForSize(maxSize);
    BufferImpl* newI = BufferImpl::newInternal(capacity);

    if (B2D_UNLIKELY(!newI))
      return nullptr;

    newI->_capacity = capacity;
    newI->_size = maxSize;

    AnyInternal::replaceImpl(this, newI);
    return newI->internalData();
  }
  else {
    size_t oldSize = thisI->size();
    size_t newSize = oldSize + maxSize;

    if (oldSize >= newSize) {
      // Overflow!
      if (maxSize != 0)
        return nullptr;

      // Zero `maxSize` - The caller CAN'T write anything into it.
      return srcData + oldSize;
    }

    if (newSize > Buffer::kMaxSize)
      return nullptr;

    if (thisI->isShared() || capacity < newSize || (op == kContainerOpConcat && (capacity - newSize) >= Globals::kMemGranularity)) {
      if (op == kContainerOpAppend)
        capacity = bDataGrow(sizeof(BufferImpl), oldSize, newSize, 1);
      else
        capacity = newSize; // TODO: Should be aligned to

      BufferImpl* newI = BufferImpl::newInternal(capacity);
      if (B2D_UNLIKELY(!newI))
        return nullptr;

      newI->_capacity = capacity;
      std::memcpy(newI->_internalData, srcData, oldSize);

      AnyInternal::replaceImpl(this, newI);
      srcData = newI->internalData();

      newI->_size = newSize;
      return srcData + oldSize;
    }
    else {
      thisI->_size = newSize;
      return srcData + oldSize;
    }
  }
}

// ============================================================================
// [b2d::Buffer - Runtime Handlers]
// ============================================================================

void BufferOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  BufferImpl* impl = new(&BufferImplNull) BufferImpl(0);
  impl->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdBuffer, impl);
}

B2D_END_NAMESPACE
