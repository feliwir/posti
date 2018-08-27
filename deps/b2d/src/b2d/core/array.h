// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ARRAY_H
#define _B2D_CORE_ARRAY_H

// [Dependencies]
#include "../core/any.h"
#include "../core/container.h"
#include "../core/math.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class ArrayBase;
class ArrayImpl;

template<typename T>
class Array;

// ============================================================================
// [b2d::ArrayInternal]
// ============================================================================

namespace ArrayInternal {
  template<typename T>
  struct ArrayTraits {
    enum : uint32_t {
      kTypeId
        = std::is_base_of<Object, T>::value ?
            Any::kTypeIdArrayOfObject

        : std::is_base_of<AnyBase, T>::value ?
            Any::kTypeIdArrayOfAny

        : std::is_integral<T>::value ?
            ( sizeof(T) == 1  && std::is_signed  <T>::value ? Any::kTypeIdArrayOfInt8   :
              sizeof(T) == 1  && std::is_unsigned<T>::value ? Any::kTypeIdArrayOfUInt8  :
              sizeof(T) == 2  && std::is_signed  <T>::value ? Any::kTypeIdArrayOfInt16  :
              sizeof(T) == 2  && std::is_unsigned<T>::value ? Any::kTypeIdArrayOfUInt16 :
              sizeof(T) == 4  && std::is_signed  <T>::value ? Any::kTypeIdArrayOfInt32  :
              sizeof(T) == 4  && std::is_unsigned<T>::value ? Any::kTypeIdArrayOfUInt32 :
              sizeof(T) == 8  && std::is_signed  <T>::value ? Any::kTypeIdArrayOfInt64  :
              sizeof(T) == 8  && std::is_unsigned<T>::value ? Any::kTypeIdArrayOfUInt64 : Any::kTypeIdNull)

        : std::is_floating_point<T>::value ?
            ( sizeof(T) == 4  ? Any::kTypeIdArrayOfFloat32 :
              sizeof(T) == 8  ? Any::kTypeIdArrayOfFloat64 : Any::kTypeIdNull)

        : std::is_pointer<T>::value ?
              Any::kTypeIdArrayOfPtrT

        : std::is_trivial<T>::value ?
            ( sizeof(T) == 1  ? Any::kTypeIdArrayOfTrivial1  :
              sizeof(T) == 2  ? Any::kTypeIdArrayOfTrivial2  :
              sizeof(T) == 4  ? Any::kTypeIdArrayOfTrivial4  :
              sizeof(T) == 6  ? Any::kTypeIdArrayOfTrivial6  :
              sizeof(T) == 8  ? Any::kTypeIdArrayOfTrivial8  :
              sizeof(T) == 12 ? Any::kTypeIdArrayOfTrivial12 :
              sizeof(T) == 16 ? Any::kTypeIdArrayOfTrivial16 :
              sizeof(T) == 20 ? Any::kTypeIdArrayOfTrivial20 :
              sizeof(T) == 24 ? Any::kTypeIdArrayOfTrivial24 :
              sizeof(T) == 28 ? Any::kTypeIdArrayOfTrivial28 :
              sizeof(T) == 32 ? Any::kTypeIdArrayOfTrivial32 :
              sizeof(T) == 48 ? Any::kTypeIdArrayOfTrivial48 :
              sizeof(T) == 64 ? Any::kTypeIdArrayOfTrivial64 : Any::kTypeIdNull)

        : Any::kTypeIdNull,

      kIsValid
        = kTypeId != Any::kTypeIdNull
    };
  };

  template<typename T, typename Arg0T>
  B2D_INLINE void copyToUninitialized(T* dst, Arg0T&& src) noexcept {
    // Otherwise MSVC would generate null checks...
    B2D_ASSUME(dst != nullptr);

    new (dst) T(std::forward<Arg0T>(src));
  }

