// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_JPEGCODEC_P_H
#define _B2D_CODEC_JPEGCODEC_P_H

// [Dependencies]
#include "../core/imagecodec.h"
#include "../core/pixelconverter.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_codecs
//! \{

//! \internal
//! \{

// ============================================================================
// [b2d::JpegEntropyBits]
// ============================================================================

//! Data type used to store entropy bits during decoding selected to be 32-bit
//! or 64-bit depending on the host architecture. The type cannot be less than
//! 32-bits as it would break `JpegDecoderStream` and  many other assumptions).
typedef uintptr_t JpegEntropyBits;

// ============================================================================
// [b2d::JpegConstants]
// ============================================================================

//! JPEG constants.
enum JpegConstants : uint32_t {
  //! JPEG file signature is 2 bytes (0xFF, 0xD8) followed by markers, SOF
  //! (start of file) marker contains 1 byte signature and at least 8 bytes of
  //! data describing basic information of the image.
  kJpegMinSize = 11,

  //! JPEG's block size (width and height), always 8x8.
  //!
  //! This constant has been introduced to make it clear in the code.
  kJpegBlockSize = 8,

  //! JPEG's block total.
  kJpegBlockSizeSq = kJpegBlockSize * kJpegBlockSize,

  //! Maximum bits of `JpegEntropyBits` type.
  kJpegEntropyBitsSize = uint32_t(sizeof(JpegEntropyBits) * 8),

  //! Maximum bytes of a baseline and progressive 8x8 block, sum of:
  //!  - 4 bytes per value of 64 values.
  //!  - 4 or 8 bytes to refill the data.
  //!  - all multiplied by 2 to avoid overflow caused by unescaping `0xFF`.
  kJpegMaxBlockSizeInBytes = (4 * 64 + uint32_t(sizeof(JpegEntropyBits))) * 2
};

template<typename T>
struct alignas(16) JpegBlock {
  B2D_INLINE void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  T data[kJpegBlockSizeSq];
};

// ============================================================================
// [b2d::JpegMarker]
// ============================================================================

//! JPEG markers.
enum JpegMarker : uint32_t {
  kJpegMarkerNULL         = 0x00u,       //!< A "stuff byte" used in Huffman stream to encode 0xFF, which is otherwise used as a marker.
  kJpegMarkerTEM          = 0x01u,       //!< Temporary use in arithmetic coding.
  kJpegMarkerRES          = 0x02u,       //!< Reserved (first) (0x02..0xBF).
  kJpegMarkerRES_Last     = 0xBFu,       //!< Reserved (last).

  kJpegMarkerSOF0         = 0xC0u,       //!< Start of Frame 0 - Baseline DCT (Huffman).
  kJpegMarkerSOF1         = 0xC1u,       //!< Start of Frame 1 - Sequential DCT (Huffman).
  kJpegMarkerSOF2         = 0xC2u,       //!< Start of Frame 2 - Progressive DCT (Huffman).
  kJpegMarkerSOF3         = 0xC3u,       //!< Start of Frame 3 - Lossless (Huffman).
  kJpegMarkerDHT          = 0xC4u,       //!< Define Huffman Table (0xC4).
  kJpegMarkerSOF5         = 0xC5u,       //!< Start of Frame 5 - Differential Sequential DCT (Huffman).
  kJpegMarkerSOF6         = 0xC6u,       //!< Start of Frame 6 - Differential Progressive DCT (Huffman).
  kJpegMarkerSOF7         = 0xC7u,       //!< Start of Frame 7 - Differential Lossless (Huffman).
  kJpegMarkerJPG          = 0xC8u,       //!< JPEG Extensions (0xC8).
  kJpegMarkerSOF9         = 0xC9u,       //!< Start of Frame 9 - Sequential DCT (Arithmetic).
  kJpegMarkerSOF10        = 0xCAu,       //!< Start of Frame 10 - Progressive DCT (Arithmetic).
  kJpegMarkerSOF11        = 0xCBu,       //!< Start of Frame 11 - Lossless (Arithmetic).
  kJpegMarkerDAC          = 0xCCu,       //!< Define Arithmetic Coding (0xCC).
  kJpegMarkerSOF13        = 0xCDu,       //!< Start of Frame 13 - Differential Sequential DCT (Arithmetic).
  kJpegMarkerSOF14        = 0xCEu,       //!< Start of Frame 14 - Differential Progressive DCT (Arithmetic).
  kJpegMarkerSOF15        = 0xCFu,       //!< Start of Frame 15 - Differential Lossless (Arithmetic).

