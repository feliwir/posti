// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_BUFFER_H
#define _B2D_CORE_BUFFER_H

// [Dependencies]
#include "../core/any.h"
#include "../core/container.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::BufferImpl]
// ============================================================================

class B2D_VIRTAPI BufferImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(BufferImpl)

  enum : uint32_t { kEmbeddedSize = 8 };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API BufferImpl(size_t capacity, uint8_t* externalData = nullptr) noexcept;
  B2D_API ~BufferImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  static constexpr size_t sizeOf(size_t capacity) noexcept {
    return sizeof(BufferImpl) - kEmbeddedSize + capacity;
  }

  static constexpr size_t capacityForSize(size_t size) noexcept {
    return Support::alignUp(sizeof(BufferImpl) - kEmbeddedSize + size, Globals::kMemGranularity) - (sizeof(BufferImpl) - kEmbeddedSize);
  }

  static B2D_INLINE BufferImpl* newInternal(size_t capacity) noexcept {
    size_t implSize = sizeOf(capacity);
    return AnyInternal::newSizedImplT<BufferImpl>(implSize, capacity);
  }

  static B2D_INLINE BufferImpl* newExternal(uint8_t* externalData, size_t size) noexcept {
    return AnyInternal::newImplT<BufferImpl>(size, externalData);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE uint8_t* data() const noexcept {
    return isExternal() ? _externalData : const_cast<uint8_t*>(_internalData);
  }

  B2D_INLINE uint8_t* internalData() const noexcept {
    B2D_ASSERT(!isExternal());
    return const_cast<uint8_t*>(_internalData);
  }

  B2D_INLINE uint8_t* externalData() const noexcept {
    B2D_ASSERT(isExternal());
    return _externalData;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _size;                          //!< Size [bytes].
  size_t _capacity;                      //!< Capacity [bytes].

  union {
    uint8_t* _externalData;              //!< External data (pointer).
    uint8_t _internalData[kEmbeddedSize];//!< Internal data (embedded).
  };
};

// ============================================================================
// [b2d::Buffer]
// ============================================================================

class Buffer : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdBuffer };

  enum Limits : size_t {
    kInitSize = 1024,
    kMaxSize = std::numeric_limits<size_t>::max() - sizeof(BufferImpl)
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Buffer() noexcept
    : Object(none().impl()) {}

  B2D_INLINE Buffer(const Buffer& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE Buffer(Buffer&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Buffer(BufferImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const Buffer& none() noexcept { return Any::noneOf<Buffer>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T = BufferImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  B2D_INLINE bool empty() const noexcept { return impl()->size() == 0; }
  B2D_INLINE size_t size() const noexcept { return impl()->size(); }
  B2D_INLINE size_t capacity() const noexcept { return impl()->capacity(); }

  template<typename T = uint8_t>
  B2D_INLINE const uint8_t* data() const noexcept { return impl()->data(); }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const Buffer& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;
  B2D_API Error reserve(size_t maxSize) noexcept;
  B2D_API uint8_t* modify(uint32_t op, size_t maxSize) noexcept;

  B2D_INLINE void _end(size_t size) noexcept {
    B2D_ASSERT(impl()->refCount() == 1);
    B2D_ASSERT(size <= impl()->capacity());

    impl()->_size = size;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Buffer& operator=(const Buffer& other) noexcept { assign(other); return *this; }

  B2D_INLINE Buffer& operator=(Buffer&& other) noexcept {
    BufferImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_BUFFER_H