  template<typename T, typename Arg0T, typename... ArgsT>
  B2D_INLINE void copyToUninitialized(T* dst, Arg0T&& arg0, ArgsT&&... args) noexcept {
    copyToUninitialized(dst + 0, std::forward<Arg0T>(arg0));
    copyToUninitialized(dst + 1, std::forward<ArgsT>(args)...);
  }
}

// ===========================================================================
// [b2d::Array - Impl]
// ===========================================================================

class ArrayImpl : public SimpleImpl {
public:
  // Used only to initialize statically allocated built-in ArrayImpl.
  constexpr ArrayImpl(uint32_t objectInfo, uint32_t itemSize) noexcept
    : _capacity(0),
      _commonData { ATOMIC_VAR_INIT(0), objectInfo | Any::kFlagIsArray, { itemSize } },
      _size(0) {}

  // -------------------------------------------------------------------------
  // [Any Interface]
  // -------------------------------------------------------------------------

  static B2D_INLINE ArrayImpl* new_(uint32_t objectInfo, uint32_t itemSize, size_t capacity) noexcept {
    Support::OverflowFlag of;
    size_t implSize = Support::safeMulAdd<size_t>(sizeof(ArrayImpl), capacity, itemSize, &of);
    if (B2D_UNLIKELY(of))
      return nullptr;

    ArrayImpl* impl = static_cast<ArrayImpl*>(Allocator::systemAlloc(implSize));
    if (B2D_UNLIKELY(!impl))
      return nullptr;

    impl->_commonData.reset(objectInfo | Any::kFlagIsArray, itemSize);
    impl->_size = 0;
    impl->_capacity = capacity;

    return impl;
  }

  template<typename T = ArrayImpl>
  B2D_INLINE T* addRef() noexcept { return SimpleImpl::addRef<T>(); }

  // -------------------------------------------------------------------------
  // [Accessors]
  // -------------------------------------------------------------------------

  //! Get a pointer to the array data.
  template<typename T = void>
  B2D_INLINE T* data() noexcept {
    return reinterpret_cast<T*>( reinterpret_cast<char*>(this) + sizeof(*this) );
  }

  //! Get a pointer to the array data (const).
  template<typename T = void>
  B2D_INLINE const T* data() const noexcept {
    return reinterpret_cast<const T*>( reinterpret_cast<const char*>(this) + sizeof(*this) );
  }

  B2D_INLINE bool hasTrivialData() const noexcept {
    return _commonData.typeId() >= Any::_kTypeIdArrayTrivialStart;
  }

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE uint32_t itemSize() const noexcept { return _commonData._extra32; }
  B2D_INLINE void setItemSize(uint32_t size) noexcept { _commonData._extra32 = size; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _capacity;                      //!< Array capacity (in `T` units).
  CommonData _commonData;                //!< Common data.
  size_t _size;                          //!< Array size (in `T` units).

  // data[] ...
};

// ===========================================================================
// [b2d::Array - Base]
// ===========================================================================

class ArrayBase : public AnyBase {
public:
  enum Limits : size_t {
    kInitSize = 8
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ArrayBase() noexcept = delete;

  B2D_INLINE ArrayBase(const ArrayBase& other) noexcept
    : AnyBase(other.impl()->addRef()) {}

  B2D_INLINE ArrayBase(ArrayBase&& other) noexcept
    : AnyBase(other.impl()) { other._impl = Any::_none[other.impl()->typeId()].impl(); }

  B2D_INLINE explicit ArrayBase(ArrayImpl* impl) noexcept
    : AnyBase(impl) {}

