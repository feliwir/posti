// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/any.h"
#include "../core/array.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Any]
// ============================================================================

AnyBase Any::_none[Any::kTypeIdCount];

static B2D_INLINE void AnyInternal_destroyAnyImplInline(void* impl) noexcept {
  AnyInternal::CommonData* commonData = AnyInternal::commonDataOf(impl);

  // Should never happen in production as the built-in none objects always have
  // `refCount == 0`, which would never dereference from 1 to 0. So we assert.
  B2D_ASSERT(!commonData->isNone());

  if (commonData->isObject()) {
    static_cast<ObjectImpl*>(impl)->destroy();
    return;
  }

  if (commonData->isArray()) {
    ArrayImpl* arrayI = static_cast<ArrayImpl*>(impl);
    if (!arrayI->hasTrivialData())
      AnyInternal::releaseAnyArray(arrayI->data<AnyBase>(), arrayI->size());
  }

  if (!commonData->isTemporary()) {
    Allocator::systemRelease(impl);
  }
}

Error AnyInternal::destroyAnyImpl(void* impl) noexcept {
  AnyInternal_destroyAnyImplInline(impl);
  return kErrorOk;
}

Error AnyInternal::assignAnyImpl(AnyBase* dst, void* impl) noexcept {
  void* oldImpl = dst->impl();
  dst->_impl = impl;
  AnyInternal::commonDataOf(impl)->incRef();

  if (AnyInternal::commonDataOf(oldImpl)->deref())
    AnyInternal_destroyAnyImplInline(oldImpl);
  return kErrorOk;
}

Error AnyInternal::assignObjectImpl(Object* dst, ObjectImpl* impl) noexcept {
  ObjectImpl* oldImpl = dst->impl();
  dst->_impl = impl->addRef();

  if (oldImpl->deref())
    oldImpl->destroy();
  return kErrorOk;
}

Error AnyInternal::releaseAnyImpl(void* impl) noexcept {
  if (AnyInternal::commonDataOf(impl)->deref())
    AnyInternal_destroyAnyImplInline(impl);
  return kErrorOk;
}

Error AnyInternal::releaseObjectImpl(ObjectImpl* impl) noexcept {
  if (impl->deref())
    impl->destroy();
  return kErrorOk;
}

Error AnyInternal::releaseAnyArray(AnyBase* arr, size_t size) noexcept {
  for (size_t i = 0; i < size; i++) {
    void* impl = arr[i]._impl;
    if (AnyInternal::commonDataOf(impl)->deref())
      AnyInternal_destroyAnyImplInline(impl);
  }
  return kErrorOk;
}

Error AnyInternal::releaseObjectArray(Object* arr, size_t size) noexcept {
  for (size_t i = 0; i < size; i++) {
    ObjectImpl* impl = arr[i].impl();
    if (impl->deref())
      impl->destroy();
  }
  return kErrorOk;
}

// ============================================================================
// [b2d::ObjectImpl]
// ============================================================================

ObjectImpl::ObjectImpl(uint32_t objectInfo) noexcept
  : _commonData { ATOMIC_VAR_INIT(1), objectInfo | Any::kFlagIsObject, { 0 } } {}

ObjectImpl::~ObjectImpl() noexcept {}

void ObjectImpl::destroy() noexcept {
  bool releaseIt = !isTemporary();
  this->~ObjectImpl();

  if (releaseIt)
    Allocator::systemRelease(static_cast<void*>(this));
}

B2D_END_NAMESPACE
