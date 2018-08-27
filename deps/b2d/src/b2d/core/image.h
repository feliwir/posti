// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_IMAGE_H
#define _B2D_CORE_IMAGE_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/any.h"
#include "../core/argb.h"
#include "../core/buffer.h"
#include "../core/container.h"
#include "../core/geomtypes.h"
#include "../core/imagescaler.h"
#include "../core/pixelformat.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::kImageAccess]
// ============================================================================

// TODO: Cannot be here...
//! Image access type.
enum kImageAccess {
  kImageAccessNull = 0,                  //!< Only used if the buffer is not initialized.
  kImageAccessReadOnly = 1,              //!< Pixels are read-only.
  kImageAccessReadWrite = 2              //!< Pixels are read/write.
};

// ============================================================================
// [b2d::Image - Buffer]
// ============================================================================

class ImageBuffer {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageBuffer() noexcept
    : _pixelData(nullptr),
      _stride(0),
      _size(0, 0),
      _pfi(),
      _access(kImageAccessNull) {}

  B2D_INLINE ImageBuffer(const ImageBuffer& other) noexcept
    : _pixelData(other._pixelData),
      _stride(other._stride),
      _size(other._size),
      _pfi(other._pfi),
      _access(other._access) {}

  B2D_INLINE explicit ImageBuffer(NoInit_) noexcept {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _pixelData = nullptr;
    _stride = 0;
    _size.reset();
    _pfi.reset();
    _access = kImageAccessNull;
  }

