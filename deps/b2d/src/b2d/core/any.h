// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ANY_H
#define _B2D_CORE_ANY_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct AnyBase;
struct SimpleImpl;

class Object;
class ObjectImpl;

// ============================================================================
// [b2d::AnyInternal]
// ============================================================================

//! \internal
namespace AnyInternal {
  B2D_API Error destroyAnyImpl(void* impl) noexcept;

  B2D_API Error assignAnyImpl(AnyBase* dst, void* impl) noexcept;
  B2D_API Error assignObjectImpl(Object* dst, ObjectImpl* impl) noexcept;

  B2D_API Error releaseAnyImpl(void* impl) noexcept;
  B2D_API Error releaseObjectImpl(ObjectImpl* impl) noexcept;

  B2D_API Error releaseAnyArray(AnyBase* arr, size_t size) noexcept;
  B2D_API Error releaseObjectArray(Object* arr, size_t size) noexcept;

  template<typename T>
  B2D_INLINE T* allocImplT() noexcept { return static_cast<T*>(Allocator::systemAlloc(sizeof(T))); }

  template<typename T>
  B2D_INLINE T* allocSizedImplT(size_t implSize) noexcept { return static_cast<T*>(Allocator::systemAlloc(implSize)); }

  template<typename T, typename... ArgsT>
  B2D_INLINE T* newImplT(ArgsT&&... args) noexcept {
    void* p = Allocator::systemAlloc(sizeof(T));
    return p ? new(p) T(std::forward<ArgsT>(args)...) : nullptr;
  }

  template<typename T, typename... ArgsT>
  B2D_INLINE T* newSizedImplT(size_t implSize, ArgsT&&... args) noexcept {
    void* p = Allocator::systemAlloc(implSize);
    return p ? new(p) T(std::forward<ArgsT>(args)...) : nullptr;
  }

  //! Common implementation data.
  struct CommonData {
    //! Should be called only once when the `CommonData` is created (if created).
    B2D_INLINE void reset(uint32_t objectInfo, uint32_t extra32 = 0) noexcept {
      _refCount.store(1, std::memory_order_relaxed);
      _objectInfo = objectInfo;
      _extra32 = extra32;
    }

    B2D_INLINE void _setToBuiltInNone() noexcept;

    //! Get a type-id of this object, see `Any::TypeId`.
    B2D_INLINE uint32_t typeId() const noexcept;
    //! Get an object information of this object, see `Any::TypeId` and `Any::Flags`.
    B2D_INLINE uint32_t objectInfo() const noexcept { return _objectInfo; }

    B2D_INLINE bool isNone() const noexcept;
    B2D_INLINE bool isArray() const noexcept;
    B2D_INLINE bool isObject() const noexcept;
    B2D_INLINE bool isExternal() const noexcept;
    B2D_INLINE bool isTemporary() const noexcept;
    B2D_INLINE bool isImmutable() const noexcept;

    //! Get whether this object is shared (refCount != 1).
    B2D_INLINE bool isShared() const noexcept {
      return refCount() != 1;
    }

    //! Get the reference count.
    B2D_INLINE size_t refCount() const noexcept {
      return _refCount.load(std::memory_order_relaxed);
    }

    B2D_INLINE void incRef() noexcept {
      size_t rc = _refCount.load(std::memory_order_relaxed);

      // Do not increase a reference count of `CommonData` if `_refCount` is zero.
      // Zero ref-counted objects are statically allocated and thus immutable.
      if (rc != 0)
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    //! Manual dereference, returns true when `refCount` goes to zero from one,
    //! which means that the caller should `destroy()` the Impl instance.
    B2D_INLINE bool deref() noexcept {
      size_t rc = _refCount.load(std::memory_order_relaxed);

      // Do not decrease a reference count of `CommonData` if `_refCount` is zero.
      // Zero ref-counted objects are statically allocated and thus immutable.
      if (rc != 0)
        rc = _refCount.fetch_sub(1, std::memory_order_acq_rel);

      return rc == 1;
    }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    std::atomic<size_t> _refCount;       //!< Atomic reference count.
    uint32_t _objectInfo;                //!< Type and object information.

    union {
      uint32_t _extra32;                 //!< Can be used for any purpose, used to align `CommonImpl` to 16 or 24 bytes.
      uint8_t _extra8[4];                //!< Mapped to the same memory as `_extra32`, but offers 8-bit access.
    };
  };