  kJpegMarkerRST          = 0xD0u,       //!< Restart Marker (first) (0xD0..0xD7)
  kJpegMarkerRST_Last     = 0xD7u,       //!< Restart Marker (last)
  kJpegMarkerSOI          = 0xD8u,       //!< Start of Image (0xD8).
  kJpegMarkerEOI          = 0xD9u,       //!< End of Image (0xD9).
  kJpegMarkerSOS          = 0xDAu,       //!< Start of Scan (0xDA).
  kJpegMarkerDQT          = 0xDBu,       //!< Define Quantization Table (0xDB).
  kJpegMarkerDNL          = 0xDCu,       //!< Define Number of Lines (0xDC).
  kJpegMarkerDRI          = 0xDDu,       //!< Define Restart Interval (0xDD).
  kJpegMarkerDHP          = 0xDEu,       //!< Define Hierarchical Progression (0xDE)
  kJpegMarkerEXP          = 0xDFu,       //!< Expand Reference Component (0xDF).

  kJpegMarkerAPP          = 0xE0u,       //!< Application (first) (0xE0..0xEF).
  kJpegMarkerAPP_Last     = 0xEFu,       //!< Application (last).

  kJpegMarkerAPP0         = 0xE0u,       //!< APP0 - JFIF, JFXX (0xE0).
  kJpegMarkerAPP1         = 0xE1u,       //!< APP1 - EXIF, XMP (0xE1).
  kJpegMarkerAPP2         = 0xE2u,       //!< APP2 - ICC (0xE2).
  kJpegMarkerAPP3         = 0xE3u,       //!< APP3 - META (0xE3).
  kJpegMarkerAPP4         = 0xE4u,       //!< APP4 (0xE4).
  kJpegMarkerAPP5         = 0xE5u,       //!< APP5 (0xE5).
  kJpegMarkerAPP6         = 0xE6u,       //!< APP6 (0xE6).
  kJpegMarkerAPP7         = 0xE7u,       //!< APP7 (0xE7).
  kJpegMarkerAPP8         = 0xE8u,       //!< APP8 (0xE8).
  kJpegMarkerAPP9         = 0xE9u,       //!< APP9 (0xE9).
  kJpegMarkerAPP10        = 0xEAu,       //!< APP10 (0xEA).
  kJpegMarkerAPP11        = 0xEBu,       //!< APP11 (0xEB).
  kJpegMarkerAPP12        = 0xECu,       //!< APP12 - Picture information (0xEC).
  kJpegMarkerAPP13        = 0xEDu,       //!< APP13 - Adobe IRB (0xED).
  kJpegMarkerAPP14        = 0xEEu,       //!< APP14 - Adobe (0xEE).
  kJpegMarkerAPP15        = 0xEFu,       //!< APP15 (0xEF).

  kJpegMarkerEXT          = 0xF0u,       //!< JPEG Extension (first) (0xF0..0xFD).
  kJpegMarkerEXT_Last     = 0xFDu,       //!< JPEG Extension (last).
  kJpegMarkerCOM          = 0xFEu,       //!< Comment (0xFE).

  kJpegMarkerNone         = 0xFFu        //!< Invalid (0xFF), sometimes used as padding.
};

// ============================================================================
// [b2d::JpegColorspace]
// ============================================================================

