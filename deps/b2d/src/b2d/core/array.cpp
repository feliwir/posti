// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/array.h"
#include "../core/math.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ===========================================================================
// [b2d::Array - Globals]
// ===========================================================================

static constexpr uint32_t kArrayBuiltInFlags = Any::kFlagIsNone | Any::kFlagIsArray;

static const constexpr ArrayImpl ArrayImpl_none[] = {
  { Any::kTypeIdArrayOfAny        | kArrayBuiltInFlags, sizeof(AnyBase) },
  { Any::kTypeIdArrayOfObject     | kArrayBuiltInFlags, sizeof(Object)  },
  { Any::kTypeIdArrayOfPtrT       | kArrayBuiltInFlags, sizeof(void*)   },
  { Any::kTypeIdArrayOfInt8       | kArrayBuiltInFlags, 1               },
  { Any::kTypeIdArrayOfUInt8      | kArrayBuiltInFlags, 1               },
  { Any::kTypeIdArrayOfInt16      | kArrayBuiltInFlags, 2               },
  { Any::kTypeIdArrayOfUInt16     | kArrayBuiltInFlags, 2               },
  { Any::kTypeIdArrayOfInt32      | kArrayBuiltInFlags, 4               },
  { Any::kTypeIdArrayOfUInt32     | kArrayBuiltInFlags, 4               },
  { Any::kTypeIdArrayOfInt64      | kArrayBuiltInFlags, 8               },
  { Any::kTypeIdArrayOfUInt64     | kArrayBuiltInFlags, 8               },
  { Any::kTypeIdArrayOfFloat32    | kArrayBuiltInFlags, 4               },
  { Any::kTypeIdArrayOfFloat64    | kArrayBuiltInFlags, 8               },
  { Any::kTypeIdArrayOfTrivial1   | kArrayBuiltInFlags, 1               },
  { Any::kTypeIdArrayOfTrivial2   | kArrayBuiltInFlags, 2               },
  { Any::kTypeIdArrayOfTrivial4   | kArrayBuiltInFlags, 4               },
  { Any::kTypeIdArrayOfTrivial6   | kArrayBuiltInFlags, 6               },
  { Any::kTypeIdArrayOfTrivial8   | kArrayBuiltInFlags, 8               },
  { Any::kTypeIdArrayOfTrivial12  | kArrayBuiltInFlags, 12              },
  { Any::kTypeIdArrayOfTrivial16  | kArrayBuiltInFlags, 16              },
  { Any::kTypeIdArrayOfTrivial20  | kArrayBuiltInFlags, 20              },
  { Any::kTypeIdArrayOfTrivial24  | kArrayBuiltInFlags, 24              },
  { Any::kTypeIdArrayOfTrivial28  | kArrayBuiltInFlags, 28              },
  { Any::kTypeIdArrayOfTrivial32  | kArrayBuiltInFlags, 32              },
  { Any::kTypeIdArrayOfTrivial48  | kArrayBuiltInFlags, 48              },
  { Any::kTypeIdArrayOfTrivial64  | kArrayBuiltInFlags, 64              }
};

// ===========================================================================
// [b2d::Array - Utilities]
// ===========================================================================

static B2D_INLINE void Array_copyToUninitialized(void* dstData, const void* srcData, size_t count, uint32_t itemSize, bool isTrivial) noexcept {
  B2D_ASSERT(count > 0);

  if (isTrivial) {
    std::memcpy(dstData, srcData, count * itemSize);
  }
  else {
    AnyBase* dst = static_cast<AnyBase*>(dstData);
    const AnyBase* src = static_cast<const AnyBase*>(srcData);

    for (size_t i = 0; i < count; i++)
      dst[i]._impl = AnyInternal::addRef(src[i].impl());
  }
}

