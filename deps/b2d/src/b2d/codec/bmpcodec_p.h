// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_BMPCODEC_P_H
#define _B2D_CODEC_BMPCODEC_P_H

// [Dependencies]
#include "../core/imagecodec.h"
#include "../core/pixelconverter.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_codecs
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct BmpHeader;
struct BmpFileHeader;
struct BmpInfoHeader;

struct BmpOS2InfoHeader;
struct BmpWinInfoHeader;

// ============================================================================
// [b2d::Bmp - Constants]
// ============================================================================

//! \internal
enum BmpCompression : uint32_t {
  kBmpCompressionRGB = 0,
  kBmpCompressionRLE8 = 1,
  kBmpCompressionRLE4 = 2,
  kBmpCompressionBitFields = 3,
  kBmpCompressionJPEG = 4,
  kBmpCompressionPNG = 5,
  kBmpCompressionAlphaBitFields = 6,
  kBmpCompressionCMYK = 11,
  kBmpCompressionCMYK_RLE8 = 12,
  kBmpCompressionCMYK_RLE4 = 13
};

//! \internal
enum BmpColorSpace : uint32_t {
  kBmpColorSpaceCalibratedRGB = 0,
  kBmpColorSpaceDdRGB = 1,
  kBmpColorSpaceDdCMYK = 2
};

// ============================================================================
// [b2d::BmpFileHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Bitmap File Header [14 Bytes, LittleEndian].
struct BmpFileHeader {
  // --------------------------------------------------------------------------
  // [ByteSwap]
  // --------------------------------------------------------------------------

  B2D_INLINE void byteSwap() {
    fileSize        = Support::byteswap32(fileSize);
    imageOffset     = Support::byteswap32(imageOffset);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t signature[2];                  //!< Bitmap signature "BM".
  uint32_t fileSize;                     //!< Bitmap file size in bytes.
  uint32_t reserved;                     //!< Reserved, should be zero.
  uint32_t imageOffset;                  //!< Offset to image data (54, 124, ...).
};
#pragma pack(pop)

// ============================================================================
// [b2d::BmpOS2InfoHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Bitmap OS/2 Header [12 Bytes, LittleEndian].
struct BmpOS2InfoHeader {
  // --------------------------------------------------------------------------
  // [ByteSwap]
  // --------------------------------------------------------------------------

  B2D_INLINE void byteSwapSize() noexcept {
    headerSize      = Support::byteswap32(headerSize);
  }

  B2D_INLINE void byteSwapExceptSize() noexcept {
    width           = Support::byteswap16(width);
    height          = Support::byteswap16(height);
    planes          = Support::byteswap16(planes);
    bitsPerPixel    = Support::byteswap16(bitsPerPixel);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t headerSize;                   //!< Header size (40, 52).
  int16_t width;                         //!< Bitmap width (16-bit value).
  int16_t height;                        //!< Bitmap height (16-bit value).
  uint16_t planes;                       //!< Number of color planes (always 1).
  uint16_t bitsPerPixel;                 //!< Bits per pixel (1, 4, 8 or 24).
};
#pragma pack(pop)

// ============================================================================
// [b2d::BmpWinInfoHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Windows Info Header [40 Bytes, LittleEndian].
struct BmpWinInfoHeader {
  // --------------------------------------------------------------------------
  // [ByteSwap]
  // --------------------------------------------------------------------------

  B2D_INLINE void byteSwapSize() noexcept {
    headerSize      = Support::byteswap32(headerSize);
  }

  B2D_INLINE void byteSwapExceptSize() noexcept {
    width           = Support::byteswap32(width);
    height          = Support::byteswap32(height);
    planes          = Support::byteswap16(planes);
    bitsPerPixel    = Support::byteswap16(bitsPerPixel);
    compression     = Support::byteswap32(compression);
    imageSize       = Support::byteswap32(imageSize);
    horzResolution  = Support::byteswap32(horzResolution);
    vertResolution  = Support::byteswap32(vertResolution);
    colorsUsed      = Support::byteswap32(colorsUsed);
    colorsImportant = Support::byteswap32(colorsImportant);

    rMask           = Support::byteswap32(rMask);
    gMask           = Support::byteswap32(gMask);
    bMask           = Support::byteswap32(bMask);

    aMask           = Support::byteswap32(aMask);

    colorspace      = Support::byteswap32(colorspace);
    rX              = Support::byteswap32(rX);
    rY              = Support::byteswap32(rY);
    rZ              = Support::byteswap32(rZ);
    gX              = Support::byteswap32(gX);
    gY              = Support::byteswap32(gY);
    gZ              = Support::byteswap32(gZ);
    bX              = Support::byteswap32(bX);
    bY              = Support::byteswap32(bY);
    bZ              = Support::byteswap32(bZ);
    rGamma          = Support::byteswap32(rGamma);
    gGamma          = Support::byteswap32(gGamma);
    bGamma          = Support::byteswap32(bGamma);

    intent          = Support::byteswap32(intent);
    profileData     = Support::byteswap32(profileData);
    profileSize     = Support::byteswap32(profileSize);
  }

  // --------------------------------------------------------------------------
  // [Members - V1]
  // --------------------------------------------------------------------------