  B2D_INLINE void reset(uint8_t* pixelData, const PixelFormatInfo& pfi, uint32_t access, intptr_t stride, const IntSize& size) noexcept {
    _pixelData = pixelData;
    _pfi = pfi;
    _access = access;
    _stride = stride;
    _size = size;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint8_t* pixelData() const noexcept { return _pixelData; }
  B2D_INLINE intptr_t stride() const noexcept { return _stride; }

  B2D_INLINE const IntSize& size() const noexcept { return _size; }
  B2D_INLINE int width() const noexcept { return _size._w; }
  B2D_INLINE int height() const noexcept { return _size._h; }

  B2D_INLINE uint32_t bpp() const noexcept { return _pfi.bpp(); }
  B2D_INLINE uint32_t pixelFlags() const noexcept { return _pfi.pixelFlags(); }
  B2D_INLINE uint32_t pixelFormat() const noexcept { return _pfi.pixelFormat(); }

  B2D_INLINE uint32_t hasAlpha() const noexcept { return _pfi.hasAlpha(); }
  B2D_INLINE uint32_t hasRGB() const noexcept { return _pfi.hasRGB(); }
  B2D_INLINE uint32_t hasARGB() const noexcept { return _pfi.hasARGB(); }

  B2D_INLINE uint32_t access() const noexcept { return _access; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageBuffer& operator=(const ImageBuffer& other) noexcept {
    _pixelData = other._pixelData;
    _stride = other._stride;
    _size = other._size;
    _pfi = other._pfi;
    _access = other._access;

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t* _pixelData;
  intptr_t _stride;
  IntSize _size;
  PixelFormatInfo _pfi;
  uint32_t _access;
};

// ============================================================================
// [b2d::Image - Impl]
// ============================================================================

//! \internal
//!
//! Image [Impl].
class B2D_VIRTAPI ImageImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(ImageImpl)

  typedef void (B2D_CDECL* DestroyHandler)(ImageImpl* d);

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static constexpr size_t strideForWidth(uint32_t width, uint32_t bpp) noexcept {
    return bpp == 1 ? size_t(width) * bpp
                    : Support::alignUp<size_t>(size_t(width) * bpp, (bpp <= 4) ? 4 : 8);
  }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API ImageImpl(int w, int h, uint32_t pixelFormat, uint8_t* pixelData, intptr_t stride, DestroyHandler destroyHandler) noexcept;
  B2D_API ~ImageImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageImpl>
  B2D_INLINE T* addRef() noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t implType() const noexcept { return _implType; }

  B2D_INLINE uint32_t bpp() const noexcept { return _pfi.bpp(); }
  B2D_INLINE uint32_t pixelFlags() const noexcept { return _pfi.pixelFlags(); }
  B2D_INLINE uint32_t pixelFormat() const noexcept { return _pfi.pixelFormat(); }
  B2D_INLINE PixelFormatInfo pixelFormatInfo() const noexcept { return _pfi; }

  B2D_INLINE uint8_t* pixelData() const noexcept { return _pixelData; }
  B2D_INLINE intptr_t stride() const noexcept { return _stride; }

  B2D_INLINE const IntSize& size() const noexcept { return _size; }
  B2D_INLINE int w() const noexcept { return _size.w(); }
  B2D_INLINE int h() const noexcept { return _size.h(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _implType;                    //!< Image implementation type.
  PixelFormatInfo _pfi;                  //!< Pixel format information.
  uint8_t* _pixelData;                   //!< Pixel data.

  mutable std::atomic<void*> _writer;    //!< Object that has an exclusive access to the image at the moment.
  mutable std::atomic<size_t> _lockCount;//!< Count of locks, basically prevents to acquire a write-lock.

  intptr_t _stride;                      //!< Image stride.
  IntSize _size;                         //!< Image size.

  DestroyHandler _destroyHandler;        //!< Destroy handler.
};

// ============================================================================
// [b2d::Image]
// ============================================================================

//! Image.
class Image : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdImage };

  typedef ImageImpl::DestroyHandler DestroyHandler;

  //! Image implementation type.
  enum ImplType : uint32_t {
    kImplTypeNone      = 0,              //!< Null implementation (uninitialized images).
    kImplTypeMemory    = 1,              //!< Memory image implementation .
    kImplTypeExternal  = 2,              //!< External image implementation (using memory not allocated by Blend2D).

    kImplTypeCount     = 3               //!< Count of image implementation types.
  };

  // TODO: Move.
  //! RAW byte offsets of supported pixel formats.
  enum Offset : uint32_t {
    kOffsetA32 = B2D_ARCH_LE ? 3 : 0,    //!< A component offset of `kFormatPRGB32` and `kFormatXRGB32`.
    kOffsetR32 = B2D_ARCH_LE ? 2 : 1,    //!< R component offset of `kFormatPRGB32` and `kFormatXRGB32`.
    kOffsetG32 = B2D_ARCH_LE ? 1 : 2,    //!< G component offset of `kFormatPRGB32` and `kFormatXRGB32`.
    kOffsetB32 = B2D_ARCH_LE ? 0 : 3     //!< B component offset of `kFormatPRGB32` and `kFormatXRGB32`.
  };

  // TODO: Move to globals?
  enum Limits : uint32_t {
    //! Maximum width and height of an \ref Image.
    kMaxSize = 65535
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Image() noexcept
    : Object(none().impl()) {}

  B2D_INLINE Image(const Image& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE Image(Image&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Image(ImageImpl* impl) noexcept
    : Object(impl) {}

  B2D_API Image(int w, int h, uint32_t pixelFormat) noexcept;

  static B2D_INLINE const Image& none() noexcept { return Any::noneOf<Image>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  //! \internal
  B2D_API Error _detach() noexcept;
  //! \copydoc Doxygen::Implicit::detach().
  B2D_INLINE Error detach() noexcept { return isShared() ? _detach() : kErrorOk; }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! \copydoc Doxygen::Implicit::reset().
  B2D_API Error reset() noexcept;

  // --------------------------------------------------------------------------
  // [Create]
  // --------------------------------------------------------------------------

  //! Create a new image of a specified `size` and `pixelFormat.
  //!
  //! Please always check error value, because allocation memory for image data
  //! can fail. Also if there are invalid arguments (dimensions or pixel format)
  //! `kErrorInvalidArgument` will be returned.
  B2D_API Error create(int w, int h, uint32_t pixelFormat) noexcept;

  //! Create a new image from external data.
  B2D_API Error create(int w, int h, uint32_t pixelFormat, void* pixelData, intptr_t stride, DestroyHandler destroyHandler = nullptr) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get image implementation type, see `Image::ImplType`.
  B2D_INLINE uint32_t implType() const noexcept { return impl()->implType(); }

  //! Get whether the image has no size.
  B2D_INLINE bool empty() const noexcept { return impl()->w() == 0; }
  //! Get image width.
  B2D_INLINE int width() const noexcept { return impl()->w(); }
  //! Get image height.
  B2D_INLINE int height() const noexcept { return impl()->h(); }
  //! Get image size (in pixels).
  B2D_INLINE const IntSize& size() const noexcept { return impl()->size(); }

  //! Get bytes per pixel.
  B2D_INLINE uint32_t bpp() const noexcept { return impl()->bpp(); }
  //! Get pixel flags, see `PixelFlags::Flags`.
  B2D_INLINE uint32_t pixelFlags() const noexcept { return impl()->pixelFlags(); }
  //! Get pixel format, see `PixelFormat::Id`.
  B2D_INLINE uint32_t pixelFormat() const noexcept { return impl()->pixelFormat(); }

  //! Get whether the image has alpha component.
  B2D_INLINE bool hasAlpha() const noexcept { return impl()->pixelFormatInfo().hasAlpha(); }
  //! Get whether the image has RGB components.
  B2D_INLINE bool hasRGB() const noexcept { return impl()->pixelFormatInfo().hasRGB(); }
  //! Get whether the image has all ARGB components.
  B2D_INLINE bool hasARGB() const noexcept { return impl()->pixelFormatInfo().hasARGB(); }
  //! Get whether the pixel contains bits which are unused.
  B2D_INLINE bool hasUnusedBits() const noexcept { return impl()->pixelFormatInfo().hasUnusedBits(); }

  B2D_INLINE bool isRGBOnly() const noexcept { return impl()->pixelFormatInfo().isRGBOnly(); }
  B2D_INLINE bool isAlphaOnly() const noexcept { return impl()->pixelFormatInfo().isAlphaOnly(); }
  B2D_INLINE bool isPremultiplied() const noexcept { return impl()->pixelFormatInfo().isPremultiplied(); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const Image& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }

  //! Create a deep copy of image @a other.
  B2D_API Error setDeep(const Image& other) noexcept;

  // --------------------------------------------------------------------------
  // [Equality]
  // --------------------------------------------------------------------------

  static B2D_API bool B2D_CDECL _eq(const Image* a, const Image* b) noexcept;
  B2D_INLINE bool eq(const Image& other) const noexcept { return _eq(this, &other); }

  // --------------------------------------------------------------------------
  // [Scale]
  // --------------------------------------------------------------------------

  static B2D_API Error scale(Image& dst, const Image& src, const IntSize& size, const ImageScaler::Params& params) noexcept;

  // --------------------------------------------------------------------------
  // [Lock / Unlock]
  // --------------------------------------------------------------------------

  // TODO: Refactor this APIs, not ideal!

  B2D_INLINE Error lock(ImageBuffer& pixels) const noexcept {
    return const_cast<Image*>(this)->lock(pixels, nullptr);
  }

  B2D_INLINE Error unlock(ImageBuffer& pixels) const noexcept {
    return const_cast<Image*>(this)->unlock(pixels, nullptr);
  }

  B2D_API Error lock(ImageBuffer& pixels, void* writer) noexcept;
  B2D_API Error unlock(ImageBuffer& pixels, void* writer) noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Image& operator=(const Image& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Image& operator=(Image&& other) noexcept {
    ImageImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const Image& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Image& other) const noexcept { return !eq(other); }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_IMAGE_H