//! JPEG's colorspace
enum JpegColorspace : uint32_t {
  kJpegColorspaceNone     = 0,           //!< Colorspace not defined.
  kJpegColorspaceY        = 1,           //!< Colorspace is Y-only (grayscale).
  kJpegColorspaceYCbCr    = 2,           //!< Colorspace is YCbCr.
  kJpegColorspaceCount    = 3            //!< Count of JPEG's colorspace types.
};

// ============================================================================
// [b2d::JpegDensityUnit]
// ============================================================================

//! JPEG's density unit specified by APP0-JFIF marker.
enum JpegDensityUnit : uint32_t {
  kJpegDensityOnlyAspect  = 0,
  kJpegDensityPixelsPerIn = 1,
  kJpegDensityPixelsPerCm = 2,
  kJpegDensityCount       = 3
};

// ============================================================================
// [b2d::JpegThumbnailFormat]
// ============================================================================

//! JPEG's thumbnail format specified by APP0-JFXX marker.
enum JpegThumbnailFormat : uint32_t {
  kJpegThumbnailJpeg      = 0,
  kJpegThumbnailPal8      = 1,
  kJpegThumbnailRgb24     = 2,
  kJpegThumbnailCount     = 3
};

// ============================================================================
// [b2d::JpegSamplingPoint]
// ============================================================================

//! JPEG's sampling point as specified by JFIF-APP0 marker.
enum JpegSamplingPoint : uint32_t {
  kJpegSamplingUnknown    = 0,           //!< Unknown / not specified sampling point (no JFIF-APP0 marker).
  kJpegSamplingCosited    = 1,           //!< Co-sitted sampling point (specified by JFIF-APP0 marker).
  kJpegSamplingCentered   = 2            //!< Centered sampling point (specified by JFIF-APP0 marker).
};

// ============================================================================
// [b2d::JpegTableClass]
// ============================================================================

//! JPEG's table class selector (DC, AC).
enum JpegTableClass : uint32_t {
  kJpegTableDC            = 0,           //!< DC table.
  kJpegTableAC            = 1,           //!< AC table.
  kJpegTableCount         = 2            //!< Number of tables.
};

// ============================================================================
// [b2d::JpegDecoderFlags]
// ============================================================================

//! JPEG decoder flags - bits of information collected from JPEG markers.
enum JpegDecoderFlags : uint32_t {
  kJpegDecoderDoneSOI     = 0x00000001u,
  kJpegDecoderDoneEOI     = 0x00000002u,
  kJpegDecoderDoneJFIF    = 0x00000004u,
  kJpegDecoderDoneJFXX    = 0x00000008u,
  kJpegDecoderDoneExif    = 0x00000010u,
  kJpegDecoderHasThumbnail = 0x80000000u
};

// ============================================================================
// [b2d::JpegAllocatorBase]
// ============================================================================

//! JPEG allocator (non-template base class).
struct JpegAllocatorBase {
  B2D_NONCOPYABLE(JpegAllocatorBase)

  B2D_INLINE JpegAllocatorBase(uint32_t n)
    : _count(0),
      _max(n) {
    std::memset(_blocks, 0, n * sizeof(void*));
  }

  B2D_INLINE ~JpegAllocatorBase() {
    _freeAll();
  }

  // --------------------------------------------------------------------------
  // [Alloc / FreeAll]
  // --------------------------------------------------------------------------

  void* _alloc(size_t size, uint32_t alignment = 1);
  void _freeAll();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _count;
  uint32_t _max;
  void* _blocks[1];
};

// ============================================================================
// [b2d::JpegAllocatorTmp]
// ============================================================================

//! JPEG allocator.
//!
//! Keeps track of allocated memory that can be released by `freeAll()` in one
//! go. The template parameter `N` specifies maximum number of blocks that can
//! be allocated. Exceeding `N` will result in returning null from `alloc()`.
template<size_t N>
struct JpegAllocatorTmp : public JpegAllocatorBase {
  B2D_NONCOPYABLE(JpegAllocatorTmp)

  B2D_INLINE JpegAllocatorTmp() : JpegAllocatorBase(N) {}
  B2D_INLINE ~JpegAllocatorTmp() {}

