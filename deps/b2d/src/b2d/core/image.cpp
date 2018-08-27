// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/image.h"
#include "../core/math.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Image - Null]
// ============================================================================

static Wrap<ImageImpl> gImageImplNone;

// ============================================================================
// [b2d::Image - Utils]
// ============================================================================

static B2D_INLINE void _bImageCopyRect(
  uint8_t* dstData, intptr_t dstStride,
  const uint8_t* srcIata, intptr_t srcStride,
  size_t wBytes, unsigned int h) noexcept {

  for (unsigned int y = h; y; y--, dstData += dstStride, srcIata += srcStride)
    std::memcpy(dstData, srcIata, wBytes);
}

// ============================================================================
// [b2d::Image - Impl]
// ============================================================================

ImageImpl::ImageImpl(int w, int h, uint32_t pixelFormat, uint8_t* pixelData, intptr_t stride, DestroyHandler destroyHandler) noexcept
  : ObjectImpl(Any::kTypeIdImage),
    _implType(Image::kImplTypeMemory),
    _pfi(PixelFormatInfo::table[pixelFormat]),
    _pixelData(pixelData),
    _writer(nullptr),
    _lockCount(0),
    _stride(stride),
    _size(w, h),
    _destroyHandler(destroyHandler) {}

ImageImpl::~ImageImpl() noexcept {
  if (_destroyHandler)
    _destroyHandler(this);
}

static ImageImpl* ImageImpl_newInternal(int w, int h, uint32_t pixelFormat) noexcept {
  B2D_ASSERT(w > 0 && h > 0);
  B2D_ASSERT(PixelFormat::isValid(pixelFormat));

  uint32_t bpp = PixelFormat::bppFromId(pixelFormat);
  size_t stride = ImageImpl::strideForWidth(w, bpp);

  B2D_ASSERT(stride != 0);

  uint32_t structSize = sizeof(ImageImpl);
  uint32_t alignSize = 16;

  Support::OverflowFlag of;
  size_t dataSize = Support::safeMulAdd<size_t>(structSize + alignSize, size_t(h), stride, &of);

  if (B2D_UNLIKELY(of))
    return nullptr;

  uint8_t* d = static_cast<uint8_t*>(Allocator::systemAlloc(dataSize));
  if (B2D_UNLIKELY(!d)) return nullptr;

  uint8_t* pixelData = Support::alignUp<uint8_t*>(d + structSize, alignSize);
  return new(d) ImageImpl(w, h, pixelFormat, pixelData, intptr_t(stride), nullptr);
}

static ImageImpl* ImageImpl_newExternal(int w, int h, uint32_t pixelFormat, void* pixelData, intptr_t stride, ImageImpl::DestroyHandler destroyHandler) noexcept {
  B2D_ASSERT(w > 0 && h > 0);
  B2D_ASSERT(stride != 0);
  B2D_ASSERT(PixelFormat::isValid(pixelFormat));

  uint8_t* d = static_cast<uint8_t*>(Allocator::systemAlloc(sizeof(ImageImpl)));
  if (B2D_UNLIKELY(!d)) return nullptr;

  return new(d) ImageImpl(w, h, pixelFormat, static_cast<uint8_t*>(pixelData), stride, destroyHandler);
}

// ============================================================================
// [b2d::Image - Construction / Destruction]
// ============================================================================

Image::Image(int w, int h, uint32_t pixelFormat) noexcept
  : Object(static_cast<ImageImpl*>(nullptr)) {

  if (w > 0 && h > 0 && PixelFormat::isValid(pixelFormat)) {
    _impl = ImageImpl_newInternal(w, h, pixelFormat);
    if (_impl) return;
  }

  _impl = Any::noneOf<Image>().impl();
}

// ============================================================================
// [b2d::Image - Sharing]
// ============================================================================

Error Image::_detach() noexcept {
  ImageImpl* thisI = impl();
  if (!thisI->isShared() || thisI->isNone())
    return kErrorOk;

  // Should be catched by _bImageImplNull.
  B2D_ASSERT(thisI->size().isValid());
  B2D_ASSERT(thisI->stride() != 0);

  int w = thisI->w();
  int h = thisI->h();

  ImageImpl* newI = ImageImpl_newInternal(w, h, thisI->pixelFormat());
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  _bImageCopyRect(
    newI->pixelData(), newI->stride(),
    thisI->pixelData(), thisI->stride(), Math::min<intptr_t>(newI->stride(), thisI->stride()), h);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Image - Reset]
// ============================================================================

Error Image::reset() noexcept {
  return AnyInternal::replaceImpl(this, none().impl());
}

// ============================================================================
// [b2d::Image - Create]
// ============================================================================