static ArrayImpl* Array_reallocImpl(ArrayImpl* impl, uint32_t itemSize, size_t newCapacity) noexcept {
  B2D_ASSERT(newCapacity >= impl->size());

  Support::OverflowFlag of;
  size_t implSize = Support::safeMulAdd<size_t>(sizeof(ArrayImpl), newCapacity, itemSize, &of);

  if (B2D_UNLIKELY(of))
    return nullptr;

  if (impl->isTemporary()) {
    ArrayImpl* newI = ArrayImpl::new_(impl->typeId(), itemSize, newCapacity);
    if (B2D_UNLIKELY(!newI))
      return nullptr;

    size_t size = impl->size();
    newI->_size = size;

    if (size)
      std::memcpy(newI->data(), impl->data(), size * itemSize);

    // This should not be necessary as temporary data would just be abandoned
    // by `~ArrayTmp()`, however, there are also asserts that make sure that
    // the static data allocated by `ArrayTmp` are not shared when it's going
    // to be destroyed.
    impl->_size = 0;
    impl->deref();

    return newI;
  }
  else {
    impl = static_cast<ArrayImpl*>(Allocator::systemRealloc(impl, implSize));
    if (B2D_UNLIKELY(!impl))
      return nullptr;

    impl->_capacity = newCapacity;
    return impl;
  }
}

// ===========================================================================
// [b2d::Array - Any Interface]
// ===========================================================================

B2D_API Error ArrayBase::_detach() noexcept {
  ArrayImpl* thisI = impl();
  if (!thisI->isShared())
    return kErrorOk;

  size_t size = thisI->size();
  size_t capacity = Math::max<size_t>(thisI->size(), ArrayBase::kInitSize);

  ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), thisI->itemSize(), capacity);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  if (size) {
    newI->_size = size;
    Array_copyToUninitialized(newI->data(), thisI->data(), size, thisI->itemSize(), thisI->hasTrivialData());
  }

  return AnyInternal::replaceImpl(this, newI);
}

// ===========================================================================
// [b2d::Array - Operations]
// ===========================================================================

Error ArrayBase::reset(uint32_t resetPolicy) noexcept {
  ArrayImpl* thisI = impl();
  if (thisI->isShared() || resetPolicy == Globals::kResetHard)
    return AnyInternal::replaceImpl(this, Any::noneByTypeId(impl()->typeId()).impl());

  if (!thisI->hasTrivialData())
    AnyInternal::releaseAnyArray(thisI->data<AnyBase>(), thisI->size());

  thisI->_size = 0;
  return kErrorOk;
}

Error ArrayBase::_resize(size_t capacity, bool zeroInitialize) noexcept {
  size_t size = impl()->size();

  if (capacity <= size) {
    B2D_PROPAGATE(_removeRange(capacity, size));
    return detach();
  }

  size_t inc = capacity - size;
  void* p = _commonOp(kContainerOpConcat, inc);
  if (B2D_UNLIKELY(!p))
    return DebugUtils::errored(kErrorNoMemory);

  ArrayImpl* thisI = impl();
  if (zeroInitialize)
    std::memset(p, 0, inc * thisI->itemSize());

  return kErrorOk;
}

Error ArrayBase::reserve(size_t capacity) noexcept {
  ArrayImpl* thisI = impl();

  size_t size = thisI->size();
  size_t rc = thisI->refCount();

  capacity = Math::max(capacity, size);
  if (rc != 1 || capacity > thisI->capacity()) {
    ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), thisI->itemSize(), capacity);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    if (size) {
      newI->_size = size;

      // Prefer move over copy if `thisI` is not shared.
      bool isTrivial = thisI->hasTrivialData();
      if (rc == 1) {
        thisI->_size = 0;
        isTrivial = true;
      }

      Array_copyToUninitialized(newI->data(), thisI->data(), size, newI->itemSize(), isTrivial);
    }

    return AnyInternal::replaceImpl(this, newI);
  }

  return kErrorOk;
}

Error ArrayBase::compact() noexcept {
  ArrayImpl* thisI = impl();

  if (thisI->isTemporary())
    return kErrorOk;

  size_t size = thisI->size();
  size_t rc = thisI->refCount();

  if (thisI->capacity() > size) {
    ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), thisI->itemSize(), size);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    if (size) {
      newI->_size = size;

      // Prefer move over copy if `thisI` is not shared.
      bool isTrivial = thisI->hasTrivialData();
      if (rc == 1) {
        thisI->_size = 0;
        isTrivial = true;
      }

      Array_copyToUninitialized(newI->data(), thisI->data(), size, newI->itemSize(), isTrivial);
    }

    return AnyInternal::replaceImpl(this, newI);
  }

  return kErrorOk;
}