  // --------------------------------------------------------------------------
  // [Alloc / Free]
  // --------------------------------------------------------------------------

  B2D_INLINE void* alloc(size_t size, uint32_t alignment = 1) {
    return _alloc(size, alignment);
  }

  B2D_INLINE void freeAll() {
    _freeAll();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  void* _blocksData[N - 1];
};

// ============================================================================
// [b2d::JpegUtils]
// ============================================================================

//! JPEG utility functions.
struct JpegUtils {
  //! Get whether the marker `m` is a SOF marker.
  static B2D_INLINE bool isSOF(uint32_t m) noexcept { return m >= kJpegMarkerSOF0 && m <= kJpegMarkerSOF2; }
  //! Get whether the marker `m` is an RST marker.
  static B2D_INLINE bool isRST(uint32_t m) noexcept { return m >= kJpegMarkerRST && m <= kJpegMarkerRST_Last; }
  //! Get whether the marker `m` is an APP marker.
  static B2D_INLINE bool isAPP(uint32_t m) noexcept { return m >= kJpegMarkerAPP && m <= kJpegMarkerAPP_Last; }
};

// ============================================================================
// [b2d::JpegDecoderHuff]
// ============================================================================

//! JPEG Huffman decompression table.
struct JpegDecoderHuff {
  enum : uint32_t {
    kFastBits = 9,
    kFastSize = 1 << kFastBits,
    kFastMask = kFastSize - 1
  };

  uint8_t fast[kFastSize];
  int32_t delta[17];
  uint32_t maxcode[18];
  uint16_t code[256];
  uint8_t size[257];
  uint8_t values[256];
};

// ============================================================================
// [b2d::JpegDecoderStream]
// ============================================================================

//! JPEG decoder's entropy stream.
//!
//! NOTE: The main reason there are no methods and C macros are used instead
//! to decode the bits from the stream is to make all variables local and not
//! synced with the `JpegDecoderStream`. Eliminating these extra reads/writes
//! improves the performance of decoding a bit.
struct JpegDecoderStream {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(nullptr, nullptr); }

  B2D_INLINE void reset(const uint8_t* ptr_, const uint8_t* end_) noexcept {
    ptr = ptr_;
    end = end_;

    data = 0;
    size = 0;
    eobRun = 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const uint8_t* ptr;                    //!< Data pointer (points to the byte to be processed).
  const uint8_t* end;                    //!< End of input (points to the first invalid byte).

  JpegEntropyBits data;                  //!< Entropy-coded buffer.
  size_t size;                           //!< Number of valid bits in `data`.

  uint32_t eobRun;                       //!< TODO:
  uint32_t restartCounter;               //!< Restart counter in the current stream (reset by DRI and RST markers).
};

struct JpegEntropyReader {
  B2D_INLINE explicit JpegEntropyReader(JpegDecoderStream& stream) noexcept
    : _bitData(stream.data),
      _bitCount(stream.size),
      _ptr(stream.ptr),
      _end(stream.end) {}

  B2D_INLINE void done(JpegDecoderStream& stream) noexcept {
    stream.data = _bitData;
    stream.size = _bitCount;
    stream.ptr = _ptr;
    stream.end = _end;
  }

  // --------------------------------------------------------------------------
  // [Reader]
  // --------------------------------------------------------------------------

  B2D_INLINE bool atEnd() const noexcept { return _ptr == _end; }
  B2D_INLINE bool hasBits(size_t nBits) const noexcept { return _bitCount >= nBits; }

  B2D_INLINE Error requireBits(size_t nBits) const noexcept {
    if (B2D_UNLIKELY(!hasBits(nBits)))
      return DebugUtils::errored(kErrorCodecInvalidHuffman);
    return kErrorOk;
  }

  B2D_INLINE void flush() noexcept {
    _bitData = 0;
    _bitCount = 0;
  }

