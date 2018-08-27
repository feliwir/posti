// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_IMAGEUTILS_H
#define _B2D_CORE_IMAGEUTILS_H

#include "../core/image.h"
#include "../core/imagecodec.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ImageUtils]
// ============================================================================

namespace ImageUtils {
  B2D_API Error readImageFromFile(Image& dst, const ImageCodecArray& codecs, const char* fileName) noexcept;
  B2D_API Error readImageFromData(Image& dst, const ImageCodecArray& codecs, const void* data, size_t size) noexcept;

  static B2D_INLINE Error readImageFromBuffer(Image& dst, const ImageCodecArray& codecs, const Buffer& buffer) noexcept {
    return readImageFromData(dst, codecs, buffer.data(), buffer.size());
  }

  B2D_API Error writeImageToFile(const char* fileName, const ImageCodec& codec, const Image& image) noexcept;
  B2D_API Error writeImageToBuffer(Buffer& dst, const ImageCodec& codec, const Image& image) noexcept;
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_IMAGEUTILS_H
