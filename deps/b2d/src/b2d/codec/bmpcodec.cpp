// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/bmpcodec_p.h"
#include "../core/math.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [Constants]
// ============================================================================

//! \internal
enum BmpHeaderSize : uint32_t {
  kBmpHeaderSizeOs2V1 = 12,
  kBmpHeaderSizeWinV1 = 40,
  kBmpHeaderSizeWinV2 = 52,
  kBmpHeaderSizeWinV3 = 56,
  kBmpHeaderSizeWinV4 = 108,
  kBmpHeaderSizeWinV5 = 124
};

//! \internal
enum BmpRLECmd : uint32_t {
  kBmpRLELine = 0,
  kBmpRLEStop = 1,
  kBmpRLEMove = 2,
  kBmpRLELast = 2
};

enum : uint32_t {
  // Spec says that skipped pixels contain background color, for us background
  // is a fully transparent pixel. If used, we have to inform the caller to set
  // the pixel format to `PixelFormat::kPRGB32` from `PixelFormat::kXRGB32`.
  kRleBackground = 0x00000000u
};

// ============================================================================
// [b2d::Bmp - Helpers]
// ============================================================================

static B2D_INLINE bool bBmpCheckHeaderSize(uint32_t headerSize) noexcept {
  return headerSize == kBmpHeaderSizeOs2V1 ||
         headerSize == kBmpHeaderSizeWinV1 ||
         headerSize == kBmpHeaderSizeWinV2 ||
         headerSize == kBmpHeaderSizeWinV3 ||
         headerSize == kBmpHeaderSizeWinV4 ||
         headerSize == kBmpHeaderSizeWinV5 ;
}

static B2D_INLINE bool bBmpCheckDepth(uint32_t depth) noexcept {
  return depth ==  1 ||
         depth ==  4 ||
         depth ==  8 ||
         depth == 16 ||
         depth == 24 ||
         depth == 32 ;
}

static B2D_INLINE bool bBmpCheckImageSize(const IntSize& size) noexcept {
  return uint32_t(size._w) <= Image::kMaxSize &&
         uint32_t(size._h) <= Image::kMaxSize ;
}

static B2D_INLINE bool bBmpCheckBitMasks(const uint32_t* mask, uint32_t size) noexcept {
  uint32_t tmp = 0;

  for (uint32_t i = 0; i < size; i++) {
    uint32_t m = mask[i];

    // RGB masks can't be zero.
    if (m == 0 && i != 0)
      return false;

    // Mask has to have consecutive bits set, mask like 000110011 is not allowed.
    if (m != 0 && !Support::isConsecutiveBitMask(m))
      return false;

    // Mask can't overlap with other.
    if ((tmp & m) != 0)
      return false;

    tmp |= m;
  }

  return true;
}