  //! A header compatible with both `ObjectImpl` and `SimpleImpl`.
  struct CommonImpl {
    union {
      void* _vPtr;                       //!< Used by any ObjectImpl (virtual table).
      uintptr_t _vUInt;                  //!< Used by any SimpleImpl (no virtual table).
    };
    CommonData _commonData;
  };

  //! Get an `CommonImpl` of this `impl`.
  static B2D_INLINE CommonData* commonDataOf(void* impl) noexcept { return &static_cast<CommonImpl*>(impl)->_commonData; }
  //! Get an `CommonImpl` of this `impl` (const).
  static B2D_INLINE const CommonData* commonDataOf(const void* impl) noexcept { return &static_cast<const CommonImpl*>(impl)->_commonData; }

  static B2D_INLINE void* addRef(void* impl) noexcept {
    commonDataOf(impl)->incRef();
    return impl;
  }

  //! Fallback to `y` if `x` is null.
  //!
  //! Used typically in scenarios where a valid `Impl` must be returned, but the
  //! allocation may fail. When the allocation fails the result would fallback to
  //! `none` built-in implementation.
  //!
  //! Example:
  //!
  //! ```
  //! ImageDecoder createCustomDecoder() noexcept {
  //!   return AnyInternal::fallback(
  //!     AnyInternal::newImpl<CustomDecoder>(),
  //!     ImageDecoder::none().impl());
  //! }
  //! ```
  template<typename T, typename X>
  B2D_INLINE T* fallback(X* x, T* y) noexcept { return x ? static_cast<T*>(x) : y; }
}

// ============================================================================
// [b2d::AnyBase]
// ============================================================================

//! Base class of anything that uses either SimpleImpl or ObjectImpl.
//!
//! This is a stub that doesn't do any management of the internal `_impl` object.
//! The reason this class exists is to have the same base class shared between
//! `Object` and other Blend2D containers.
//!
//! If you are looking for class that can hold anything, but performs reference
//! counting then look at `Any`, which can be seen as a low-level variant type.
struct AnyBase {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline AnyBase() noexcept = default;
  constexpr explicit AnyBase(void* impl) noexcept : _impl(impl) {}

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T>
  B2D_INLINE T& cast() noexcept { return *static_cast<T*>(this); }
  template<typename T>
  B2D_INLINE const T& cast() const noexcept { return *static_cast<const T*>(this); }

  template<typename T = void>
  B2D_INLINE T* impl() const noexcept { return reinterpret_cast<T*>(_impl); }

  //! \internal
  B2D_INLINE void _setImpl(void* impl) noexcept { _impl = impl; }

  //! Get whether this object is shared (refCount != 1).
  B2D_INLINE bool isShared() const noexcept { return AnyInternal::commonDataOf(_impl)->isShared(); }
  //! Get the reference count of this `AnyBase` object.
  B2D_INLINE size_t refCount() const noexcept { return AnyInternal::commonDataOf(_impl)->refCount(); }

  //! Get a type-id of this object, see `Any::TypeId`.
  B2D_INLINE uint32_t typeId() const noexcept { return AnyInternal::commonDataOf(_impl)->typeId(); }
  //! Get an object information of this object, see `Any::TypeId` and `Any::Flags`.
  B2D_INLINE uint32_t objectInfo() const noexcept { return AnyInternal::commonDataOf(_impl)->objectInfo(); }

