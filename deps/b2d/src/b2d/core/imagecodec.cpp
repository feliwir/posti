// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/bmpcodec_p.h"
#include "../codec/jpegcodec_p.h"
#include "../codec/pngcodec_p.h"
#include "../core/imagecodec.h"
#include "../core/lock.h"
#include "../core/runtime.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

static Wrap<ImageCodecArray> gImageCodecBuiltins;

// ============================================================================
// [b2d::ImageDecoderImpl - Construction / Destruction]
// ============================================================================

ImageDecoderImpl::ImageDecoderImpl() noexcept
  : ObjectImpl(Any::kTypeIdImageDecoder),
    _error(kErrorOk),
    _frameIndex(0),
    _bufferIndex(0) {}
ImageDecoderImpl::~ImageDecoderImpl() noexcept {}

// ============================================================================
// [b2d::ImageDecoderImpl - Interface]
// ============================================================================

Error ImageDecoderImpl::reset() noexcept {
  if (isNone())
    return _error;

  _error = kErrorOk;
  _frameIndex = 0;
  _bufferIndex = 0;
  _imageInfo.reset();

  return kErrorOk;
}

Error ImageDecoderImpl::setup(const uint8_t* p, size_t size) noexcept {
  B2D_UNUSED(p);
  B2D_UNUSED(size);

  return _error;
}

Error ImageDecoderImpl::decode(Image& dst, const uint8_t* p, size_t size) noexcept {
  B2D_UNUSED(dst);
  B2D_UNUSED(p);
  B2D_UNUSED(size);

  return _error;
}

// ============================================================================
// [b2d::ImageEncoderImpl - Construction / Destruction]
// ============================================================================

ImageEncoderImpl::ImageEncoderImpl() noexcept
  : ObjectImpl(Any::kTypeIdImageEncoder),
    _error(kErrorOk),
    _frameIndex(0),
    _bufferIndex(0) {}
ImageEncoderImpl::~ImageEncoderImpl() noexcept {}

// ============================================================================
// [b2d::ImageEncoderImpl - Interface]
// ============================================================================

Error ImageEncoderImpl::reset() noexcept {
  if (isNone())
    return _error;

  _error = kErrorOk;
  _frameIndex = 0;
  _bufferIndex = 0;
  return kErrorOk;
}

Error ImageEncoderImpl::encode(Buffer& dst, const Image& src) noexcept {
  B2D_UNUSED(dst);
  B2D_UNUSED(src);

  return _error;
}

// ============================================================================
// [b2d::ImageCodecImpl - Construction / Destruction]
// ============================================================================

ImageCodecImpl::ImageCodecImpl() noexcept
  : ObjectImpl(Any::kTypeIdImageCodec) {}
ImageCodecImpl::~ImageCodecImpl() noexcept {}

// ============================================================================
// [b2d::ImageCodecImpl - Interface]
// ============================================================================

void ImageCodecImpl::setupStrings(const char* strings) noexcept {
  const char* src = strings;

  for (uint32_t propertyId = 0; propertyId < 4; propertyId++) {
    char* dst;
    size_t size;

    switch (propertyId) {
      case 0 : dst = _vendor    ; size = B2D_ARRAY_SIZE(_vendor    ); break;
      case 1 : dst = _name      ; size = B2D_ARRAY_SIZE(_name      ); break;
      case 2 : dst = _mimeType  ; size = B2D_ARRAY_SIZE(_mimeType  ); break;
      case 3 : dst = _extensions; size = B2D_ARRAY_SIZE(_extensions); break;
    }

    size_t i = 0;
    for (;;) {
      char c = src[i];
      dst[i++] = c;

      if (c == 0)
        break;

      // If this happens there is a bug in the codec or our codec's buffers
      // are too small. Report in debug-mode.
      B2D_ASSERT(i < size);
    }
    src += i;

    // Zero the rest of the destination buffer.
    while (i < size) {
      dst[i++] = 0;
    }
  }
}

int ImageCodecImpl::inspect(const uint8_t* data, size_t size) const noexcept {
  B2D_UNUSED(data);
  B2D_UNUSED(size);

  return 0;
}

ImageDecoderImpl* ImageCodecImpl::createDecoderImpl() const noexcept { return ImageDecoder::none().impl(); }
ImageEncoderImpl* ImageCodecImpl::createEncoderImpl() const noexcept { return ImageEncoder::none().impl(); }

// ============================================================================
// [b2d::ImageCodec - Statics]
// ============================================================================

ImageCodecArray ImageCodec::builtinCodecs() noexcept { return gImageCodecBuiltins; }

ImageCodec ImageCodec::codecByName(const ImageCodecArray& codecs, const char* name) noexcept {
  ArrayIterator<ImageCodec> it(codecs);

  while (it.isValid()) {
    const ImageCodec& codec = it.item();
    if (::strcmp(codec.name(), name) == 0)
      return codec;
    it.next();
  }

  return ImageCodec();
}

ImageCodec ImageCodec::codecByData(const ImageCodecArray& codecs, const void* data, size_t size) noexcept {
  ArrayIterator<ImageCodec> it(codecs);

  int bestScore = 0;
  const ImageCodec* candidate = &ImageCodec::none();

  while (it.isValid()) {
    const ImageCodec& codec = it.item();
    int score = codec.inspect(data, size);

    if (bestScore < score) {
      bestScore = score;
      candidate = &codec;
    }

    it.next();
  }

  return ImageCodec(*candidate);
}

// ============================================================================
// [b2d::ImageCodec - Runtime Handlers]
// ============================================================================

static Wrap<ImageCodecImpl> gImageCodecImplNone;
static Wrap<ImageDecoderImpl> gImageDecoderImplNone;
static Wrap<ImageEncoderImpl> gImageEncoderImplNone;

static void ImageCodecOnShutdown(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);
  gImageCodecBuiltins.destroy();
}

void ImageCodecOnInit(Runtime::Context& rt) noexcept {
  // Initialize built-in none instances.
  ImageCodecImpl* codecI = gImageCodecImplNone.init();
  codecI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdImageCodec, codecI);

  ImageDecoderImpl* decoderI = gImageDecoderImplNone.init();
  decoderI->_commonData._setToBuiltInNone();
  decoderI->setError(kErrorInvalidState);
  Any::_initNone(Any::kTypeIdImageDecoder, decoderI);

  ImageEncoderImpl* encoderI = gImageEncoderImplNone.init();
  encoderI->_commonData._setToBuiltInNone();
  encoderI->setError(kErrorInvalidState);
  Any::_initNone(Any::kTypeIdImageEncoder, encoderI);

  // Register built-in codecs.
  ImageCodecArray* codecList = gImageCodecBuiltins.init();
  ImageCodecOnInitBmp(codecList);
  ImageCodecOnInitJpeg(codecList);
  ImageCodecOnInitPng(codecList);
  rt.addShutdownHandler(ImageCodecOnShutdown);
}

B2D_END_NAMESPACE