static B2D_INLINE Error bBmpDecodeRLE4(
  uint8_t* dstLine, intptr_t dstStride,
  const uint8_t* p, size_t size, uint32_t w, uint32_t h, const uint32_t* pal) noexcept {

  uint8_t* dstData = dstLine;
  const uint8_t* pEnd = p + size;

  uint32_t x = 0;
  uint32_t y = 0;

  for (;;) {
    if ((size_t)(pEnd - p) < 2)
      return DebugUtils::errored(kErrorCodecIncompleteData);

    uint32_t b0 = p[0];
    uint32_t b1 = p[1]; p += 2;

    if (b0 != 0) {
      // RLE-Fill (b0 = Size, b1 = Pattern).
      uint32_t c0 = pal[b1 >> 4];
      uint32_t c1 = pal[b1 & 15];

      uint32_t i = Math::min<uint32_t>(b0, w - x);
      for (x += i; i >= 2; i -= 2, dstData += 8) {
        Support::writeU32a(dstData + 0, c0);
        Support::writeU32a(dstData + 4, c1);
      }

      if (i) {
        Support::writeU32a(dstData + 0, c0);
        dstData += 4;
      }
    }
    else if (b1 > kBmpRLELast) {
      // Absolute (b1 = Size).
      uint32_t i = Math::min<uint32_t>(b1, w - x);
      uint32_t reqBytes = ((b1 + 3) >> 1) & ~0x1;

      if ((size_t)(pEnd - p) < reqBytes)
        return DebugUtils::errored(kErrorCodecIncompleteData);

      for (x += i; i >= 4; i -= 4, dstData += 16) {
        b0 = p[0];
        b1 = p[1]; p += 2;

        Support::writeU32a(dstData +  0, pal[b0 >> 4]);
        Support::writeU32a(dstData +  4, pal[b0 & 15]);
        Support::writeU32a(dstData +  8, pal[b1 >> 4]);
        Support::writeU32a(dstData + 12, pal[b1 & 15]);
      }

      if (i) {
        b0 = p[0];
        b1 = p[1]; p += 2;

        Support::writeU32a(dstData, pal[b0 >> 4]);
        dstData += 4;

        if (--i) {
          Support::writeU32a(dstData, pal[b0 & 15]);
          dstData += 4;

          if (--i) {
            Support::writeU32a(dstData, pal[b1 >> 4]);
            dstData += 4;
          }
        }
      }
    }
    else {
      // RLE-Skip - fill by background pixel.
      uint32_t toX = x;
      uint32_t toY = y;

      if (b1 == kBmpRLELine) {
        toX = 0;
        toY++;
      }
      else if (b1 == kBmpRLEMove) {
        if ((size_t)(pEnd - p) < 2)
          return DebugUtils::errored(kErrorCodecIncompleteData);

        toX += p[0];
        toY += p[1]; p += 2;

        if (toX > w || toY > h)
          return DebugUtils::errored(kErrorCodecInvalidRLE);
      }
      else {
        toX = 0;
        toY = h;
      }

      for (; y < toY; y++) {
        for (x = w - x; x; x--, dstData += 4) {
          Support::writeU32a(dstData, kRleBackground);
        }

        dstLine += dstStride;
        dstData = dstLine;
      }

      for (; x < toX; x++, dstData += 4) {
        Support::writeU32a(dstData, kRleBackground);
      }

      if (b1 == kBmpRLEStop || y == h)
        return kErrorOk;
    }
  }
}

static B2D_INLINE Error bBmpDecodeRLE8(
  uint8_t* dstLine, intptr_t dstStride,
  const uint8_t* p, size_t size, uint32_t w, uint32_t h, const uint32_t* pal) noexcept {

  uint8_t* dstData = dstLine;
  const uint8_t* pEnd = p + size;

  uint32_t x = 0;
  uint32_t y = 0;

  for (;;) {
    if ((size_t)(pEnd - p) < 2)
      return DebugUtils::errored(kErrorCodecIncompleteData);

    uint32_t b0 = p[0];
    uint32_t b1 = p[1]; p += 2;

    if (b0 != 0) {
      // RLE-Fill (b0 = Size, b1 = Pattern).
      uint32_t c0 = pal[b1];
      uint32_t i = Math::min<uint32_t>(b0, w - x);

      for (x += i; i; i--, dstData += 4) {
        Support::writeU32a(dstData, c0);
      }
    }
    else if (b1 > kBmpRLELast) {
      // Absolute (b1 = Size).
      uint32_t i = Math::min<uint32_t>(b1, w - x);
      uint32_t reqBytes = ((b1 + 1) >> 1) << 1;

      if ((size_t)(pEnd - p) < reqBytes)
        return DebugUtils::errored(kErrorCodecIncompleteData);

      for (x += i; i >= 2; i -= 2, dstData += 8) {
        b0 = p[0];
        b1 = p[1]; p += 2;

        Support::writeU32a(dstData + 0, pal[b0]);
        Support::writeU32a(dstData + 4, pal[b1]);
      }

      if (i) {
        b0 = p[0]; p += 2;

        Support::writeU32a(dstData, pal[b0]);
        dstData += 4;
      }
    }
    else {
      // RLE-Skip - fill by background pixel.
      uint32_t toX = x;
      uint32_t toY = y;

      if (b1 == kBmpRLELine) {
        toX = 0;
        toY++;
      }
      else if (b1 == kBmpRLEMove) {
        if ((size_t)(pEnd - p) < 2)
          return DebugUtils::errored(kErrorCodecIncompleteData);

        toX += p[0];
        toY += p[1]; p += 2;

        if (toX > w || toY > h)
          return DebugUtils::errored(kErrorCodecInvalidRLE);
      }
      else {
        toX = 0;
        toY = h;
      }

      for (; y < toY; y++) {
        for (x = w - x; x; x--, dstData += 4) {
          Support::writeU32a(dstData, kRleBackground);
        }

        dstLine += dstStride;
        dstData = dstLine;
      }

      for (; x < toX; x++, dstData += 4) {
        Support::writeU32a(dstData, kRleBackground);
      }

      if (b1 == kBmpRLEStop || y == h)
        return kErrorOk;
    }
  }
}