  B2D_INLINE ~ArrayBase() noexcept { AnyInternal::releaseAnyImpl(impl()); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ArrayImpl>
  B2D_INLINE T* impl() const noexcept { return AnyBase::impl<T>(); }

  B2D_API Error _detach() noexcept;
  B2D_INLINE Error detach() noexcept { return isShared() ? _detach() : kErrorOk; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return impl()->size() == 0; }
  B2D_INLINE size_t size() const noexcept { return impl()->size(); }
  B2D_INLINE size_t capacity() const noexcept { return impl()->capacity(); }
  B2D_INLINE uint32_t itemSize() const noexcept { return impl()->itemSize(); }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  //! Reset the whole array to a construction state.
  //!
  //! The data of the array will be released completely if `resetPolicy` is
  //! `Globals::kResetHard` (which means the memory will be freed and capacity
  //! will be zero after the operation).
  //!
  //! NOTE: This function always returns `kErrorOk`. It only returns it so it
  //! can be chained by other functions (and used as a tail-call if possible).
  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  //! Clear the whole array so its size goes back to 0.
  //!
  //! NOTE: This function always returns `kErrorOk`. It only returns it so it
  //! can be chained by other functions (and used as a tail-call if possible).
  B2D_INLINE Error clear() noexcept { return reset(Globals::kResetSoft); }

  B2D_API Error _resize(size_t capacity, bool zeroInitialize) noexcept;
  B2D_API Error reserve(size_t capacity) noexcept;
  B2D_API Error compact() noexcept;

  B2D_API Error _copy(const void* data, size_t size) noexcept;

  B2D_API void* _commonOp(uint32_t op, size_t opSize) noexcept;
  B2D_API void* _insertOp(size_t index, size_t opSize) noexcept;

  B2D_API Error _removeRange(size_t rIdx, size_t rEnd) noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

protected:
  B2D_INLINE ArrayBase& operator=(const ArrayBase& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE ArrayBase& operator=(ArrayBase&& other) noexcept {
    ArrayImpl* oldI = impl();
    _impl = other.impl();

    other._impl = Any::noneByTypeId(other.typeId()).impl<ArrayImpl>();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

// ============================================================================
// [b2d::Array<T>]
// ============================================================================

template<typename T>
class Array : public ArrayBase {
public:
  enum : uint32_t { kTypeId = ArrayInternal::ArrayTraits<T>::kTypeId };

  static_assert(kTypeId != Any::kTypeIdNull,
                "Type 'T' cannot be used with 'b2d::Array<T>' as it's "
                "either non-trivial or not specialized by Blend2D");

  enum Limits : size_t {
    kInitSize = ArrayBase::kInitSize,
    kMaxSize = (std::numeric_limits<size_t>::max() - sizeof(ArrayImpl)) / sizeof(T)
  };

  // -------------------------------------------------------------------------
  // [Construction / Destruction]
  // -------------------------------------------------------------------------

  B2D_INLINE Array() noexcept
    : ArrayBase(Any::noneByTypeId<ArrayBase>(kTypeId).impl()) {}

  B2D_INLINE Array(const Array& other) noexcept
    : ArrayBase(other) {}

  B2D_INLINE Array(Array&& other) noexcept
    : ArrayBase(other.impl()) { other._impl = Any::noneByTypeId(kTypeId).impl(); }

  B2D_INLINE explicit Array(ArrayImpl* impl) noexcept
    : ArrayBase(impl) {}

  // -------------------------------------------------------------------------
  // [Accessors]
  // -------------------------------------------------------------------------

  //! Get a pointer to the array data (mutable).
  B2D_INLINE T* _mutableData() noexcept { return impl()->template data<T>(); }
  //! Get a pointer to the array data.
  B2D_INLINE const T* data() const noexcept { return impl()->template data<T>(); }

  using ArrayBase::empty;
  using ArrayBase::size;
  using ArrayBase::capacity;

  constexpr uint32_t itemSize() const noexcept { return uint32_t(sizeof(T)); }

  B2D_INLINE const T& at(size_t index) const noexcept {
    B2D_ASSERT(index < size());
    return data()[index];
  }

  B2D_INLINE const T& first() const noexcept {
    B2D_ASSERT(!empty());
    return data()[0];
  }

  B2D_INLINE const T& last() const noexcept {
    B2D_ASSERT(!empty());
    return data()[size() - 1];
  }

  B2D_INLINE Error setAt(size_t index, const T& item) noexcept {
    B2D_ASSERT(index < size());
    B2D_PROPAGATE(detach());

    _mutableData()[index] = item;
    return kErrorOk;
  }

  B2D_INLINE Error setAt(size_t index, T&& item) noexcept {
    B2D_ASSERT(index < size());
    B2D_PROPAGATE(detach());

    _mutableData()[index] = item;
    return kErrorOk;
  }

  // -------------------------------------------------------------------------
  // [Operations]
  // -------------------------------------------------------------------------

  using ArrayBase::_resize;

  //! Set all items of the array to `other`.
  B2D_INLINE Error assign(const Array<T>& other) noexcept {
    return AnyInternal::assignImpl(this, other.impl());
  }

  B2D_INLINE Error copyFrom(const Array<T>& other) noexcept {
    ArrayImpl* otherI = other.impl();
    return _copy(otherI->data(), otherI->size());
  }

  B2D_INLINE Error copyFrom(const Array<T>& other, const Range& range) noexcept {
    ArrayImpl* otherI = other.impl();

    size_t rEnd = Math::min(range.end(), otherI->size());
    size_t rIdx = Math::min(range.start(), rEnd);

    return _copy(otherI->template data<T>() + rIdx, rEnd - rIdx);
  }

  B2D_INLINE Error copyFrom(const T* data, size_t size) noexcept { return _copy(data, size); }

  //! \internal
  B2D_INLINE T* _commonOp(uint32_t op, size_t size) noexcept { return reinterpret_cast<T*>(ArrayBase::_commonOp(op, size)); }
  //! \internal
  B2D_INLINE T* _insertOp(size_t index, size_t size) noexcept { return reinterpret_cast<T*>(ArrayBase::_insertOp(index, size)); }

  template<typename... ArgsT>
  B2D_INLINE Error append(ArgsT&&... args) noexcept {
    T* dst = _commonOp(kContainerOpAppend, sizeof...(args));
    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    ArrayInternal::copyToUninitialized(dst, std::forward<ArgsT>(args)...);
    return kErrorOk;
  }

  Error concat(const Array<T>& other) noexcept {
    size_t n = other.size();
    T* dst = _commonOp(kContainerOpAppend, n);

    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    const T* src = other.data();
    for (size_t i = 0; i < n; i++)
      ArrayInternal::copyToUninitialized(&dst[i], src[i]);
    return kErrorOk;
  }

  Error concat(const Array<T>& other, const Range& range) noexcept {
    size_t rEnd = Math::min(range.end(), other.size());
    size_t rIdx = Math::min(range.start(), rEnd);

    size_t n = rEnd - rIdx;
    T* dst = _commonOp(kContainerOpAppend, n);

    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    const T* src = other.data() + rIdx;
    for (size_t i = 0; i < n; i++)
      ArrayInternal::copyToUninitialized(&dst[i], src[i]);
    return kErrorOk;
  }

  Error concat(const T* data, size_t size) noexcept {
    T* dst = _commonOp(kContainerOpAppend, size);

    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    for (size_t i = 0; i < size; i++)
      ArrayInternal::copyToUninitialized(&dst[i], data[i]);
    return kErrorOk;
  }

  template<typename... ArgsT>
  B2D_INLINE Error prepend(ArgsT&&... args) noexcept {
    T* dst = _insertOp(0, sizeof...(args));
    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    ArrayInternal::copyToUninitialized(dst, std::forward<ArgsT>(args)...);
    return kErrorOk;
  }

  template<typename... ArgsT>
  B2D_INLINE Error insert(size_t index, ArgsT&&... args) noexcept {
    T* dst = _insertOp(index, sizeof...(args));
    if (B2D_UNLIKELY(!dst))
      return DebugUtils::errored(kErrorNoMemory);

    ArrayInternal::copyToUninitialized(dst, std::forward<ArgsT>(args)...);
    return kErrorOk;
  }

  B2D_INLINE Error removeFirst() noexcept { return _removeRange(0, 1); }
  B2D_INLINE Error removeLast() noexcept { return _removeRange(size() - 1, size()); }
  B2D_INLINE Error removeAt(size_t index) noexcept { return _removeRange(index, index + 1); }

  B2D_INLINE Error removeRange(const Range& range) noexcept { return _removeRange(range.start(), range.end()); }
  B2D_INLINE Error removeRange(size_t start, size_t end) noexcept { return _removeRange(start, end); }

  B2D_INLINE Error slice(const Range& range) noexcept {
    size_t rEnd = Math::min(range.end(), size());
    size_t rIdx = Math::min(range.start(), rEnd);

    return _copy(reinterpret_cast<const void*>(data() + rIdx), rEnd - rIdx);
  }

  // --------------------------------------------------------------------------
  // [Search]
  // --------------------------------------------------------------------------

  size_t indexOf(const T& item) const noexcept {
    size_t iEnd = size();
    const T* p = data();

    for (size_t i = 0; i < iEnd; i++)
      if (p[i] == item)
        return i;

    return Globals::kInvalidIndex;
  }

  size_t indexOf(const T& item, const Range& range) const noexcept {
    size_t iIdx = range.start();
    size_t iEnd = Math::min(range.end(), size());
    const T* p = data();

    for (size_t i = iIdx; i < iEnd; i++)
      if (p[i] == item)
        return i;

    return Globals::kInvalidIndex;
  }

  size_t lastIndexOf(const T& item) const noexcept {
    size_t i = size();
    const T* p = data();

    while (i != 0)
      if (p[--i] == item)
        return i;

    return Globals::kInvalidIndex;
  }

  size_t lastIndexOf(const T& item, const Range& range) const noexcept {
    size_t iIdx = range.start();
    size_t i = Math::min(range.end(), size());
    const T* p = data();

    while (i > iIdx)
      if (p[--i] == item)
        return i;

    return Globals::kInvalidIndex;
  }

  B2D_INLINE bool contains(const T& item) const noexcept {
    return indexOf(item) != Globals::kInvalidIndex;
  }

  B2D_INLINE bool contains(const T& item, const Range& range) const noexcept {
    return indexOf(item, range) != Globals::kInvalidIndex;
  }

  // -------------------------------------------------------------------------
  // [Operator Overload]
  // -------------------------------------------------------------------------

  B2D_INLINE Array& operator=(const Array& other) noexcept { return static_cast<Array&>(ArrayBase::operator=(other)); }
  B2D_INLINE Array& operator=(Array&& other) noexcept { return static_cast<Array&>(ArrayBase::operator=(other)); }

  B2D_INLINE const T& operator[](size_t index) const noexcept {
    B2D_ASSERT(index < size());
    return impl()->template data<T>()[index];
  }
};

// ============================================================================
// [b2d::Array - Iterator<T>]
// ============================================================================

template<typename T>
class ArrayIterator {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ArrayIterator(const Array<T>& container) noexcept { reset(container); }
  B2D_INLINE ~ArrayIterator() noexcept {}

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isValid() const noexcept { return _p != _pEnd; }
  B2D_INLINE size_t index() const noexcept { return (size_t)(_p - _impl->data<T>()); }

  B2D_INLINE const T& item() const noexcept {
    B2D_ASSERT(isValid());
    return *_p;
  }

  // --------------------------------------------------------------------------
  // [Reset / Next]
  // --------------------------------------------------------------------------

  B2D_INLINE bool reset(const Array<T>& container) noexcept {
    _impl = container.impl();
    return reset();
  }

  B2D_INLINE bool reset() noexcept {
    _p = _impl->data<T>();
    _pEnd = _p + _impl->size();
    return isValid();
  }

  B2D_INLINE bool next() noexcept {
    B2D_ASSERT(_p != _pEnd);
    _p++;
    return isValid();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  //! Operator* that acts like `item()`.
  B2D_INLINE const T& operator*() const noexcept {
    B2D_ASSERT(isValid());
    return *_p;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const ArrayImpl* _impl;
  const T* _p;
  const T* _pEnd;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ARRAY_H