  B2D_INLINE bool isNone() const noexcept { return AnyInternal::commonDataOf(_impl)->isNone(); }
  B2D_INLINE bool isSame(const AnyBase& other) const noexcept { return _impl == other._impl; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  void* _impl;
};

// ============================================================================
// [b2d::Any]
// ============================================================================

//! Can hold any Blend2D object that uses either SimpleImpl or ObjectImpl.
class Any final : public AnyBase {
public:
  //! Type id.
  enum TypeId : uint32_t {
    kTypeIdNull               = 0,       //!< Type is `Null`.

    kTypeIdArrayOfAny         = 1,       //!< Type is `Array<Any>`.
    kTypeIdArrayOfObject      = 2,       //!< Type is `Array<Object>` or `Array<T>` where T inherits `Object`.

    kTypeIdArrayOfPtrT        = 3,       //!< Type is `Array<T*>` where T is considered to be just a value.
    kTypeIdArrayOfInt8        = 4,       //!< Type is `Array<int8_t>` or compatible.
    kTypeIdArrayOfUInt8       = 5,       //!< Type is `Array<uint8_t>` or compatible.
    kTypeIdArrayOfInt16       = 6,       //!< Type is `Array<int16_t>` or compatible.
    kTypeIdArrayOfUInt16      = 7,       //!< Type is `Array<uint16_t>` or compatible.
    kTypeIdArrayOfInt32       = 8,       //!< Type is `Array<int32_t>` or compatible.
    kTypeIdArrayOfUInt32      = 9,       //!< Type is `Array<uint32_t>` or compatible.
    kTypeIdArrayOfInt64       = 10,      //!< Type is `Array<int64_t>` or compatible.
    kTypeIdArrayOfUInt64      = 11,      //!< Type is `Array<uint64_t>` or compatible.
    kTypeIdArrayOfFloat32     = 12,      //!< Type is `Array<float>` or compatible.
    kTypeIdArrayOfFloat64     = 13,      //!< Type is `Array<double>` or compatible.
    kTypeIdArrayOfTrivial1    = 14,      //!< Type is `Array<T>` where `T` is a trivial type of size 1.
    kTypeIdArrayOfTrivial2    = 15,      //!< Type is `Array<T>` where `T` is a trivial type of size 2.
    kTypeIdArrayOfTrivial4    = 16,      //!< Type is `Array<T>` where `T` is a trivial type of size 4.
    kTypeIdArrayOfTrivial6    = 17,      //!< Type is `Array<T>` where `T` is a trivial type of size 6.
    kTypeIdArrayOfTrivial8    = 18,      //!< Type is `Array<T>` where `T` is a trivial type of size 8.
    kTypeIdArrayOfTrivial12   = 19,      //!< Type is `Array<T>` where `T` is a trivial type of size 12.
    kTypeIdArrayOfTrivial16   = 20,      //!< Type is `Array<T>` where `T` is a trivial type of size 16.
    kTypeIdArrayOfTrivial20   = 21,      //!< Type is `Array<T>` where `T` is a trivial type of size 20.
    kTypeIdArrayOfTrivial24   = 22,      //!< Type is `Array<T>` where `T` is a trivial type of size 24.
    kTypeIdArrayOfTrivial28   = 23,      //!< Type is `Array<T>` where `T` is a trivial type of size 28.
    kTypeIdArrayOfTrivial32   = 24,      //!< Type is `Array<T>` where `T` is a trivial type of size 32.
    kTypeIdArrayOfTrivial48   = 25,      //!< Type is `Array<T>` where `T` is a trivial type of size 48.
    kTypeIdArrayOfTrivial64   = 26,      //!< Type is `Array<T>` where `T` is a trivial type of size 64.