// ============================================================================
// [b2d::BmpDecoderImpl - Construction / Destruction]
// ============================================================================

BmpDecoderImpl::BmpDecoderImpl() noexcept { reset(); }
BmpDecoderImpl::~BmpDecoderImpl() noexcept {}

// ============================================================================
// [b2d::BmpDecoderImpl - Interface]
// ============================================================================

Error BmpDecoderImpl::reset() noexcept {
  Base::reset();

  std::memset(&_bmp, 0, sizeof(BmpHeader));
  std::memset(&_bmpMasks, 0, sizeof(_bmpMasks));

  _bmpStride = 0;
  return kErrorOk;
}

Error BmpDecoderImpl::setup(const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  if (_bufferIndex != 0)
    return setError(DebugUtils::errored(kErrorInvalidState));

  // Signature + BmpFile header + BmpInfo header size (18 bytes total).
  const size_t kBmpMinSize = 18;
  if (size < kBmpMinSize)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));

  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + size;

  std::memcpy(&_bmp.file, p, 18);
  p += 18;

  // Check BMP signature.
  if (_bmp.file.signature[0] != 'B' || _bmp.file.signature[1] != 'M')
    return setError(DebugUtils::errored(kErrorCodecInvalidSignature));

  if (!B2D_ARCH_LE) {
    _bmp.file.byteSwap();
    _bmp.info.byteSwapSize();
  }

  // First check if the header is supported by the BmpDecoder.
  uint32_t headerSize = _bmp.info.headerSize;
  uint32_t fileAndInfoHeaderSize = 14 + headerSize;

  if (!bBmpCheckHeaderSize(headerSize))
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  // Read [BmpInfoHeader].
  if ((size_t)(pEnd - p) < headerSize - 4)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));

  std::memcpy(reinterpret_cast<char*>(&_bmp.info) + 4, p, headerSize - 4);
  p += headerSize - 4;

  int32_t w, h;
  uint32_t depth;
  uint32_t planes;
  uint32_t compression = kBmpCompressionRGB;
  bool rleUsed = false;

  if (headerSize == kBmpHeaderSizeOs2V1) {
    // Handle OS/2 BMP.
    if (!B2D_ARCH_LE)
      _bmp.info.os2.byteSwapExceptSize();

    w      = _bmp.info.os2.width;
    h      = _bmp.info.os2.height;
    depth  = _bmp.info.os2.bitsPerPixel;
    planes = _bmp.info.os2.planes;

    // Convert to Windows BMP, there is no difference except the header.
    _bmp.info.win.width = uint32_t(_imageInfo._size._w);
    _bmp.info.win.height = uint32_t(_imageInfo._size._h);
    _bmp.info.win.planes = uint16_t(_imageInfo._numPlanes);
    _bmp.info.win.bitsPerPixel = uint16_t(_imageInfo._depth);
    _bmp.info.win.compression = kBmpCompressionRGB;
  }
  else {
    // Handle Windows BMP.
    if (!B2D_ARCH_LE)
      _bmp.info.win.byteSwapExceptSize();

    w = _bmp.info.win.width;
    h = _bmp.info.win.height;
    depth = _bmp.info.win.bitsPerPixel;
    planes = _bmp.info.win.planes;
    compression = _bmp.info.win.compression;
  }

  // Num planes has to be one.
  if (planes != 1)
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  if (h < 0)
    h = -h;

  _imageInfo.setSize(w, h);
  _imageInfo.setDepth(depth);
  _imageInfo.setNumPlanes(planes);
  _imageInfo.setNumFrames(1);

  // Check if the compression field is correct when depth <= 8.
  if (compression != kBmpCompressionRGB) {
    if (depth <= 8) {
      rleUsed = (depth == 4 && compression == kBmpCompressionRLE4) ||
                (depth == 8 && compression == kBmpCompressionRLE8) ;

      if (!rleUsed)
        return setError(DebugUtils::errored(kErrorCodecInvalidFormat));
    }
  }

  if (_bmp.file.imageOffset < fileAndInfoHeaderSize)
    return setError(DebugUtils::errored(kErrorBmpInvalidImageOffset));

  // Check if the size is valid.
  if (!bBmpCheckImageSize(_imageInfo._size))
    return setError(DebugUtils::errored(kErrorBmpInvalidImageSize));

  // Check if the depth is valid.
  if (!bBmpCheckDepth(_imageInfo._depth))
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  // Calculate the BMP stride aligned to 32 bits.
  _bmpStride = (((uint32_t(_imageInfo._size._w) *
                  uint32_t(_imageInfo._depth) + 7) >> 3) + 3) & ~3;
  uint32_t imageSize = _bmpStride * uint32_t(_imageInfo._size._h);

  // 1. OS/2 format doesn't specify imageSize, it's always calculated.
  // 2. BMP allows `imageSize` to be zero in case of uncompressed bitmaps.
  if (headerSize == kBmpHeaderSizeOs2V1 || (_bmp.info.win.imageSize == 0 && !rleUsed))
    _bmp.info.win.imageSize = imageSize;

  // Check if the `imageSize` matches the calculated. It's malformed if it doesn't.
  if (!rleUsed && _bmp.info.win.imageSize < imageSize)
    return setError(DebugUtils::errored(kErrorCodecInvalidData));

  switch (_imageInfo._depth) {
    // Indexed.
    case 1:
    case 2:
    case 4:
    case 8:
      break;

    // Setup 16-bit RGB (555).
    case 16:
      _bmpMasks[1] = 0x7C00;
      _bmpMasks[2] = 0x03E0;
      _bmpMasks[3] = 0x001F;
      break;

    // Setup 24/32-bit RGB (888).
    case 24:
    case 32:
      _bmpMasks[1] = 0x00FF0000;
      _bmpMasks[2] = 0x0000FF00;
      _bmpMasks[3] = 0x000000FF;
      break;
  }

  if (headerSize == kBmpHeaderSizeWinV1) {
    // Use BITFIELDS if specified.
    uint32_t compression = _bmp.info.win.compression;

    if (compression == kBmpCompressionBitFields || compression == kBmpCompressionAlphaBitFields) {
      uint32_t channels = 3 + (compression == kBmpCompressionAlphaBitFields);
      if (depth != 16 && depth != 32)
        return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

      if ((size_t)(pEnd - p) < channels * 4)
        return setError(DebugUtils::errored(kErrorCodecIncompleteData));

      for (uint32_t i = 0; i < 3; i++)
        _bmpMasks[i + 1] = Support::readU32uLE(p + i * 4);

      if (channels == 4)
        _bmpMasks[0] = Support::readU32uLE(p + 12);

      p += channels * 4;
    }
  }
  else if (headerSize >= kBmpHeaderSizeWinV2) {
    // Use [A]RGB masks if present.
    _bmpMasks[1] = _bmp.info.win.rMask;
    _bmpMasks[2] = _bmp.info.win.gMask;
    _bmpMasks[3] = _bmp.info.win.bMask;

    if (headerSize >= kBmpHeaderSizeWinV3)
      _bmpMasks[0] = _bmp.info.win.aMask;
  }

  // Check whether the [A]RGB masks are consecutive and non-overlapping.
  if (depth > 8 && !bBmpCheckBitMasks(_bmpMasks, 4))
    return setError(DebugUtils::errored(kErrorCodecInvalidData));

  _bufferIndex = (size_t)(p - pBegin);
  return kErrorOk;
}