  uint32_t headerSize;                   //!< Header size (40, 52).
  int32_t width;                         //!< Bitmap width.
  int32_t height;                        //!< Bitmap height.
  uint16_t planes;                       //!< Count of planes, always 1.
  uint16_t bitsPerPixel;                 //!< Bits per pixel (1, 4, 8, 16, 24 or 32).
  uint32_t compression;                  //!< Compression methods used, see `BmpCompression`.
  uint32_t imageSize;                    //!< Image data size (in bytes).
  uint32_t horzResolution;               //!< Horizontal resolution in pixels per meter.
  uint32_t vertResolution;               //!< Vertical resolution in pixels per meter.
  uint32_t colorsUsed;                   //!< Number of colors in the image.
  uint32_t colorsImportant;              //!< Minimum number of important colors.

  // --------------------------------------------------------------------------
  // [Members - V2]
  // --------------------------------------------------------------------------

  uint32_t rMask;                        //!< Mask identifying bits of red component.
  uint32_t gMask;                        //!< Mask identifying bits of green component.
  uint32_t bMask;                        //!< Mask identifying bits of blue component.

  // --------------------------------------------------------------------------
  // [Members - V3]
  // --------------------------------------------------------------------------

  uint32_t aMask;                        //!< Mask identifying bits of alpha component.

  // --------------------------------------------------------------------------
  // [Members - V4]
  // --------------------------------------------------------------------------

  uint32_t colorspace;                   //!< Color space type.
  uint32_t rX;                           //!< X coordinate of red endpoint.
  uint32_t rY;                           //!< Y coordinate of red endpoint.
  uint32_t rZ;                           //!< Z coordinate of red endpoint.
  uint32_t gX;                           //!< X coordinate of green endpoint.
  uint32_t gY;                           //!< Y coordinate of green endpoint.
  uint32_t gZ;                           //!< Z coordinate of green endpoint.
  uint32_t bX;                           //!< X coordinate of blue endpoint.
  uint32_t bY;                           //!< Y coordinate of blue endpoint.
  uint32_t bZ;                           //!< Z coordinate of blue endpoint.
  uint32_t rGamma;                       //!< Gamma red coordinate scale value.
  uint32_t gGamma;                       //!< Gamma green coordinate scale value.
  uint32_t bGamma;                       //!< Gamma blue coordinate scale value.

  // --------------------------------------------------------------------------
  // [Members - V5]
  // --------------------------------------------------------------------------

  uint32_t intent;                       //!< Rendering intent for bitmap.
  uint32_t profileData;                  //!< ProfileData offset (in bytes), from the beginning of `BmpWinInfoHeader`.
  uint32_t profileSize;                  //!< Size, in bytes, of embedded profile data.
  uint32_t reserved;                     //!< Reserved, should be zero.
};
#pragma pack(pop)

// ============================================================================
// [b2d::BmpInfoHeader]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! All bitmap headers in one union.
struct BmpInfoHeader {
  // --------------------------------------------------------------------------
  // [ByteSwap]
  // --------------------------------------------------------------------------

  B2D_INLINE void byteSwapSize() noexcept {
    headerSize = Support::byteswap32(headerSize);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint32_t headerSize;
    BmpOS2InfoHeader os2;
    BmpWinInfoHeader win;
  };
};
#pragma pack(pop)

// ============================================================================
// [b2d::BmpHeader]
// ============================================================================

//! Bitmap header - `BmpFileHeader` + `BmpInfoHeader`.
struct BmpHeader {
  //! Unused bytes used as padding.
  //!
  //! Bitmap file header starts with "BM" signature followed by data generally
  //! aligned to 32 bits. On systems where unaligned reads are forbidden this
  //! can cause program termination. So to fix this problem we shift all data
  //! by two bytes to prevent adding specific functions to read/write to/from
  //! unaligned memory.
  uint8_t unused[2];

  //! Bitmap file header.
  BmpFileHeader file;
  //! Bitmap info header.
  BmpInfoHeader info;
};

// ============================================================================
// [b2d::BmpDecoderImpl]
// ============================================================================

class BmpDecoderImpl final : public ImageDecoderImpl {
public:
  typedef ImageDecoderImpl Base;

  BmpDecoderImpl() noexcept;
  virtual ~BmpDecoderImpl() noexcept;

  Error reset() noexcept override;
  Error setup(const uint8_t* p, size_t size) noexcept override;
  Error decode(Image& dst, const uint8_t* p, size_t size) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  BmpHeader _bmp;
  uint32_t _bmpMasks[4];
  uint32_t _bmpStride;
};

// ============================================================================
// [b2d::BmpEncoderImpl]
// ============================================================================

class BmpEncoderImpl final : public ImageEncoderImpl {
public:
  typedef ImageEncoderImpl Base;

  BmpEncoderImpl() noexcept;
  virtual ~BmpEncoderImpl() noexcept;

  Error reset() noexcept override;
  Error encode(Buffer& dst, const Image& src) noexcept override;
};

// ============================================================================
// [b2d::BmpCodecImpl]
// ============================================================================

class BmpCodecImpl final : public ImageCodecImpl {
public:
  BmpCodecImpl() noexcept;
  virtual ~BmpCodecImpl() noexcept;

  int inspect(const uint8_t* data, size_t size) const noexcept override;
  ImageDecoderImpl* createDecoderImpl() const noexcept override;
  ImageEncoderImpl* createEncoderImpl() const noexcept override;
};

// ============================================================================
// [b2d::Bmp - Runtime Handlers]
// ============================================================================

#if defined(B2D_EXPORTS)
void ImageCodecOnInitBmp(Array<ImageCodec>* codecList) noexcept;
#endif // B2D_EXPORTS

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_BMPCODEC_P_H