    kTypeIdBuffer             = 32,      //!< Type is `Buffer`.
    kTypeIdImage              = 33,      //!< Type is `Image`.
    kTypeIdImageCodec         = 34,      //!< Type is `ImageCodec`.
    kTypeIdImageDecoder       = 35,      //!< Type is `ImageDecoder`.
    kTypeIdImageEncoder       = 36,      //!< Type is `ImageEncoder`.
    kTypeIdPattern            = 37,      //!< Type is `Pattern`.
    kTypeIdLinearGradient     = 38,      //!< Type is `LinearGradient`.
    kTypeIdRadialGradient     = 39,      //!< Type is `RadialGradient`.
    kTypeIdConicalGradient    = 40,      //!< Type is `ConicalGradient`.
    kTypeIdPath2D             = 41,      //!< Type is `Path2D`.
    kTypeIdRegion             = 42,      //!< Type is `Region`.
    kTypeIdRegionInfiniteSlot = 43,      //!< Used as a built-in slot of `Region::infinite()`, never used as a type-id.
    kTypeIdContext2D          = 44,      //!< Type is `Context2D`.

    kTypeIdFont               = 50,
    kTypeIdFontData           = 51,
    kTypeIdFontFace           = 52,
    kTypeIdFontLoader         = 53,
    kTypeIdFontMatcher        = 54,
    kTypeIdFontEnumerator     = 55,
    kTypeIdFontCollection     = 56,

    _kTypeIdArrayFirst        = 1,
    _kTypeIdArrayLast         = 26,
    _kTypeIdArrayTrivialStart = kTypeIdArrayOfPtrT,

    _kTypeIdGradientFirst     = kTypeIdLinearGradient,
    _kTypeIdGradientLast      = kTypeIdConicalGradient,

    kTypeIdCount              = 128,        //!< Count of TypeId slots.
    kTypeIdMask               = 0x0000FFFFu //!< Object type mask.
  };

  //! Type flags (additional flags that can be used with ObjectInfo).
  enum Flags : uint32_t {
    kFlagIsNone           = 0x01000000u, //!< Object is a built-in `none` instance.
    kFlagIsArray          = 0x02000000u, //!< Object inherits directly or indirectly `b2d::Object`.
    kFlagIsObject         = 0x04000000u, //!< Object inherits directly or indirectly `b2d::Object`.
    kFlagIsExternal       = 0x08000000u, //!< Object has external data.
    kFlagIsTemporary      = 0x10000000u, //!< Object is temporary - allocated on the stack, won't survive its scope.
    kFlagIsImmutable      = 0x20000000u, //!< Object is immutable.
    kFlagIsInfinite       = 0x40000000u, //!< Object is infinite, only used by `Region::infinite()` instance.

    kFlagsMask            = 0xFFFF0000u  //!< Object flags mask.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Any(const Any& other) noexcept
    : AnyBase(AnyInternal::addRef(other.impl())) {}

  B2D_INLINE Any(const AnyBase& other) noexcept
    : AnyBase(AnyInternal::addRef(other.impl())) {}

  B2D_INLINE Any(AnyBase&& other) noexcept
    : AnyBase(other.impl()) { other._impl = noneByTypeId<AnyBase>(AnyInternal::commonDataOf(other.impl())->typeId()).impl(); }

  B2D_INLINE explicit Any(void* impl) noexcept
    : AnyBase(impl) {}