Error BmpDecoderImpl::decode(Image& dst, const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  if (_bufferIndex == 0)
    B2D_PROPAGATE(setup(p, size));

  if (_frameIndex)
    return DebugUtils::errored(DebugUtils::errored(kErrorCodecNoMoreFrames));

  uint32_t err = kErrorOk;
  const uint8_t* pEnd = p + size;

  // Image info.
  uint32_t w = uint32_t(_imageInfo._size._w);
  uint32_t h = uint32_t(_imageInfo._size._h);

  uint32_t depth = _imageInfo.depth();
  uint32_t pixelFormat = _bmpMasks[0] ? PixelFormat::kPRGB32 : PixelFormat::kXRGB32;
  uint32_t fileAndInfoHeaderSize = 14 + _bmp.info.headerSize;

  if (size < fileAndInfoHeaderSize)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));

  // Palette.
  uint32_t pal[256];
  uint32_t palSize;

  if (depth <= 8) {
    const uint8_t* pPal = p + fileAndInfoHeaderSize;
    palSize = _bmp.file.imageOffset - fileAndInfoHeaderSize;

    uint32_t palEntitySize = _bmp.info.headerSize == kBmpHeaderSizeOs2V1 ? 3 : 4;
    uint32_t palBytesTotal;

    palSize = Math::min<uint32_t>(palSize / palEntitySize, 256);
    palBytesTotal = palSize * palEntitySize;

    if ((size_t)(pEnd - pPal) < palBytesTotal)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    // Stored as BGR|BGR (OS/2) or BGRX|BGRX (Windows).
    uint32_t i = 0;
    while (i < palSize) {
      pal[i++] = ArgbUtil::pack32(0xFF, pPal[2], pPal[1], pPal[0]);
      pPal += palEntitySize;
    }

    // All remaining entries should be fully opaque black.
    while (i < 256) {
      pal[i++] = ArgbUtil::pack32(0xFF, 0, 0, 0);
    }
  }

  // Move the cursor at the beginning of image data and check if the whole
  // image content specified by info.win.imageSize is present in the buffer.
  if (size - _bmp.file.imageOffset < _bmp.info.win.imageSize)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));
  p += _bmp.file.imageOffset;

  // Make sure that the destination image has the correct pixel format and size.
  err = dst.create(int(w), int(h), pixelFormat);
  if (err) return setError(err);

  ImageBuffer pixels;
  err = dst.lock(pixels, this);
  if (err) return setError(err);

  uint8_t* dstLine = pixels.pixelData();
  intptr_t dstStride = pixels.stride();

  // Flip.
  if (_bmp.info.win.height > 0) {
    dstLine += (h - 1) * dstStride;
    dstStride = -dstStride;
  }

  // Decode.
  if (depth == 4 && _bmp.info.win.compression == kBmpCompressionRLE4) {
    err = bBmpDecodeRLE4(dstLine, dstStride, p, _bmp.info.win.imageSize, w, h, pal);
  }
  else if (depth == 8 && _bmp.info.win.compression == kBmpCompressionRLE8) {
    err = bBmpDecodeRLE8(dstLine, dstStride, p, _bmp.info.win.imageSize, w, h, pal);
  }
  else {
    PixelConverter cvt;
    uint32_t layout = depth | PixelConverter::kLayoutLE | PixelConverter::kLayoutIsPremultiplied;

    err = depth <= 8 ? cvt.initImport(pixelFormat, layout | PixelConverter::kLayoutIsIndexed, pal)
                     : cvt.initImport(pixelFormat, layout, _bmpMasks);

    if (err == kErrorOk)
      cvt.convertRect(dstLine, dstStride, p, _bmpStride, w, h);
  }

  dst.unlock(pixels, this);
  if (err)
    return setError(err);

  _frameIndex++;
  return kErrorOk;
}