  B2D_INLINE void advance(size_t nBytes) noexcept {
    B2D_ASSERT((size_t)(_end - _ptr) >= nBytes);
    _ptr += nBytes;
  }

  B2D_INLINE void drop(size_t nBits) noexcept {
    _bitData <<= nBits;
    _bitCount -= nBits;
  }

  template<typename T = JpegEntropyBits>
  B2D_INLINE T peek(size_t nBits) const noexcept {
    typedef typename std::make_unsigned<T>::type U;
    return T(U(_bitData >> (kJpegEntropyBitsSize - nBits)));
  }

  B2D_INLINE void refill() noexcept {
    while (_bitCount <= kJpegEntropyBitsSize - 8 && _ptr != _end) {
      uint32_t tmpByte = *_ptr++;

      // The [0xFF] byte has to be escaped by [0xFF, 0x00], so read two bytes.
      if (tmpByte == 0xFF) {
        if (B2D_UNLIKELY(_ptr == _end))
          break;

        uint32_t tmpMarker = *_ptr++;
        if (B2D_UNLIKELY(tmpMarker != 0x00)) {
          _ptr -= 2;
          _end = _ptr;
          break;
        }
      }

      _bitData += JpegEntropyBits(tmpByte) << ((kJpegEntropyBitsSize - 8) - _bitCount);
      _bitCount += 8;
    }
  }

  B2D_INLINE void refillIf32Bit() noexcept {
    if (kJpegEntropyBitsSize < 32)
      return refill();
  }

  // Read a single bit (0 or 1).
  template<typename T = JpegEntropyBits>
  B2D_INLINE T readBit() noexcept {
    B2D_ASSERT(_bitCount >= 1);

    T result = peek<T>(1);
    drop(1);
    return result;
  }

  // Read `n` bits and sign extend.
  B2D_INLINE int32_t readSigned(size_t nBits) noexcept {
    B2D_ASSERT(_bitCount >= nBits);

    int32_t tmpMask = Support::shl(int32_t(-1), nBits) + 1;
    int32_t tmpSign = -peek<int32_t>(1);

    int32_t result = peek<int32_t>(nBits) + (tmpMask & ~tmpSign);
    drop(nBits);
    return result;
  }

  // Read `n` bits and zero extend.
  B2D_INLINE uint32_t readUnsigned(size_t nBits) noexcept {
    B2D_ASSERT(_bitCount >= nBits);

    uint32_t result = peek<uint32_t>(nBits);
    drop(nBits);
    return result;
  }

  template<typename T>
  B2D_INLINE Error readCode(T& dst, const JpegDecoderHuff* table) noexcept {
    uint32_t code = table->fast[peek(JpegDecoderHuff::kFastBits)];
    uint32_t codeSize;

    if (code < 255) {
      // FAST: Look at the top bits and determine the symbol id.
      codeSize = table->size[code];
      if (B2D_UNLIKELY(codeSize > _bitCount))
        return DebugUtils::errored(kErrorCodecInvalidHuffman);
    }
    else {
      // SLOW: Naive test is to shift the `_bitData` down so `s` bits are valid,
      // then test against `maxcode`. To speed this up, we've preshifted maxcode
      // left so that it has `16-s` 0s at the end; in other words, regardless of
      // the number of bits, it wants to be compared against something shifted to
      // have 16; that way we don't need to shift inside the loop.
      code = peek<uint32_t>(16);
      codeSize = JpegDecoderHuff::kFastBits + 1;

      while (code >= table->maxcode[codeSize])
        codeSize++;

      // Maximum code size is 16, check for 17/_bitCount and fail if reached.
      if (B2D_UNLIKELY(codeSize == 17 || codeSize > _bitCount))
        return DebugUtils::errored(kErrorCodecInvalidHuffman);

      // Convert the huffman code to the symbol ID.
      code = peek<uint32_t>(codeSize) + uint32_t(table->delta[codeSize]);
    }

    // Convert the symbol ID to the resulting BYTE.
    dst = T(table->values[code]);
    drop(codeSize);
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  JpegEntropyBits _bitData;
  size_t _bitCount;

  const uint8_t* _ptr;
  const uint8_t* _end;
};

// ============================================================================
// [b2d::JpegDecoderInfo]
// ============================================================================

struct JpegDecoderInfo {
  uint8_t sofMarker;                     //!< SOF marker (so we can select right decompression algorithm), initially set to zero.
  uint8_t colorspace;                    //!< Colorspace, see `JpegColorspace`.
  uint8_t delayedHeight;                 //!< True if the data contains zero height (delayed height).
  uint8_t reserved;                      //!< Reserved.