  B2D_INLINE ~Any() noexcept { AnyInternal::releaseAnyImpl(impl()); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Any& operator=(const AnyBase& other) noexcept {
    AnyInternal::assignAnyImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Any& operator=(AnyBase&& other) noexcept {
    void* oldI = impl();
    _impl = other.impl();

    other._impl = noneByTypeId<AnyBase>(AnyInternal::commonDataOf(oldI)->typeId()).impl();
    AnyInternal::releaseAnyImpl(oldI);

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  //! Built-in `none` objects indexed by `Any::TypeId`.
  static B2D_API AnyBase _none[Any::kTypeIdCount];

  //! \internal
  //!
  //! Called on startup to initialize none instances for all valid `typeId` values.
  static B2D_INLINE void _initNone(uint32_t typeId, void* impl) noexcept { _none[typeId]._impl = impl; }

  //! Get a built-in none object of `T` type.
  template<typename T>
  static B2D_INLINE const T& noneOf() noexcept { return static_cast<const T&>(_none[T::kTypeId]); }

  template<typename T = AnyBase>
  static B2D_INLINE const T& noneByTypeId(uint32_t typeId) noexcept { return static_cast<const T&>(_none[typeId]); }
};

B2D_INLINE void AnyInternal::CommonData::_setToBuiltInNone() noexcept {
  _refCount.store(0, std::memory_order_relaxed);
  _objectInfo |= Any::kFlagIsNone;
}

B2D_INLINE uint32_t AnyInternal::CommonData::typeId() const noexcept { return _objectInfo & Any::kTypeIdMask; }
B2D_INLINE bool AnyInternal::CommonData::isNone() const noexcept { return (_objectInfo & Any::kFlagIsNone) != 0; }
B2D_INLINE bool AnyInternal::CommonData::isArray() const noexcept { return (_objectInfo & Any::kFlagIsArray) != 0; }
B2D_INLINE bool AnyInternal::CommonData::isObject() const noexcept { return (_objectInfo & Any::kFlagIsObject) != 0; }
B2D_INLINE bool AnyInternal::CommonData::isExternal() const noexcept { return (_objectInfo & Any::kFlagIsExternal) != 0; }
B2D_INLINE bool AnyInternal::CommonData::isTemporary() const noexcept { return (_objectInfo & Any::kFlagIsTemporary) != 0; }
B2D_INLINE bool AnyInternal::CommonData::isImmutable() const noexcept { return (_objectInfo & Any::kFlagIsImmutable) != 0; }

// ============================================================================
// [b2d::SimpleImpl]
// ============================================================================

//! \internal
struct SimpleImpl {
  typedef AnyInternal::CommonData CommonData;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  //! Get an `CommonHeader` of this impl.
  B2D_INLINE CommonData* commonData() noexcept { return AnyInternal::commonDataOf(this); }
  //! Get an `CommonHeader` of this impl (const).
  B2D_INLINE const CommonData* commonData() const noexcept { return AnyInternal::commonDataOf(this); }

  //! Get a type-id of this object, see `Any::TypeId`.
  B2D_INLINE uint32_t typeId() const noexcept { return commonData()->typeId(); }
  //! Get an object information of this object, see `Any::TypeId` and `Any::Flags`.
  B2D_INLINE uint32_t objectInfo() const noexcept { return commonData()->objectInfo(); }

  B2D_INLINE bool isNone() const noexcept { return commonData()->isNone(); }
  B2D_INLINE bool isArray() const noexcept { return commonData()->isArray(); }
  B2D_INLINE bool isObject() const noexcept { return commonData()->isObject(); }
  B2D_INLINE bool isExternal() const noexcept { return commonData()->isExternal(); }
  B2D_INLINE bool isTemporary() const noexcept { return commonData()->isTemporary(); }
  B2D_INLINE bool isImmutable() const noexcept { return commonData()->isImmutable(); }

  template<typename T>
  B2D_INLINE T* as() noexcept { return reinterpret_cast<T*>(this); }

  template<typename T>
  B2D_INLINE const T* as() const noexcept { return reinterpret_cast<const T*>(this); }

  //! Get whether this object is shared (refCount != 1).
  B2D_INLINE bool isShared() const noexcept { return commonData()->isShared(); }
  //! Get the reference count of this impl.
  B2D_INLINE size_t refCount() const noexcept { return commonData()->refCount(); }

  template<typename T = SimpleImpl>
  B2D_INLINE T* addRef() noexcept {
    commonData()->incRef();
    return as<T>();
  }

  //! Manual dereference, returns true when `refCount` goes to zero from one,
  //! which means that the caller should `destroy()` the Impl instance.
  B2D_INLINE bool deref() noexcept {
    return commonData()->deref();
  }

  B2D_INLINE void release() noexcept {
    if (deref())
      AnyInternal::destroyAnyImpl(this);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  // <None> - This is a stub that doesn't have any members.
};

// ============================================================================
// [b2d::ObjectImpl]
// ============================================================================

class ObjectImpl {
public:
  B2D_NONCOPYABLE(ObjectImpl)
  typedef AnyInternal::CommonData CommonData;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API ObjectImpl(uint32_t objectInfo) noexcept;
  B2D_API virtual ~ObjectImpl() noexcept;

  virtual void destroy() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  //! Get an `CommonHeader` of this impl.
  B2D_INLINE CommonData* commonData() noexcept { return &_commonData; }
  //! Get an `CommonHeader` of this impl (const).
  B2D_INLINE const CommonData* commonData() const noexcept { return &_commonData; }

  //! Get a type-id of this object, see `Any::TypeId`.
  B2D_INLINE uint32_t typeId() const noexcept { return _commonData.typeId(); }
  //! Get an object information of this object, see `Any::TypeId` and `Any::Flags`.
  B2D_INLINE uint32_t objectInfo() const noexcept { return _commonData.objectInfo(); }

  B2D_INLINE bool isNone() const noexcept { return commonData()->isNone(); }
  B2D_INLINE bool isArray() const noexcept { return false; }
  B2D_INLINE bool isObject() const noexcept { return true; }
  B2D_INLINE bool isExternal() const noexcept { return commonData()->isExternal(); }
  B2D_INLINE bool isTemporary() const noexcept { return commonData()->isTemporary(); }
  B2D_INLINE bool isImmutable() const noexcept { return commonData()->isImmutable(); }

  template<typename T>
  B2D_INLINE T* as() noexcept { return reinterpret_cast<T*>(this); }
  template<typename T>
  B2D_INLINE const T* as() const noexcept { return reinterpret_cast<const T*>(this); }

  //! Get whether this object is shared (refCount != 1).
  B2D_INLINE bool isShared() const noexcept { return _commonData.isShared(); }
  //! Get reference count.
  B2D_INLINE size_t refCount() const noexcept { return _commonData.refCount(); }

  template<typename T = ObjectImpl>
  B2D_INLINE T* addRef() noexcept {
    _commonData.incRef();
    return as<T>();
  }

  B2D_INLINE bool deref() noexcept {
    return _commonData.deref();
  }

  B2D_INLINE void release() noexcept {
    if (deref())
      destroy();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  /*
  hidden void* _vPtr;
  */

  CommonData _commonData;
};

// ============================================================================
// [b2d::Object]
// ============================================================================

class Object : public AnyBase {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Object(const Object& other) noexcept
    : AnyBase(other.impl()->addRef()) {}

  B2D_INLINE Object(Object&& other) noexcept
    : AnyBase(other.impl()) { other._impl = Any::noneByTypeId<Object>(other.impl()->typeId()).impl(); }

  B2D_INLINE explicit Object(ObjectImpl* impl) noexcept
    : AnyBase(impl) {}

  B2D_INLINE ~Object() noexcept { AnyInternal::releaseObjectImpl(impl()); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ObjectImpl>
  B2D_INLINE T* impl() const noexcept { return AnyBase::impl<T>(); }

  //! Get a type-id of this object, see `Any::Type`.
  B2D_INLINE uint32_t typeId() const noexcept { return impl()->typeId(); }
  //! Get an object information of this object, see `Any::TypeId` and `Any::Flags`.
  B2D_INLINE uint32_t objectInfo() const noexcept { return impl()->objectInfo(); }

  //! Get whether this object is shared (refCount != 1).
  B2D_INLINE bool isShared() const noexcept { return impl()->isShared(); }
  //! Get reference count.
  B2D_INLINE size_t refCount() const noexcept { return impl()->refCount(); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  // TODO: operator= ?
};

// ============================================================================
// [b2d::Weak<T>]
// ============================================================================

//! Weak reference to either SimpleImpl or ObjectImpl (manual ref-count management if needed).
template<typename T>
class Weak {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Weak() noexcept { _obj->_impl = nullptr; }
  B2D_INLINE explicit Weak(void* impl) noexcept { _obj->_impl = impl; }
  B2D_INLINE explicit Weak(const T& other) noexcept { _obj->_impl = other._impl; }
  B2D_INLINE explicit Weak(const Weak<T>& other) noexcept { _obj->_impl = other._obj->_impl; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _obj->_impl == nullptr; }

  B2D_INLINE T* get() noexcept { return static_cast<T*>(&_obj); }
  B2D_INLINE const T* get() const noexcept { return static_cast<const T*>(&_obj); }

  B2D_INLINE void reset() noexcept { _obj->_impl = nullptr; }
  B2D_INLINE void reset(void* impl) noexcept { _obj->_impl = impl; }
  B2D_INLINE void reset(const T& other) noexcept { _obj->_impl = other._impl; }
  B2D_INLINE void reset(const Weak<T>& other) noexcept { _obj->_impl = other._obj->_impl; }

  // Manual ref-count management, be careful!
  B2D_INLINE void _addRef() noexcept { get()->impl()->addRef(); }
  B2D_INLINE void _release() noexcept { get()->impl()->release(); reset(); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Weak<T>& operator=(const T& other) noexcept { reset(other); return *this; }
  B2D_INLINE Weak<T>& operator=(const Weak<T>& other) noexcept { reset(other); return *this; }

  B2D_INLINE T& operator()() noexcept { return *get(); }
  B2D_INLINE const T& operator()() const noexcept { return *get(); }

  B2D_INLINE T* operator->() noexcept { return get(); }
  B2D_INLINE const T* operator->() const noexcept { return get(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Wrap<AnyBase> _obj;
};

// ============================================================================
// [b2d::AnyInternal::Additional API]
// ============================================================================

namespace AnyInternal {
  template<typename BaseT, typename T>
  constexpr bool isBaseOf() noexcept { return std::is_base_of<BaseT, T>::value; }

  template<typename BaseT, typename T>
  constexpr bool isBaseOf(const T&) noexcept { return std::is_base_of<BaseT, T>::value; }

  template<typename T>
  constexpr bool isAny() noexcept { return isBaseOf<AnyBase, T>(); }
  template<typename T>
  constexpr bool isAny(const T&) noexcept { return isBaseOf<AnyBase, T>(); }

  template<typename T>
  constexpr bool isObject() noexcept { return isBaseOf<Object, T>(); }
  template<typename T>
  constexpr bool isObject(const T&) noexcept { return isBaseOf<Object, T>(); }

  template<typename T, typename ImplT>
  B2D_INLINE Error assignImpl(T* dst, ImplT* impl) noexcept {
    if (isBaseOf<Object>(*dst))
      return assignObjectImpl(reinterpret_cast<Object*>(dst), reinterpret_cast<ObjectImpl*>(impl));
    else
      return assignAnyImpl(dst, impl);
  }

  template<typename ImplT>
  B2D_INLINE Error releaseImpl(ImplT* impl) noexcept {
    if (isBaseOf<ObjectImpl>(*impl))
      return releaseObjectImpl(reinterpret_cast<ObjectImpl*>(impl));
    else
      return releaseAnyImpl(impl);
  }

  template<typename T, typename ImplT>
  B2D_INLINE Error replaceImpl(T* dst, ImplT* newI) noexcept {
    auto* oldI = dst->impl();
    dst->_impl = newI;
    return releaseImpl(oldI);
  }
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ANY_H