// ============================================================================
// [b2d::BmpEncoderImpl - Construction / Destruction]
// ============================================================================

BmpEncoderImpl::BmpEncoderImpl() noexcept {}
BmpEncoderImpl::~BmpEncoderImpl() noexcept {}

// ============================================================================
// [b2d::BmpEncoderImpl - Interface]
// ============================================================================

Error BmpEncoderImpl::reset() noexcept {
  Base::reset();
  return kErrorOk;
}

Error BmpEncoderImpl::encode(Buffer& dst, const Image& src) noexcept {
  B2D_PROPAGATE(_error);

  ImageBuffer pixels;

  Error err = src.lock(pixels);
  if (err) return setError(_error);

  uint32_t w = uint32_t(pixels.width());
  uint32_t h = uint32_t(pixels.height());
  uint32_t pixelFormat = pixels.pixelFormat();

  uint32_t headerSize = kBmpHeaderSizeWinV1;
  uint32_t bpl = 0;
  uint32_t depth = 0;
  uint32_t alignment = 0;
  uint32_t masks[4] = { 0 };

  BmpHeader bmp;
  std::memset(&bmp, 0, sizeof(BmpHeader));

  bmp.file.signature[0] = 'B';
  bmp.file.signature[1] = 'M';

  bmp.info.win.width = w;
  bmp.info.win.height = h;
  bmp.info.win.planes = 1;
  bmp.info.win.compression = kBmpCompressionRGB;
  bmp.info.win.colorspace = kBmpColorSpaceDdRGB;

  switch (pixelFormat) {
    case PixelFormat::kPRGB32: {
      headerSize = kBmpHeaderSizeWinV3;
      bpl = w * 4;

      depth = 32;
      masks[0] = 0xFF000000u;
      masks[1] = 0x00FF0000u;
      masks[2] = 0x0000FF00u;
      masks[3] = 0x000000FFu;
      break;
    }

    case PixelFormat::kXRGB32: {
      bpl = w * 3;
      alignment = (4 - (bpl & 3)) & 3;

      depth = 24;
      masks[1] = 0x00FF0000u;
      masks[2] = 0x0000FF00u;
      masks[3] = 0x000000FFu;
      break;
    }

    case PixelFormat::kA8: {
      // TODO:
      break;
    }
  }

  PixelConverter cvt;
  uint32_t imageOffset = 14 + headerSize;
  uint32_t imageSize = (bpl + alignment) * h;
  uint32_t fileSize = imageOffset + imageSize;

  bmp.file.fileSize = fileSize;
  bmp.file.imageOffset = imageOffset;
  bmp.info.win.headerSize = headerSize;
  bmp.info.win.bitsPerPixel = depth;
  bmp.info.win.imageSize = imageSize;
  bmp.info.win.rMask = masks[1];
  bmp.info.win.gMask = masks[2];
  bmp.info.win.bMask = masks[3];
  bmp.info.win.aMask = masks[0];

  if (!B2D_ARCH_LE) {
    bmp.file.byteSwap();
    bmp.info.byteSwapSize();
    bmp.info.win.byteSwapExceptSize();
  }

  // This should never fail as there is only a limited subset of possibilities
  // that are always implemented.
  err = cvt.initExport(pixelFormat, depth | PixelConverter::kLayoutLE | PixelConverter::kLayoutIsPremultiplied, masks);
  B2D_ASSERT(err == kErrorOk);

  uint8_t* dstLine = dst.modify(kContainerOpReplace, fileSize);
  if (dstLine) {
    const uint8_t* src = pixels.pixelData();
    intptr_t stride = pixels.stride();

    std::memcpy(dstLine, &bmp.file, imageOffset);
    cvt.convertRect(dstLine + imageOffset, bpl, src + (intptr_t(h - 1) * stride), -stride, w, h, alignment);
  }
  else {
    err = DebugUtils::errored(DebugUtils::errored(kErrorNoMemory));
  }

  src.unlock(pixels);
  return err ? setError(err) : kErrorOk;
}

