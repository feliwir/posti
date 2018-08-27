// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/filesystem.h"
#include "../core/imageutils.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::ImageUtils - Image Read / Write]
// ============================================================================

Error ImageUtils::readImageFromFile(Image& dst, const ImageCodecArray& codecs, const char* fileName) noexcept {
  Buffer buf;
  B2D_PROPAGATE(FileUtils::readFile(fileName, buf));

  ImageCodec codec = ImageCodec::codecByData(codecs, buf);
  if (B2D_UNLIKELY(codec.isNone()))
    return DebugUtils::errored(kErrorCodecNotFound);

  if (B2D_UNLIKELY(!codec.hasDecoder()))
    return DebugUtils::errored(kErrorCodecNoDecoder);

  ImageDecoder decoder = codec.createDecoder();
  if (B2D_UNLIKELY(decoder.isNone()))
    return DebugUtils::errored(kErrorNoMemory);

  return decoder.decode(dst, buf);
}

Error ImageUtils::readImageFromData(Image& dst, const ImageCodecArray& codecs, const void* data, size_t size) noexcept {
  ImageCodec codec = ImageCodec::codecByData(codecs, data, size);
  if (B2D_UNLIKELY(codec.isNone()))
    return DebugUtils::errored(kErrorCodecNotFound);

  if (B2D_UNLIKELY(!codec.hasDecoder()))
    return DebugUtils::errored(kErrorCodecNoDecoder);

  ImageDecoder decoder = codec.createDecoder();
  if (B2D_UNLIKELY(decoder.isNone()))
    return DebugUtils::errored(kErrorNoMemory);

  return decoder.decode(dst, data, size);
}

Error ImageUtils::writeImageToFile(const char* fileName, const ImageCodec& codec, const Image& image) noexcept {
  Buffer buf;
  B2D_PROPAGATE(writeImageToBuffer(buf, codec, image));

  return FileUtils::writeFile(fileName, buf);
}

Error ImageUtils::writeImageToBuffer(Buffer& dst, const ImageCodec& codec, const Image& image) noexcept {
  if (B2D_UNLIKELY(!codec.hasEncoder()))
    return DebugUtils::errored(kErrorCodecNoEncoder);

  ImageEncoder encoder = codec.createEncoder();
  if (B2D_UNLIKELY(encoder.isNone()))
    return DebugUtils::errored(kErrorNoMemory);

  return encoder.encode(dst, image);
}

B2D_END_NAMESPACE