Error ArrayBase::_copy(const void* data, size_t size) noexcept {
  if (!size)
    return clear();

  ArrayImpl* thisI = impl();
  if (thisI->hasTrivialData()) {
    uint32_t itemSize = thisI->itemSize();
    size_t byteSize = size * itemSize;

    if (thisI->isShared()) {
      ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), itemSize, size);
      if (B2D_UNLIKELY(!newI))
        return DebugUtils::errored(kErrorNoMemory);

      std::memcpy(newI->data(), data, byteSize);

      newI->_size = size;
      return AnyInternal::replaceImpl(this, newI);
    }
    else {
      // Could be in the same buffer as `thisI` (slicing) so `memmove()` must be used.
      std::memmove(thisI->data(), data, byteSize);

      thisI->_size = size;
      return kErrorOk;
    }
  }
  else {
    if (thisI->isShared()) {
      ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), sizeof(AnyBase), size);
      if (B2D_UNLIKELY(!newI))
        return DebugUtils::errored(kErrorNoMemory);

      Array_copyToUninitialized(newI->data(), data, size, sizeof(AnyBase), false);

      newI->_size = size;
      return AnyInternal::replaceImpl(this, newI);
    }
    else {
      // We MUST check whether this is a slice operation or a regular copy.
      const AnyBase* srcPtr = static_cast<const AnyBase*>(data);
      const AnyBase* srcEnd = srcPtr + size;

      AnyBase* dstPtr = thisI->data<AnyBase>();
      AnyBase* dstEnd = dstPtr + thisI->size();

      if (srcEnd <= dstPtr || srcPtr >= dstEnd) {
        // Copy operation.
        if (thisI->size())
          AnyInternal::releaseAnyArray(dstPtr, thisI->size());

        Array_copyToUninitialized(dstPtr, srcPtr, size, sizeof(AnyBase), false);

        thisI->_size = size;
        return kErrorOk;
      }
      else {
        // Slice operation, note that it cannot just partially intersect the data.
        B2D_ASSERT(srcPtr >= dstPtr && srcEnd <= dstEnd);

        size_t sliceIdx = (size_t)(srcPtr - dstPtr);
        size_t tailSize = (size_t)(dstEnd - srcEnd);

        if (sliceIdx) {
          AnyInternal::releaseAnyArray(dstPtr, sliceIdx);
          std::memmove(dstPtr, srcPtr, size * sizeof(AnyBase));
        }

        if (tailSize)
          AnyInternal::releaseAnyArray(dstPtr + sliceIdx + size, tailSize);

        thisI->_size = size;
        return kErrorOk;
      }
    }
  }
}

void* ArrayBase::_commonOp(uint32_t op, size_t opSize) noexcept {
  ArrayImpl* thisI = impl();

  size_t thisSize = thisI->size();
  uint32_t itemSize = thisI->itemSize();

  if (op == kContainerOpReplace) {
    if (thisI->isShared() || opSize > thisI->capacity()) {
      ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), itemSize, opSize);
      if (B2D_UNLIKELY(!newI))
        return nullptr;

      AnyInternal::replaceImpl(this, newI);
      newI->_size = opSize;
      return newI->data();
    }
    else {
      AnyInternal::releaseAnyArray(thisI->data<AnyBase>(), thisI->size());
      thisI->_size = opSize;
      return thisI->data();
    }
  }
  else {
    Support::OverflowFlag of;
    size_t finalSize = Support::safeAdd(thisSize, opSize, &of);

    if (B2D_UNLIKELY(of))
      return nullptr;

    size_t newCapacity =
      op == kContainerOpAppend
        ? bDataGrow(sizeof(ArrayImpl), thisSize, finalSize, itemSize)
        : finalSize;

    if (thisI->isShared()) {
      ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), itemSize, newCapacity);

      if (B2D_UNLIKELY(!newI))
        return nullptr;

      if (thisSize)
        Array_copyToUninitialized(newI->data(), thisI->data(), thisSize, itemSize, thisI->hasTrivialData());

      AnyInternal::replaceImpl(this, newI);
      thisI = newI;
    }
    else if (finalSize > thisI->capacity()) {
      thisI = Array_reallocImpl(thisI, itemSize, newCapacity);

      if (B2D_UNLIKELY(!thisI))
        return nullptr;

      _impl = thisI;
    }

    thisI->_size = finalSize;
    return thisI->data<uint8_t>() + thisSize * itemSize;
  }
}