// ============================================================================
// [b2d::BmpCodecImpl - Construction / Destruction]
// ============================================================================

BmpCodecImpl::BmpCodecImpl() noexcept {
  _features = ImageCodec::kFeatureDecoder  |
              ImageCodec::kFeatureEncoder  |
              ImageCodec::kFeatureLossless ;

  _formats  = ImageCodec::kFormatIndexed1  |
              ImageCodec::kFormatIndexed2  |
              ImageCodec::kFormatIndexed4  |
              ImageCodec::kFormatIndexed8  |
              ImageCodec::kFormatRGB16     |
              ImageCodec::kFormatRGB24     |
              ImageCodec::kFormatRGB32     |
              ImageCodec::kFormatARGB16    |
              ImageCodec::kFormatARGB32    ;

  setupStrings("B2D\0" "BMP\0" "image/x-bmp\0" "bmp|ras\0");
}
BmpCodecImpl::~BmpCodecImpl() noexcept {}

// ============================================================================
// [b2d::BmpCodecImpl - Interface]
// ============================================================================

int BmpCodecImpl::inspect(const uint8_t* data, size_t size) const noexcept {
  // BMP minimum size and signature (BM).
  if (size < 18 || data[0] != 0x42 || data[1] != 0x4D) return 0;

  // Check if `data` contains correct BMP header size.
  if (!bBmpCheckHeaderSize(Support::readU32uLE(data + 14))) return 0;

  // Don't return 100, maybe there are other codecs with a higher precedence.
  return 95;
}

ImageDecoderImpl* BmpCodecImpl::createDecoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<BmpDecoderImpl>(), ImageDecoder::none().impl()); }
ImageEncoderImpl* BmpCodecImpl::createEncoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<BmpEncoderImpl>(), ImageEncoder::none().impl()); }

// ============================================================================
// [b2d::Bmp - Runtime Handlers]
// ============================================================================

void ImageCodecOnInitBmp(Array<ImageCodec>* codecList) noexcept {
  codecList->append(ImageCodec(AnyInternal::newImplT<BmpCodecImpl>()));
}

B2D_END_NAMESPACE