  uint32_t flags;                        //!< JPEG decoder flags, see `JpegDecoderFlags`.

  uint8_t mcuSfW;                        //!< MCU width in blocks (maximum horizontal sampling factor of all components).
  uint8_t mcuSfH;                        //!< MCU height in blocks (maximum veftical sampling factor of all components).

  uint8_t mcuPxW;                        //!< MCU width in pixels (horizontal resolution of a single MCU).
  uint8_t mcuPxH;                        //!< MCU height in pixels (vertical resolution of a single MCU).

  uint32_t mcuCountW;                    //!< Number of MCUs in horizontal direction.
  uint32_t mcuCountH;                    //!< Number of MCUs in vertical direction.

  uint8_t jfifMajor;                     //!< JFIF major version (if present).
  uint8_t jfifMinor;                     //!< JFIF minor version (if present).

  uint8_t dcTableMask;                   //!< Mask of all defined DC tables.
  uint8_t acTableMask;                   //!< Mask of all defined AC tables.

  uint8_t quantTableMask;                //!< Mask of all defined (de)quantization tables.
  uint8_t reserved_2;                    //!< Reserved

  uint32_t restartInterval;              //!< Restart interval as specified by DRI marker.
};

// ============================================================================
// [b2d::JpegDecoderComponent]
// ============================================================================

struct JpegDecoderComponent {
  uint8_t* data;                         //!< Raster data.

  uint8_t compId;                        //!< Component ID.
  uint8_t quantId;                       //!< Quantization table ID.
  uint8_t dcId;                          //!< DC Huffman-Table ID.
  uint8_t acId;                          //!< AC Huffman-Table ID.

  uint32_t pxW;                          //!< Effective width.
  uint32_t pxH;                          //!< Effective height.
  uint32_t osW;                          //!< Oversized width to match the total width requires by all MCUs.
  uint32_t osH;                          //!< Oversized height to match the total height required by all MCUs.
  uint32_t blW;                          //!< Number of 8x8 blocks in horizontal direction.
  uint32_t blH;                          //!< Number of 8x8 blocks in vertical direction.

  uint8_t sfW;                           //!< Horizontal sampling factor (width).
  uint8_t sfH;                           //!< Vertical sampling factor (height).

  int32_t dcPred;                        //!< DC prediction (modified during decoding phase).
  int16_t* coeff;                        //!< Coefficients used only by progressive JPEGs.
};

// ============================================================================
// [b2d::JpegDecoderSOS]
// ============================================================================

//! Start of stream (SOS) data.
struct JpegDecoderSOS {
  JpegDecoderComponent* scComp[4];       //!< Maps a stream component index into the `JpegDecoderComponent`.
  uint8_t scCount;                       //!< Count of components in this stream.
  uint8_t ssStart;                       //!< Start of spectral selection.
  uint8_t ssEnd;                         //!< End of spectral selection.
  uint8_t saLowBit;                      //!< Successive approximation low bit.
  uint8_t saHighBit;                     //!< Successive approximation high bit.
};

// ============================================================================
// [b2d::JpegDecoderThumbnail]
// ============================================================================

//! In case of RGB24 or PAL8 thumbnail data, the index points to the first
//! byte describing W, H, and then data follows. In case of an embedded JPEG
//! the `index` points to the first byte of that JPEG. So to decode the RAW
//! uncompressed PAL8/RGB24 data, skip first two bytes, that describe W/H,
//! which is already filled in this struct.
struct JpegDecoderThumbnail {
  uint8_t format;                        //!< Thumbnail format, see `JpegThumbnailFormat`.
  uint8_t reserved;                      //!< Reserved
  uint8_t w, h;                          //!< Thumbnail width and height (8-bit, as in JFIF spec.).
  size_t index;                          //!< Index of the thumbnail data from the beginning of the stream.
  size_t size;                           //!< Thumbnail data size (raw data, the JFIF headers not included here).
};

// ============================================================================
// [b2d::JpegDecoderImpl]
// ============================================================================

class JpegDecoderImpl final : public ImageDecoderImpl {
public:
  typedef ImageDecoderImpl Base;

