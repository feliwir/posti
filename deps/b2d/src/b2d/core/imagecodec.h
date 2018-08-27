// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_IMAGECODEC_H
#define _B2D_CORE_IMAGECODEC_H

#include "../core/any.h"
#include "../core/array.h"
#include "../core/buffer.h"
#include "../core/geomtypes.h"
#include "../core/image.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class ImageCodec;
typedef Array<ImageCodec> ImageCodecArray;

// ============================================================================
// [b2d::ImageInfo]
// ============================================================================

//! Image information.
class ImageInfo {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageInfo() noexcept { reset(); }
  B2D_INLINE ImageInfo(const ImageInfo& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  B2D_INLINE void _copy(const ImageInfo& other) noexcept {
    std::memcpy(this, &other, sizeof(ImageInfo));
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { std::memset(this, 0, sizeof(ImageInfo)); }

  B2D_INLINE IntSize size() const noexcept { return _size; }
  B2D_INLINE int width() const noexcept { return _size._w; }
  B2D_INLINE int height() const noexcept { return _size._h; }

  B2D_INLINE void setSize(const IntSize& size) noexcept { _size = size; }
  B2D_INLINE void setSize(int w, int h) noexcept { _size.reset(w, h); }
  B2D_INLINE void setWidth(int w) noexcept { _size._w = w; }
  B2D_INLINE void setHeight(int h) noexcept { _size._h = h; }

  B2D_INLINE uint32_t depth() const noexcept { return _depth; }
  B2D_INLINE uint32_t numPlanes() const noexcept { return _numPlanes; }
  B2D_INLINE uint32_t numFrames() const noexcept { return _numFrames; }
  B2D_INLINE uint32_t isProgressive() const noexcept { return _progressive; }

  B2D_INLINE void setDepth(uint32_t depth) noexcept { _depth = uint8_t(depth); }
  B2D_INLINE void setNumPlanes(uint32_t numPlanes) noexcept { _numPlanes = uint8_t(numPlanes); }
  B2D_INLINE void setNumFrames(uint32_t numFrames) noexcept { _numFrames = numFrames; }
  B2D_INLINE void setProgressive(uint32_t progressive) noexcept { _progressive = uint8_t(progressive); }

  B2D_INLINE Size density() const noexcept { return _density; }
  B2D_INLINE void setDensity(const Size& density) noexcept { _density = density; }
  B2D_INLINE void setDensity(double w, double h) noexcept { _density.reset(w, h); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageInfo& operator=(const ImageInfo& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  IntSize _size;                         //!< Image size.
  uint8_t _depth;                        //!< Image depth.
  uint8_t _numPlanes;                    //!< Count of planes.
  uint8_t _progressive;                  //!< Whether the image is progressive (i.e. interlaced).
  uint8_t _reserved[1];                  //!< \internal

  uint32_t _numFrames;                   //!< Count of frames (0 = Unknown/Unspecified).
  Size _density;                         //!< Pixel density per one meter, can contain fractions.

  char _format[8];                       //!< Image format (related to the codec).
  char _compression[8];                  //!< Image compression (related to the codec).
};

// ============================================================================
// [b2d::ImageDecoderImpl]
// ============================================================================

//! Image decoder (impl).
class B2D_VIRTAPI ImageDecoderImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(ImageDecoderImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API ImageDecoderImpl() noexcept;
  B2D_API virtual ~ImageDecoderImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageDecoderImpl>
  B2D_INLINE T* addRef() noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Error error() const noexcept { return _error; }
  B2D_INLINE Error setError(Error err) noexcept { return (_error = err); }

  B2D_INLINE uint32_t frameIndex() const noexcept { return _frameIndex; }
  B2D_INLINE size_t bufferIndex() const noexcept { return _bufferIndex; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual Error reset() noexcept;
  B2D_API virtual Error setup(const uint8_t* p, size_t size) noexcept;
  B2D_API virtual Error decode(Image& dst, const uint8_t* p, size_t size) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Error _error;                          //!< Last decoding error, used to prevent from calling `decode()` again.
  uint32_t _frameIndex;                  //!< Current frame index.
  size_t _bufferIndex;                   //!< Position in source buffer.
  ImageInfo _imageInfo;                  //!< Image information.
};

// ============================================================================
// [b2d::ImageDecoder]
// ============================================================================

//! Image decoder.
class ImageDecoder : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdImageDecoder };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageDecoder() noexcept
    : Object(none().impl()) {}

  B2D_INLINE ImageDecoder(const ImageDecoder& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE ImageDecoder(ImageDecoder&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit ImageDecoder(ImageDecoderImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const ImageDecoder& none() noexcept { return Any::noneOf<ImageDecoder>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageDecoderImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the last decoding error.
  B2D_INLINE Error error() const noexcept { return impl()->error(); }
  //! Get position in source buffer.
  B2D_INLINE size_t bufferIndex() const noexcept { return impl()->bufferIndex(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE Error decode(Image& dst, const Buffer& src) noexcept {
    return impl()->decode(dst, src.data(), src.size());
  }

  B2D_INLINE Error decode(Image& dst, const void* data, size_t size) noexcept {
    return impl()->decode(dst, static_cast<const uint8_t*>(data), size);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageDecoder& operator=(const ImageDecoder& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE ImageDecoder& operator=(ImageDecoder&& other) noexcept {
    ImageDecoderImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const ImageDecoder& other) const noexcept { return impl() == other.impl(); }
  B2D_INLINE bool operator!=(const ImageDecoder& other) const noexcept { return impl() != other.impl(); }
};

// ============================================================================
// [b2d::ImageEncoderImpl]
// ============================================================================

//! Image encoder (impl).
class B2D_VIRTAPI ImageEncoderImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(ImageEncoderImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API ImageEncoderImpl() noexcept;
  B2D_API virtual ~ImageEncoderImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageEncoderImpl>
  B2D_INLINE T* addRef() noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Error error() const noexcept { return _error; }
  B2D_INLINE Error setError(Error err) noexcept { return (_error = err); }

  B2D_INLINE uint32_t frameIndex() const noexcept { return _frameIndex; }
  B2D_INLINE size_t bufferIndex() const noexcept { return _bufferIndex; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual Error reset() noexcept;
  B2D_API virtual Error encode(Buffer& dst, const Image& src) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Error _error;                          //!< Last encoding error.
  uint32_t _frameIndex;                  //!< Current frame index.
  size_t _bufferIndex;                   //!< Position in destination buffer.
};

// ============================================================================
// [b2d::ImageEncoder]
// ============================================================================

//! Image encoder.
class ImageEncoder : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdImageEncoder };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageEncoder() noexcept
    : Object(none().impl()) {}

  B2D_INLINE ImageEncoder(const ImageEncoder& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE ImageEncoder(ImageEncoder&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit ImageEncoder(ImageEncoderImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const ImageEncoder& none() noexcept { return Any::noneOf<ImageEncoder>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageEncoderImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the last encoding error.
  B2D_INLINE Error error() const noexcept { return impl()->error(); }
  //! Get destination buffer index.
  B2D_INLINE size_t bufferIndex() const noexcept { return impl()->bufferIndex(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Encode a given image `src`.
  B2D_INLINE Error encode(Buffer& dst, const Image& src) noexcept {
    return impl()->encode(dst, src);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageEncoder& operator=(const ImageEncoder& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE ImageEncoder& operator=(ImageEncoder&& other) noexcept {
    ImageEncoderImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const ImageEncoder& other) const noexcept { return impl() == other.impl(); }
  B2D_INLINE bool operator!=(const ImageEncoder& other) const noexcept { return impl() != other.impl(); }
};

// ============================================================================
// [b2d::ImageCodecImpl]
// ============================================================================

//! Image codec (impl).
class B2D_VIRTAPI ImageCodecImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(ImageCodecImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API ImageCodecImpl() noexcept;
  B2D_API virtual ~ImageCodecImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageCodecImpl>
  B2D_INLINE T* addRef() noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t features() const noexcept { return _features; }
  B2D_INLINE uint32_t formats() const noexcept { return _formats; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Setup codec `_vendor`, `_name`, `_mimeType`, and `_extensions`.
  //!
  //! The input string should contain all these separated by `\0` terminator.
  B2D_API void setupStrings(const char* strings) noexcept;

  B2D_API virtual int inspect(const uint8_t* data, size_t size) const noexcept;
  B2D_API virtual ImageDecoderImpl* createDecoderImpl() const noexcept;
  B2D_API virtual ImageEncoderImpl* createEncoderImpl() const noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _features;                    //!< Image codec features.
  uint32_t _formats;                     //!< Image codec formats.

  char _vendor[16];                      //!< Image codec vendor string, built-in codecs use "B2D".
  char _name[16];                        //!< Image codec name, like "BMP, "GIF", "JPEG", "PNG".
  char _mimeType[32];                    //!< Mime type.
  char _extensions[32];                  //!< Known file extensions used by this image codec separated by "|".
};

// ============================================================================
// [b2d::ImageCodec]
// ============================================================================

//! Image codec provides a unified interface for inspecting image data and
//! creating image decoders & encoders.
class ImageCodec : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdImageCodec };

  //! Codec features.
  enum Features : uint32_t {
    kFeatureDecoder  = 0x00000001u,      //!< Image codec supports decoding.
    kFeatureEncoder  = 0x00000002u,      //!< Image codec supports encoding.

    kFeatureLossless = 0x00000004u,      //!< Image codec supports lossless compression.
    kFeatureLossy    = 0x00000008u,      //!< Image codec supports loosy compression.
    kFeatureFrames   = 0x00000100u,      //!< Image codec supports multiple frames that can be animated.
    kFeatureIPTC     = 0x00000200u,      //!< Image codec supports IPTC metadata.
    kFeatureEXIF     = 0x00000400u,      //!< Image codec supports EXIF metadata.
    kFeatureXMP      = 0x00000800u       //!< Image codec supports XMP metadata.
  };

  //! Codec formats.
  enum Formats : uint32_t {
    kFormatIndexed1  = 0x00000001u,      //!< Image codec supports 1-bit indexed pixel format.
    kFormatIndexed2  = 0x00000002u,      //!< Image codec supports 2-bit indexed pixel format.
    kFormatIndexed4  = 0x00000004u,      //!< Image codec supports 4-bit indexed pixel format.
    kFormatIndexed8  = 0x00000008u,      //!< Image codec supports 8-bit indexed pixel format.

    kFormatGray1     = 0x00000010u,      //!< Image codec supports 1-bit grayscale pixel format.
    kFormatGray2     = 0x00000020u,      //!< Image codec supports 2-bit grayscale pixel format.
    kFormatGray4     = 0x00000040u,      //!< Image codec supports 4-bit grayscale pixel format.
    kFormatGray8     = 0x00000080u,      //!< Image codec supports 8-bit grayscale pixel format.
    kFormatGray16    = 0x00000100u,      //!< Image codec supports 16-bit grayscale pixel format.

    kFormatRGB16     = 0x00001000u,      //!< Image codec supports 16-bit RGB pixel format.
    kFormatRGB24     = 0x00002000u,      //!< Image codec supports 24-bit RGB pixel format.
    kFormatRGB32     = 0x00004000u,      //!< Image codec supports 32-bit RGB pixel format (more than 8 bits per components is allowed).
    kFormatRGB48     = 0x00008000u,      //!< Image codec supports 48-bit RGB pixel format (usually 16 bits per component).

    kFormatARGB16    = 0x00010000u,      //!< Image codec supports 16-bit ARGB pixel format.
    kFormatARGB32    = 0x00020000u,      //!< Image codec supports 32-bit ARGB pixel format.
    kFormatARGB64    = 0x00040000u,      //!< Image codec supports 64-bit ARGB pixel format.

    kFormatYCbCr24   = 0x01000000u       //!< Image codec supports 24-bit YCbCr pixel format (JPEG, TIFF).
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageCodec() noexcept
    : Object(none().impl()) {}

  B2D_INLINE ImageCodec(const ImageCodec& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE ImageCodec(ImageCodec&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit ImageCodec(ImageCodecImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const ImageCodec& none() noexcept { return Any::noneOf<ImageCodec>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = ImageCodecImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get image codec flags, see \ref Features.
  B2D_INLINE uint32_t features() const noexcept { return impl()->features(); }
  //! Get image codec formats, see \ref Formats.
  B2D_INLINE uint32_t formats() const noexcept { return impl()->formats(); }

  //! Get whether the image codec has a flag `flag`.
  B2D_INLINE bool hasFeature(uint32_t feature) const noexcept { return (features() & feature) != 0; }
  //! Get whether the image codec has a format `format`.
  B2D_INLINE bool hasFormat(uint32_t format) const noexcept { return (formats() & format) != 0; }

  //! Get whether the image codec is valid (i.e. not a null instance).
  B2D_INLINE bool isValid() const noexcept { return hasFeature(kFeatureDecoder | kFeatureEncoder); }
  //! Get whether the image codec supports decoding.
  B2D_INLINE bool hasDecoder() const noexcept { return hasFeature(kFeatureDecoder); }
  //! Get whether the image codec supports encoding.
  B2D_INLINE bool hasEncoder() const noexcept { return hasFeature(kFeatureEncoder); }
  //! Get whether the image codec supports lossless compression.
  B2D_INLINE bool hasLosslessCompression() const noexcept { return hasFeature(kFeatureLossless); }
  //! Get whether the image codec supports loosy compression.
  B2D_INLINE bool hasLoosyCompression() const noexcept { return hasFeature(kFeatureLossy); }
  //! Get whether the image codec supports multiple frames (animation).
  B2D_INLINE bool hasFrames() const noexcept { return hasFeature(kFeatureFrames); }
  //! Get whether the image codec supports IPTC metadata.
  B2D_INLINE bool hasIPTC() const noexcept { return hasFeature(kFeatureIPTC); }
  //! Get whether the image codec supports EXIF metadata.
  B2D_INLINE bool hasEXIF() const noexcept { return hasFeature(kFeatureEXIF); }
  //! Get whether the image codec supports XMP metadata.
  B2D_INLINE bool hasXMP() const noexcept { return hasFeature(kFeatureXMP); }

  //! Get whether the image codec supports 8-bit indexed pixel format.
  B2D_INLINE bool hasIndexed1() const noexcept { return hasFormat(kFormatIndexed1); }
  //! Get whether the image codec supports 8-bit indexed pixel format.
  B2D_INLINE bool hasIndexed2() const noexcept { return hasFormat(kFormatIndexed2); }
  //! Get whether the image codec supports 8-bit indexed pixel format.
  B2D_INLINE bool hasIndexed4() const noexcept { return hasFormat(kFormatIndexed4); }
  //! Get whether the image codec supports 8-bit indexed pixel format.
  B2D_INLINE bool hasIndexed8() const noexcept { return hasFormat(kFormatIndexed8); }
  //! Get whether the image codec supports 1-bit grayscale pixel format.
  B2D_INLINE bool hasGray1() const noexcept { return hasFormat(kFormatGray8); }
  //! Get whether the image codec supports 1-bit grayscale pixel format.
  B2D_INLINE bool hasGray2() const noexcept { return hasFormat(kFormatGray2); }
  //! Get whether the image codec supports 1-bit grayscale pixel format.
  B2D_INLINE bool hasGray4() const noexcept { return hasFormat(kFormatGray4); }
  //! Get whether the image codec supports 8-bit grayscale pixel format.
  B2D_INLINE bool hasGray8() const noexcept { return hasFormat(kFormatGray8); }
  //! Get whether the image codec supports 16-bit grayscale pixel format.
  B2D_INLINE bool hasGray16() const noexcept { return hasFormat(kFormatGray16); }
  //! Get whether the image codec supports 16-bit RGB pixel format.
  B2D_INLINE bool hasRGB16() const noexcept { return hasFormat(kFormatRGB16); }
  //! Get whether the image codec supports 24-bit RGB pixel format.
  B2D_INLINE bool hasRGB24() const noexcept { return hasFormat(kFormatRGB24); }
  //! Get whether the image codec supports 32-bit RGB pixel format.
  B2D_INLINE bool hasRGB32() const noexcept { return hasFormat(kFormatRGB32); }
  //! Get whether the image codec supports 64-bit RGB pixel format.
  B2D_INLINE bool hasRGB48() const noexcept { return hasFormat(kFormatRGB48); }
  //! Get whether the image codec supports 32-bit ARGB pixel format.
  B2D_INLINE bool hasARGB32() const noexcept { return hasFormat(kFormatARGB32); }
  //! Get whether the image codec supports 64-bit ARGB pixel format.
  B2D_INLINE bool hasARGB64() const noexcept { return hasFormat(kFormatARGB64); }

  //! Get the image codec name (i.e, "PNG", "JPEG", etc...).
  B2D_INLINE const char* name() const noexcept { return impl()->_name; }
  //! Get the image codec vendor (i.e. "B2D" for all built-in codecs).
  B2D_INLINE const char* vendor() const noexcept { return impl()->_vendor; }

  //! Get a mime-type associated with the image codec's format.
  B2D_INLINE const char* mimeType() const noexcept { return impl()->_mimeType; }
  //! Get a list of file extensions used to store image of this codec, separated by `"|"` character.
  B2D_INLINE const char* extensions() const noexcept { return impl()->_extensions; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_INLINE int inspect(const Buffer& buffer) const noexcept {
    return impl()->inspect(buffer.data(), buffer.size());
  }

  B2D_INLINE int inspect(const void* data, size_t size) const noexcept {
    return impl()->inspect(static_cast<const uint8_t*>(data), size);
  }

  B2D_INLINE ImageDecoder createDecoder() const noexcept { return ImageDecoder(impl()->createDecoderImpl()); }
  B2D_INLINE ImageEncoder createEncoder() const noexcept { return ImageEncoder(impl()->createEncoderImpl()); }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static B2D_API ImageCodecArray builtinCodecs() noexcept;

  static B2D_API ImageCodec codecByName(const ImageCodecArray& codecs, const char* name) noexcept;
  static B2D_API ImageCodec codecByData(const ImageCodecArray& codecs, const void* data, size_t size) noexcept;

  static B2D_INLINE ImageCodec codecByData(const ImageCodecArray& codecs, const Buffer& buffer) noexcept {
    return codecByData(codecs, buffer.data(), buffer.size());
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageCodec& operator=(const ImageCodec& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE ImageCodec& operator=(ImageCodec&& other) noexcept {
    ImageCodecImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const ImageCodec& other) const noexcept { return impl() == other.impl(); }
  B2D_INLINE bool operator!=(const ImageCodec& other) const noexcept { return impl() != other.impl(); }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_IMAGECODEC_H