void* ArrayBase::_insertOp(size_t index, size_t opSize) noexcept {
  ArrayImpl* thisI = impl();

  size_t thisSize = thisI->size();
  uint32_t itemSize = thisI->itemSize();

  Support::OverflowFlag of;
  size_t finalSize = Support::safeAdd(thisSize, opSize, &of);

  if (B2D_UNLIKELY(of))
    return nullptr;

  index = Math::min(index, thisSize);
  size_t rc = thisI->refCount();

  size_t byteIndex = index * itemSize;
  size_t moveCount = thisSize - index;

  if (rc != 1 || finalSize > thisI->capacity()) {
    size_t newCapacity = bDataGrow(sizeof(ArrayImpl), thisSize, finalSize, itemSize);
    ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), itemSize, newCapacity);

    if (B2D_UNLIKELY(!newI))
      return nullptr;

    uint8_t* newData = newI->data<uint8_t>();
    uint8_t* oldData = thisI->data<uint8_t>();

    // Prefer move over copy if `thisI` is not shared.
    bool isTrivial = thisI->hasTrivialData();
    if (rc == 1) {
      thisI->_size = 0;
      isTrivial = true;
    }

    if (byteIndex)
      Array_copyToUninitialized(newData, oldData, index, itemSize, isTrivial);

    if (moveCount)
      Array_copyToUninitialized(newData + byteIndex + opSize * itemSize, oldData + byteIndex, index, itemSize, isTrivial);

    newI->_size = finalSize;
    _impl = newI;

    thisI->release();
    return newData + byteIndex;
  }
  else {
    uint8_t* thisData = thisI->data<uint8_t>();

    if (moveCount)
      std::memmove(thisData + byteIndex + opSize * itemSize, thisData + byteIndex, moveCount * itemSize);

    thisI->_size = finalSize;
    return thisData + byteIndex;
  }
}

Error ArrayBase::_removeRange(size_t rIdx, size_t rEnd) noexcept {
  ArrayImpl* thisI = impl();
  size_t thisSize = thisI->size();

  rEnd = Math::min(rEnd, thisSize);
  rIdx = Math::min(rIdx, rEnd);

  size_t rangeSize = rEnd - rIdx;
  uint32_t itemSize = thisI->itemSize();

  if (!rangeSize)
    return kErrorOk;

  bool isTrivial = thisI->hasTrivialData();
  size_t tailSize = thisSize - rEnd;

  if (thisI->isShared()) {
    ArrayImpl* newI = ArrayImpl::new_(thisI->typeId(), itemSize, thisSize - rangeSize);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);

    uint8_t* newData = newI->data<uint8_t>();
    const uint8_t* oldData = thisI->data<uint8_t>();

    if (rIdx)
      Array_copyToUninitialized(newData, oldData, rIdx, itemSize, isTrivial);

    if (tailSize)
      Array_copyToUninitialized(newData + rIdx * itemSize, oldData + rEnd * itemSize, rIdx, itemSize, isTrivial);

    newI->_size = thisSize - rangeSize;
    _impl = newI;

    thisI->_size = 0;
    thisI->release();

    return kErrorOk;
  }
  else {
    uint8_t* thisData = thisI->data<uint8_t>();

    if (rangeSize && !isTrivial)
      AnyInternal::releaseAnyArray(reinterpret_cast<AnyBase*>(thisData) + rIdx, rangeSize);

    if (tailSize)
      std::memmove(thisData + rIdx * itemSize, thisData + rEnd * itemSize, tailSize * itemSize);

    thisI->_size = thisSize - rangeSize;
    return kErrorOk;
  }
}

// ===========================================================================
// [b2d::Array - Runtime Handlers]
// ===========================================================================

void ArrayOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  uint32_t count = Any::_kTypeIdArrayLast - Any::_kTypeIdArrayFirst + 1;
  for (uint32_t i = 0; i < count; i++)
    Any::_initNone(Any::_kTypeIdArrayFirst + i, const_cast<ArrayImpl*>(&ArrayImpl_none[i]));
}

B2D_END_NAMESPACE