  JpegDecoderImpl() noexcept;
  virtual ~JpegDecoderImpl() noexcept;

  Error reset() noexcept override;
  Error setup(const uint8_t* p, size_t size) noexcept override;
  Error decode(Image& dst, const uint8_t* p, size_t size) noexcept override;

  // --------------------------------------------------------------------------
  // [Process Marker]
  // --------------------------------------------------------------------------

  //! Process a marker `m`.
  Error processMarker(uint32_t m, const uint8_t* p, size_t remain, size_t& consumedBytes) noexcept;

  // --------------------------------------------------------------------------
  // [Process Stream]
  // --------------------------------------------------------------------------

  //! Process an entropy stream (called after SOS marker).
  Error processStream(const uint8_t* p, size_t remain, size_t& consumedBytes) noexcept;

  // --------------------------------------------------------------------------
  // [Process MCUs]
  // --------------------------------------------------------------------------

  //! Process all MCUs, called after all components have been decoded.
  Error processMCUs() noexcept;

  // --------------------------------------------------------------------------
  // [ConvertToRGB]
  // --------------------------------------------------------------------------

  Error convertToRgb(ImageBuffer& dst) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  JpegDecoderInfo _jpeg;                 //!< JPEG decoder info and states (defined by SOF and all following markers).
  JpegDecoderSOS _sos;                   //!< JPEG decoder's current stream data (defined and overwritten by SOS markers).
  JpegDecoderThumbnail _thumb;           //!< JPEG decoder thumbnail data.
  JpegDecoderComponent _comp[4];         //!< JPEG decoder components.
  JpegAllocatorTmp<16> _mem;             //!< JPEG memory manager (can allocate aligned blocks and keep track of them).
  JpegDecoderHuff _huffDC[4];            //!< Huffman DC tables.
  JpegDecoderHuff _huffAC[4];            //!< Huffman AC tables.
  int16_t _fastAC[4][JpegDecoderHuff::kFastSize]; //! Huffman AC fast case.
  JpegBlock<uint16_t> _qTable[4];        //!< Quantization/Dequantization tables.
};

// ============================================================================
// [b2d::JpegEncoderImpl]
// ============================================================================

class JpegEncoderImpl final : public ImageEncoderImpl {
public:
  typedef ImageEncoderImpl Base;

  JpegEncoderImpl() noexcept;
  virtual ~JpegEncoderImpl() noexcept;

  Error reset() noexcept override;
  Error encode(Buffer& dst, const Image& src) noexcept override;
};

// ============================================================================
// [b2d::JpegCodecImpl]
// ============================================================================

class JpegCodecImpl final : public ImageCodecImpl {
public:
  JpegCodecImpl() noexcept;
  virtual ~JpegCodecImpl() noexcept;

  int inspect(const uint8_t* data, size_t size) const noexcept override;
  ImageDecoderImpl* createDecoderImpl() const noexcept override;
  ImageEncoderImpl* createEncoderImpl() const noexcept override;
};

// ============================================================================
// [b2d::Jpeg - Runtime Handlers]
// ============================================================================

#if defined(B2D_EXPORTS)
void ImageCodecOnInitJpeg(Array<ImageCodec>* codecList) noexcept;
#endif // B2D_EXPORTS

//! \}
//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_JPEGCODEC_P_H