Error Image::create(int w, int h, uint32_t pixelFormat) noexcept {
  if (w <= 0 || h <= 0 || !PixelFormat::isValid(pixelFormat))
    return DebugUtils::errored(kErrorInvalidArgument);

  ImageImpl* newI = ImageImpl_newInternal(w, h, pixelFormat);
  if (newI == nullptr)
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

Error Image::create(int w, int h, uint32_t pixelFormat, void* pixelData, intptr_t stride, DestroyHandler destroyHandler) noexcept {
  if (w <= 0 || h <= 0 || !PixelFormat::isValid(pixelFormat))
    return DebugUtils::errored(kErrorInvalidArgument);

  ImageImpl* newI = ImageImpl_newExternal(w, h, pixelFormat, pixelData, stride, destroyHandler);
  if (newI == nullptr)
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Image - SetImage]
// ============================================================================

Error Image::setDeep(const Image& other) noexcept {
  // TODO:
  return DebugUtils::errored(kErrorNotImplemented);
}

// ============================================================================
// [b2d::Image - Equality]
// ============================================================================

bool Image::_eq(const Image* a, const Image* b) noexcept {
  const ImageImpl* aImpl = a->impl();
  const ImageImpl* bImpl = b->impl();

  if (aImpl == bImpl)
    return true;

  if (aImpl->size() != bImpl->size() || aImpl->pixelFormat() != bImpl->pixelFormat())
    return false;

  uint32_t w = uint32_t(aImpl->w());
  uint32_t h = uint32_t(aImpl->h());

  const uint8_t* aData = aImpl->pixelData();
  const uint8_t* bData = bImpl->pixelData();

  intptr_t aStride = aImpl->stride();
  intptr_t bStride = bImpl->stride();

  size_t cmpSize = w * aImpl->bpp();
  // Can only happen when both images are empty.
  if (cmpSize == 0)
    return true;

  for (uint32_t y = 0; y < h; y++) {
    if (std::memcmp(aData, bData, cmpSize) != 0)
      return false;

    aData += aStride;
    bData += bStride;
  }

  return true;
}

// ============================================================================
// [b2d::Image - Scale]
// ============================================================================

Error Image::scale(Image& dst, const Image& src, const IntSize& size, const ImageScaler::Params& params) noexcept {
  if (src.empty()) {
    dst.reset();
    return kErrorOk;
  }

  ImageScaler scaler;
  B2D_PROPAGATE(scaler.create(size, src.size(), params));

  ImageImpl* srcI = src.impl();
  uint32_t pixelFormat = srcI->pixelFormat();

  int tw = scaler.dstWidth();
  int th = scaler.srcHeight();

  Image tmp;
  ImageBuffer buf;

  if (th == scaler.dstHeight() || tw == scaler.srcWidth()) {
    // Move to `tmp` so it's not destroyed by `dst.create()`.
    if (&dst == &src) tmp = src;

    // Only horizontal or vertical scale.
    B2D_PROPAGATE(dst.create(scaler.dstWidth(), scaler.dstHeight(), pixelFormat));
    B2D_PROPAGATE(dst.lock(buf, nullptr));

    if (th == scaler.dstHeight())
      scaler.processHorzData(buf.pixelData(), buf.stride(), srcI->pixelData(), srcI->stride(), pixelFormat);
    else
      scaler.processVertData(buf.pixelData(), buf.stride(), srcI->pixelData(), srcI->stride(), pixelFormat);

    dst.unlock(buf, nullptr);
  }
  else {
    // Both horizontal and vertical scale.
    B2D_PROPAGATE(tmp.create(tw, th, pixelFormat));
    B2D_PROPAGATE(tmp.lock(buf, nullptr));

    scaler.processHorzData(buf.pixelData(), buf.stride(), srcI->pixelData(), srcI->stride(), pixelFormat);
    tmp.unlock(buf, nullptr);

    srcI = tmp.impl();
    B2D_PROPAGATE(dst.create(scaler.dstWidth(), scaler.dstHeight(), pixelFormat));
    B2D_PROPAGATE(dst.lock(buf, nullptr));

    scaler.processVertData(buf.pixelData(), buf.stride(), srcI->pixelData(), srcI->stride(), pixelFormat);
    dst.unlock(buf, nullptr);
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::Image - Lock / Unlock]
// ============================================================================

Error Image::lock(ImageBuffer& buffer, void* writer) noexcept {
  ImageImpl* thisI = impl();
  uint32_t access = kImageAccessReadOnly;

  if (thisI->w() == 0)
    return DebugUtils::errored(kErrorInvalidState);

  if (writer) {
    B2D_PROPAGATE(detach());
    thisI = impl();

    void* expected = nullptr;
    if (!thisI->_writer.compare_exchange_strong(expected, writer))
      return DebugUtils::errored(kErrorResourceBusy);

    access = kImageAccessReadWrite;
  }

  buffer.reset(thisI->pixelData(), thisI->pixelFormatInfo(), access, thisI->stride(), thisI->size());
  thisI->_lockCount.fetch_add(1, std::memory_order_relaxed);

  return kErrorOk;
}

Error Image::unlock(ImageBuffer& buffer, void* writer) noexcept {
  ImageImpl* thisI = impl();

  if (thisI->pixelData() != buffer.pixelData())
    return DebugUtils::errored(kErrorResourceMismatch);

  if (writer) {
    void* expected = writer;
    if (!thisI->_writer.compare_exchange_strong(expected, nullptr))
      return DebugUtils::errored(kErrorResourceMismatch);
  }

  thisI->_lockCount.fetch_sub(1, std::memory_order_acq_rel);
  buffer._pixelData = nullptr;

  return kErrorOk;
}

// ============================================================================
// [b2d::Image - Runtime Handlers]
// ============================================================================

void ImageOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  ImageImpl* impl = gImageImplNone.init(0, 0, PixelFormat::kNone, nullptr, 0, nullptr);
  impl->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdImage, impl);
}

B2D_END_NAMESPACE
