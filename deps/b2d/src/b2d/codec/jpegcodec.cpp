// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// The JPEG codec is based on stb_image <https://github.com/nothings/stb>
// released into PUBLIC DOMAIN. Blend2D's JPEG codec can be distributed
// under Blend2D's ZLIB license or under STB's PUBLIC DOMAIN as well.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/jpegcodec_p.h"
#include "../codec/jpegutils_p.h"
#include "../core/allocator.h"
#include "../core/math.h"
#include "../core/membuffer.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Jpeg - Constants]
// ============================================================================

// Mapping table of zigzagged 8x8 data into a natural order.
static const uint8_t gJpegDeZigZagTable[64 + 16] = {
  0 , 1 , 8 , 16, 9 , 2 , 3 , 10,
  17, 24, 32, 25, 18, 11, 4 , 5 ,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13, 6 , 7 , 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63,

  // These are not part of JPEG's spec, however, it's convenient as the decoder
  // doesn't have to check whether the coefficient index is out of bounds.
  63, 63, 63, 63, 63, 63, 63, 63,
  63, 63, 63, 63, 63, 63, 63, 63
};

// TIFF Header used by EXIF (LE or BE).
static const uint8_t gJpegExifLE[4] = { 0x49, 0x49, 0x2A, 0x00 };
static const uint8_t gJpegExifBE[4] = { 0x4D, 0x4D, 0x00, 0x2A };

// ============================================================================
// [b2d::JpegAllocatorBase]
// ============================================================================

void* JpegAllocatorBase::_alloc(size_t size, uint32_t alignment) {
  uint32_t count = _count;
  if (count >= _max)
    return nullptr;

  void* p = Allocator::systemAlloc(size + alignment - 1);
  if (p == nullptr)
    return nullptr;

  _blocks[count] = p;
  _count = count + 1;

  return Support::alignUp(p, alignment);
}

void JpegAllocatorBase::_freeAll() {
  uint32_t i = 0;
  uint32_t count = _count;

  _count = 0;
  do {
    if (_blocks[i])
      Allocator::systemRelease(_blocks[i]);
    _blocks[i] = nullptr;
  } while (++i < count);
}

// ============================================================================
// [b2d::JpegDecoderHuff]
// ============================================================================

static Error JpegDecoder_buildTable(JpegDecoderHuff* table, const uint8_t* count) noexcept {
  std::memset(table->fast, 255, JpegDecoderHuff::kFastSize);

  table->delta[0] = 0;
  table->maxcode[0] = 0;            // Not used.
  table->maxcode[17] = 0xFFFFFFFFu; // Sentinel.

  // Build size list for each symbol.
  uint32_t i = 0;
  uint32_t k = 0;
  do {
    uint32_t c = count[i++];
    for (uint32_t j = 0; j < c; j++)
      table->size[k++] = uint8_t(i);
  } while (i < 16);
  table->size[k] = 0;

  // Compute actual symbols.
  {
    uint32_t code = 0;
    for (i = 1, k = 0; i <= 16; i++, code <<= 1) {
      // Compute delta to add to code to compute symbol id.
      table->delta[i] = int32_t(k) - int32_t(code);

      if (table->size[k] == i) {
        while (table->size[k] == i)
          table->code[k++] = uint16_t(code++);

        if (code - 1 >= (1u << i))
          return DebugUtils::errored(kErrorCodecInvalidData);
      }

      // Compute largest code + 1 for this size, preshifted as needed later.
      table->maxcode[i] = code << (16u - i);
    }
  }

  // Build non-spec acceleration table; 255 is flag for not-accelerated.
  for (i = 0; i < k; i++) {
    uint32_t s = table->size[i];

    if (s <= JpegDecoderHuff::kFastBits) {
      s = JpegDecoderHuff::kFastBits - s;

      uint32_t code = uint32_t(table->code[i]) << s;
      uint32_t cMax = code + (1u << s);

      while (code < cMax) {
        table->fast[code++] = uint8_t(i);
      }
    }
  }

  return kErrorOk;
}

// Build a table that decodes both magnitude and value of small ACs in one go.
static void JpegDecoder_buildFastAC(const JpegDecoderHuff* table, int16_t* fastAC) noexcept {
  for (uint32_t i = 0; i < JpegDecoderHuff::kFastSize; i++) {
    uint32_t fast = table->fast[i];
    int32_t ac = 0;

    if (fast < 255) {
      uint32_t val = table->values[fast];
      uint32_t size = table->size[fast];

      uint32_t run = val >> 4;
      uint32_t mag = val & 15;

      if (mag != 0 && size + mag <= JpegDecoderHuff::kFastBits) {
        // Magnitude code followed by receive/extend code.
        int32_t k = ((i << size) & JpegDecoderHuff::kFastMask) >> (JpegDecoderHuff::kFastBits - mag);
        int32_t m = Support::shl(1, mag - 1);

        if (k < m)
          k += Support::shl(-1, mag) + 1;

        // If the result is small enough, we can fit it in fastAC table.
        if (k >= -128 && k <= 127) {
          ac = int32_t(Support::shl(k, 8)) +
               int32_t(Support::shl(run, 4)) + size + mag;
        }
      }
    }

    fastAC[i] = int16_t(ac);
  }
}

// ============================================================================
// [b2d::JpegDecoderImpl - Construction / Destruction]
// ============================================================================

JpegDecoderImpl::JpegDecoderImpl() noexcept { reset(); }
JpegDecoderImpl::~JpegDecoderImpl() noexcept {}

// ============================================================================
// [b2d::JpegDecoderImpl - Interface]
// ============================================================================

Error JpegDecoderImpl::reset() noexcept {
  Base::reset();

  _mem.freeAll();
  std::memset(&_jpeg , 0, sizeof(_jpeg));
  std::memset(&_sos  , 0, sizeof(_sos));
  std::memset(&_thumb, 0, sizeof(_thumb));
  std::memset(&_comp , 0, sizeof(_comp));

  return kErrorOk;
}

Error JpegDecoderImpl::setup(const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  if (_bufferIndex != 0)
    return setError(DebugUtils::errored(kErrorInvalidState));

  if (size < kJpegMinSize)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));

  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + size;

  // Check JPEG signature (SOI marker).
  if (p[0] != 0xFF || p[1] != kJpegMarkerSOI)
    return setError(DebugUtils::errored(kErrorCodecInvalidSignature));

  p += 2;
  _jpeg.flags |= kJpegDecoderDoneSOI;

  // --------------------------------------------------------------------------
  // [Process Markers - Until SOF]
  // --------------------------------------------------------------------------

  for (;;) {
    size_t remain = (size_t)(pEnd - p);
    _bufferIndex = (size_t)(p - pBegin);

    if (remain < 2)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    if (p[0] != 0xFF)
      return setError(DebugUtils::errored(kErrorCodecInvalidData));

    uint32_t m = p[1];
    p += 2;
    remain -= 2;

    // Some files have an extra padding (0xFF) after their blocks, ignore it.
    if (m == kJpegMarkerNone) {
      while (p != pEnd && (m = p[0]) == kJpegMarkerNone)
        p++;

      if (p == pEnd)
        break;

      // Update `p` and `remain` so `p` points at the beginning of the segment
      // and `remain` contains the number of remaining bytes without the marker.
      remain = (size_t)(pEnd - ++p);
    }

    size_t consumedBytes = 0;
    Error err = processMarker(m, p, (size_t)(pEnd - p), consumedBytes);

    if (err != kErrorOk)
      return setError(err);

    p += consumedBytes;
    B2D_ASSERT(consumedBytes <= remain);

    // Terminate after SOF has been processed, the rest is handled by `decode()`.
    if (JpegUtils::isSOF(m))
      break;
  }

  _bufferIndex = (size_t)(p - pBegin);
  return kErrorOk;
}

Error JpegDecoderImpl::decode(Image& dst, const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  // Setup should process all margers until SOF, which is processed here.
  if (_bufferIndex == 0)
    B2D_PROPAGATE(setup(p, size));

  if (_frameIndex)
    return DebugUtils::errored(kErrorCodecNoMoreFrames);

  uint32_t err = kErrorOk;
  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + size;

  if (size < _bufferIndex)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));
  p += _bufferIndex;

  // --------------------------------------------------------------------------
  // [Process Markers - After SOF]
  // --------------------------------------------------------------------------

  for (;;) {
    size_t remain = (size_t)(pEnd - p);
    _bufferIndex = (size_t)(p - pBegin);

    if (remain < 2)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    if (p[0] != 0xFF)
      return setError(DebugUtils::errored(kErrorCodecInvalidData));

    uint32_t m = p[1];
    p += 2;
    remain -= 2;

    // Some files have an extra padding (0xFF) after their blocks, ignore it.
    if (m == kJpegMarkerNone) {
      while (p != pEnd && (m = p[0]) == kJpegMarkerNone)
        p++;

      if (p == pEnd)
        break;

      // Update `p` and `remain` so `p` points at the beginning of the segment
      // and `remain` contains the number of remaining bytes without the marker.
      remain = (size_t)(pEnd - ++p);
    }

    // Process the marker.
    {
      size_t consumedBytes = 0;
      err = processMarker(m, p, (size_t)(pEnd - p), consumedBytes);

      if (err != kErrorOk)
        return setError(err);

      p += consumedBytes;
      B2D_ASSERT(consumedBytes <= remain);
    }

    // EOI - terminate.
    if (m == kJpegMarkerEOI)
      break;

    // SOS - process the entropy coded data-stream that follows SOS.
    if (m == kJpegMarkerSOS) {
      size_t consumedBytes = 0;
      err = processStream(p, (size_t)(pEnd - p), consumedBytes);

      if (err != kErrorOk)
        return setError(err);

      p += consumedBytes;
      B2D_ASSERT(consumedBytes <= remain);
    }
  }

  // --------------------------------------------------------------------------
  // [Process MCUs]
  // --------------------------------------------------------------------------

  err = processMCUs();
  if (err != kErrorOk)
    return setError(err);

  // --------------------------------------------------------------------------
  // [YCbCr -> RGB]
  // --------------------------------------------------------------------------

  // Image info.
  uint32_t w = uint32_t(_imageInfo._size._w);
  uint32_t h = uint32_t(_imageInfo._size._h);
  uint32_t pixelFormat = PixelFormat::kXRGB32;

  // Make sure that the destination image has the correct pixel format and size.
  err = dst.create(int(w), int(h), pixelFormat);
  if (err) return setError(err);

  ImageBuffer pixels;

  err = dst.lock(pixels, this);
  if (err) return setError(err);

  err = convertToRgb(pixels);

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

  dst.unlock(pixels, this);
  if (err)
    return setError(err);

  _frameIndex++;
  return kErrorOk;
}

// ============================================================================
// [b2d::JpegDecoderImpl - Process Marker]
// ============================================================================

Error JpegDecoderImpl::processMarker(uint32_t m, const uint8_t* p, size_t remain, size_t& consumedBytes) noexcept {
  // Should be zero when passed in.
  B2D_ASSERT(consumedBytes == 0);

#define GET_PAYLOAD_SIZE(MinSize) \
  size_t size; \
  \
  do { \
    if (remain < MinSize) \
      return DebugUtils::errored(kErrorCodecIncompleteData); \
    \
    size = Support::readU16uBE(p); \
    if (size < MinSize) \
      return DebugUtils::errored(kErrorCodecInvalidData); \
    \
    if (size > remain) \
      return DebugUtils::errored(kErrorCodecIncompleteData); \
    \
    p += 2; \
    remain = size - 2; \
  } while (false)

  // --------------------------------------------------------------------------
  // [SOF - Start of Frame]
  //
  //        WORD - Size (Consumed by GET_PAYLOAD_...)
  //
  //   [00] BYTE - Precision `P`
  //   [01] WORD - Height `Y`
  //   [03] WORD - Width `X`
  //   [05] BYTE - Number of components `Nf`
  //
  //   [06] Specification of each component [0..Nf] {
  //        [00] BYTE Component identifier `id`
  //        [01] BYTE Horizontal `Hi` and vertical `Vi` sampling factor
  //        [02] BYTE Quantization table destination selector `TQi`
  //   }
  // --------------------------------------------------------------------------
  if (JpegUtils::isSOF(m)) {
    uint32_t sofMarker = m;

    // Forbid multiple SOF markers in a single JPEG file.
    if (_jpeg.sofMarker)
      return DebugUtils::errored(kErrorJpegDuplicatedSOFMarker);

    // Check if SOF type is supported.
    if (sofMarker != kJpegMarkerSOF0 &&
        sofMarker != kJpegMarkerSOF1 &&
        sofMarker != kJpegMarkerSOF2)
      return DebugUtils::errored(kErrorJpegUnsupportedSOFMarker);

    // 11 is a minimum number of bytes for SOF, describing only one component.
    GET_PAYLOAD_SIZE(2 + 6 + 3);

    uint32_t bpp = p[0];
    uint32_t h = Support::readU16uBE(p + 1);
    uint32_t w = Support::readU16uBE(p + 3);
    uint32_t numComponents = p[5];

    if (size != 8 + 3 * numComponents)
      return DebugUtils::errored(kErrorJpegInvalidSOFLength);

    // Skip HEADER bytes.
    p += 6;

    // TODO: Unsupported delayed height (0).
    if (w == 0)
      return DebugUtils::errored(kErrorCodecInvalidSize);

    if (h == 0)
      return DebugUtils::errored(kErrorJpegUnsupportedFeature);

    if (w > Image::kMaxSize || h > Image::kMaxSize)
      return DebugUtils::errored(kErrorCodecUnsupportedSize);

    // Check number of components and SOF size.
    if ((numComponents != 1 && numComponents != 3))
      return DebugUtils::errored(kErrorCodecInvalidFormat);

    // TODO: 16-BPC.
    if (bpp != 8)
      return DebugUtils::errored(kErrorCodecInvalidFormat);

    // Maximum horizontal/vertical sampling factor of all components.
    uint32_t mcuSfW = 1;
    uint32_t mcuSfH = 1;

    uint32_t i, j;
    for (i = 0; i < numComponents; i++, p += 3) {
      JpegDecoderComponent* comp = &_comp[i];

      // Check if the ID doesn't collide with previous components.
      uint32_t compId = p[0];
      for (j = 0; j < i; j++) {
        if (_comp[j].compId == compId)
          return DebugUtils::errored(kErrorCodecInvalidData);
      }

      // TODO: Is this necessary?
      // Required by JFIF.
      if (compId != i + 1) {
        // Some version of JpegTran outputs non-JFIF-compliant files!
        if (compId != i)
          return DebugUtils::errored(kErrorCodecInvalidData);
      }

      // Horizontal/Vertical sampling factor.
      uint32_t sf = p[1];
      uint32_t sfW = sf >> 4;
      uint32_t sfH = sf & 15;

      if (sfW == 0 || sfW > 4 || sfH == 0 || sfH > 4)
        return DebugUtils::errored(kErrorCodecInvalidData);

      // Quantization ID.
      uint32_t quantId = p[2];
      if (quantId > 3)
        return DebugUtils::errored(kErrorCodecInvalidData);

      // Save to JpegDecoderComponent.
      // printf("COMPONENT %u %ux%u quantId=%u\n", compId, sfW, sfH, quantId);
      comp->compId  = uint8_t(compId);
      comp->sfW     = uint8_t(sfW);
      comp->sfH     = uint8_t(sfH);
      comp->quantId = uint8_t(quantId);

      // We need to know maximum horizontal and vertical sampling factor to
      // calculate the correct MCU size (WxH).
      mcuSfW = Math::max(mcuSfW, sfW);
      mcuSfH = Math::max(mcuSfH, sfH);
    }

    // Compute interleaved MCU info.
    uint32_t mcuPxW = mcuSfW * kJpegBlockSize;
    uint32_t mcuPxH = mcuSfH * kJpegBlockSize;

    uint32_t mcuCountW = (w + mcuPxW - 1) / mcuPxW;
    uint32_t mcuCountH = (h + mcuPxH - 1) / mcuPxH;
    bool isBaseline = sofMarker != kJpegMarkerSOF2;

    for (i = 0; i < numComponents; i++) {
      JpegDecoderComponent* comp = &_comp[i];

      // Number of effective pixels (e.g. for non-interleaved MCU).
      comp->pxW = (w * uint32_t(comp->sfW) + mcuSfW - 1) / mcuSfW;
      comp->pxH = (h * uint32_t(comp->sfH) + mcuSfH - 1) / mcuSfH;

      // Allocate enough memory for all blocks even those that won't be used fully.
      comp->blW = mcuCountW * uint32_t(comp->sfW);
      comp->blH = mcuCountH * uint32_t(comp->sfH);

      comp->osW = comp->blW * kJpegBlockSize;
      comp->osH = comp->blH * kJpegBlockSize;

      comp->data = static_cast<uint8_t*>(_mem.alloc(comp->osW * comp->osH));
      if (comp->data == nullptr)
        return DebugUtils::errored(kErrorNoMemory);

      if (!isBaseline) {
        uint32_t kBlock8x8UInt16 = kJpegBlockSize * kJpegBlockSize * uint32_t(sizeof(int16_t));

        size_t coeffSize = comp->blW * comp->blH * kBlock8x8UInt16;
        int16_t* coeffData = static_cast<int16_t*>(_mem.alloc(coeffSize, 16));

        if (coeffData == nullptr)
          return DebugUtils::errored(kErrorNoMemory);

        comp->coeff = coeffData;
        std::memset(comp->coeff, 0, coeffSize);
      }
    }

    // Everything seems ok, store the image information.
    _imageInfo.setSize(int(w), int(h));
    _imageInfo.setDepth(numComponents * bpp);
    _imageInfo.setNumPlanes(numComponents);
    _imageInfo.setProgressive(!isBaseline);
    _imageInfo.setNumFrames(1);

    _jpeg.sofMarker = sofMarker;
    _jpeg.delayedHeight = (h == 0);
    _jpeg.mcuSfW = uint8_t(mcuSfW);
    _jpeg.mcuSfH = uint8_t(mcuSfH);
    _jpeg.mcuPxW = uint8_t(mcuPxW);
    _jpeg.mcuPxH = uint8_t(mcuPxH);
    _jpeg.mcuCountW = mcuCountW;
    _jpeg.mcuCountH = mcuCountH;

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [DHT - Define Huffman Table]
  //
  //        WORD - Size (Consumed by GET_PAYLOAD_...)
  //
  //   [00] BYTE - Table class `tc` and table identifier `ti`.
  //   [01] 16xB - The count of Huffman codes of size 1..16.
  //
  //   [17] .... - The one byte symbols sorted by Huffman code. The number of
  //               symbols is the sum of the 16 code counts.
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerDHT) {
    GET_PAYLOAD_SIZE(2 + 17);

    while (remain >= 17) {
      uint32_t q = *p++;

      uint32_t tableClass = q >> 4; // Table class.
      uint32_t tableId = q & 15; // Table id (0-3).

      // Invalid class or id.
      if (tableClass >= kJpegTableCount || tableId > 3)
        return DebugUtils::errored(kErrorCodecInvalidData);

      uint32_t i;
      uint32_t n = 0;

      for (i = 0; i < 16; i++)
        n += uint32_t(p[i]);

      if (remain < n + 17)
        return DebugUtils::errored(kErrorCodecInvalidData);

      JpegDecoderHuff* huff = (tableClass == kJpegTableDC) ? &_huffDC[tableId] : &_huffAC[tableId];
      B2D_PROPAGATE(JpegDecoder_buildTable(huff, p));
      std::memcpy(huff->values, p + 16, n);

      if (tableClass == kJpegTableDC) {
        _jpeg.dcTableMask = uint8_t(_jpeg.dcTableMask | Support::bitMask<uint32_t>(tableId));
      }
      else {
        _jpeg.acTableMask = uint8_t(_jpeg.acTableMask | Support::bitMask<uint32_t>(tableId));
        JpegDecoder_buildFastAC(&_huffAC[tableId], _fastAC[tableId]);
      }

      p += n + 16;
      remain -= n + 17;
    }

    if (remain != 0)
      return DebugUtils::errored(kErrorCodecInvalidData);

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [DQT - Define Quantization Table]
  //
  //        WORD - Size (Consumed by GET_PAYLOAD_...)
  //
  //   [00] BYTE - Quantization value size `quantSz` (0-1) and table identifier
  //               `quantId`.
  //   [01] .... - 64 or 128 bytes depending on `qs`.
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerDQT) {
    GET_PAYLOAD_SIZE(2 + 65);

    while (remain >= 65) {
      uint32_t q = *p++;

      uint32_t qSize = q >> 4;
      uint32_t qId = q & 15;

      if (qSize > 1 || qId > 3)
        return DebugUtils::errored(kErrorCodecInvalidData);

      uint16_t* qTable = _qTable[qId].data;
      uint32_t requiredSize = 1 + 64 * (qSize + 1);

      if (requiredSize > remain)
        break;

      if (qSize == 0) {
        for (uint32_t k = 0; k < 64; k++, p++)
          qTable[gJpegDeZigZagTable[k]] = *p;
      }
      else {
        for (uint32_t k = 0; k < 64; k++, p += 2)
          qTable[gJpegDeZigZagTable[k]] = Support::readU16uBE(reinterpret_cast<const uint16_t*>(p));
      }

      _jpeg.quantTableMask = uint8_t(_jpeg.quantTableMask | Support::bitMask<uint8_t>(qId));
      remain -= requiredSize;
    }

    if (remain != 0)
      return DebugUtils::errored(kErrorCodecInvalidData);

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [DRI - Define Restart Interval]
  //
  //        WORD - Size (Consumed by GET_PAYLOAD_...)
  //
  //   [00] WORD - Restart interval.
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerDRI) {
    if (remain < 4)
      return DebugUtils::errored(kErrorCodecIncompleteData);

    size_t size = Support::readU16uBE(p + 0);
    uint32_t ri = Support::readU16uBE(p + 2);

    // DRI payload should be 4 bytes.
    if (size != 4)
      return DebugUtils::errored(kErrorCodecInvalidData);

    _jpeg.restartInterval = ri;

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [SOS - Start of Scan]
  //
  //        WORD - Size (Consumed by GET_PAYLOAD_...)
  //
  //   [00] BYTE - Number of components in this SOS:
  //
  //   [01] Specification of each component - {
  //        [00] BYTE - Component ID
  //        [01] BYTE - DC and AC Selector
  //   }
  //
  //   [01 + NumComponents * 2]:
  //        [00] BYTE - Spectral Selection Start
  //        [01] BYTE - Spectral Selection End
  //        [02] BYTE - Successive Approximation High/Low
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerSOS) {
    GET_PAYLOAD_SIZE(2 + 6);

    uint32_t sofMarker = _jpeg.sofMarker;
    uint32_t numComponents = _imageInfo.numPlanes();

    uint32_t scCount = *p++;
    uint32_t scMask = 0;

    if (size != 6 + scCount * 2)
      return DebugUtils::errored(kErrorCodecInvalidFormat);

    if (scCount < 1 || scCount > numComponents)
      return DebugUtils::errored(kErrorJpegInvalidSOSComponentCount);

    uint32_t ssStart   = uint32_t(p[scCount * 2 + 0]);
    uint32_t ssEnd     = uint32_t(p[scCount * 2 + 1]);
    uint32_t saLowBit  = uint32_t(p[scCount * 2 + 2]) & 15;
    uint32_t saHighBit = uint32_t(p[scCount * 2 + 2]) >> 4;

    if (sofMarker == kJpegMarkerSOF0 || sofMarker == kJpegMarkerSOF1) {
      if (ssStart != 0 || saLowBit != 0 || saHighBit != 0)
        return DebugUtils::errored(kErrorCodecInvalidData);

      // The value should be 63, but it's zero sometimes.
      ssEnd = 63;
    }

    if (sofMarker == kJpegMarkerSOF2) {
      if (ssStart > 63 || ssEnd > 63 || ssStart > ssEnd || saLowBit > 13 || saHighBit > 13)
        return DebugUtils::errored(kErrorCodecInvalidData);

      // AC & DC cannot be merged in a progressive JPEG.
      if (ssStart == 0 && ssEnd != 0)
        return DebugUtils::errored(kErrorCodecInvalidData);
    }

    _sos.scCount   = scCount;
    _sos.ssStart   = uint8_t(ssStart);
    _sos.ssEnd     = uint8_t(ssEnd);
    _sos.saLowBit  = uint8_t(saLowBit);
    _sos.saHighBit = uint8_t(saHighBit);

    for (uint32_t i = 0; i < scCount; i++, p += 2) {
      uint32_t compId = p[0];
      uint32_t index = 0;

      while (_comp[index].compId != compId)
        if (++index >= numComponents)
          return DebugUtils::errored(kErrorJpegInvalidSOSComponent);

      // One huffman stream shouldn't overwrite the same component.
      if (Support::bitTest(scMask, index))
        return DebugUtils::errored(kErrorJpegDuplicatedSOSComponent);

      scMask |= Support::bitMask<uint32_t>(index);

      uint32_t selector = p[1];
      uint32_t acId = selector & 15;
      uint32_t dcId = selector >> 4;

      // Validate AC & DC selectors.
      if (acId > 3 || (!Support::bitTest(_jpeg.acTableMask, acId) && ssEnd  > 0))
        return DebugUtils::errored(kErrorJpegInvalidACSelector);

      if (dcId > 3 || (!Support::bitTest(_jpeg.dcTableMask, dcId) && ssEnd == 0))
        return DebugUtils::errored(kErrorJpegInvalidDCSelector);

      // Link the current component to the `index` and update AC & DC selectors.
      JpegDecoderComponent* comp = &_comp[index];
      comp->dcId = uint8_t(dcId);
      comp->acId = uint8_t(acId);
      _sos.scComp[i] = comp;
    }

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [APP - Application]
  // --------------------------------------------------------------------------
  if (JpegUtils::isAPP(m)) {
    GET_PAYLOAD_SIZE(2);

    // ------------------------------------------------------------------------
    // [APP0 - "JFIF\0"]
    // ------------------------------------------------------------------------
    if (m == kJpegMarkerAPP0 && remain >= 5 && std::memcmp(p, "JFIF", 5) == 0) {
      if (_jpeg.flags & kJpegDecoderDoneJFIF)
        return DebugUtils::errored(kErrorJFIFAlreadyDefined);

      if (remain < 14)
        return DebugUtils::errored(kErrorJFIFInvalidSize);

      uint32_t jfifMajor = p[5];
      uint32_t jfifMinor = p[6];

      // Check the density unit, correct it to aspect-only if it's wrong, but
      // don't fail as of one wrong value won't make any difference anyway.
      uint32_t densityUnit = p[7];
      uint32_t xDensity = Support::readU16uBE(p + 8);
      uint32_t yDensity = Support::readU16uBE(p + 10);

      switch (densityUnit) {
        case kJpegDensityOnlyAspect:
          // TODO:
          break;

        case kJpegDensityPixelsPerIn:
          _imageInfo.setDensity(
            double(int(xDensity)) * 39.3701,
            double(int(yDensity)) * 39.3701);
          break;

        case kJpegDensityPixelsPerCm:
          _imageInfo.setDensity(
            double(int(xDensity * 100)),
            double(int(yDensity * 100)));
          break;

        default:
          densityUnit = kJpegDensityOnlyAspect;
          break;
      }

      uint32_t thumbW = p[12];
      uint32_t thumbH = p[13];

      _jpeg.flags |= kJpegDecoderDoneJFIF;
      _jpeg.jfifMajor = jfifMajor;
      _jpeg.jfifMinor = jfifMinor;

      if (thumbW && thumbH) {
        uint32_t thumbSize = thumbW * thumbH * 3;

        if (thumbSize + 14 < remain)
          return DebugUtils::errored(kErrorJFIFInvalidSize);

        _thumb.format = kJpegThumbnailRgb24;
        _thumb.w = uint8_t(thumbW);
        _thumb.h = uint8_t(thumbH);
        _thumb.index = _bufferIndex + 18;
        _thumb.size = thumbSize;

        _jpeg.flags |= kJpegDecoderHasThumbnail;
      }
    }

    // ------------------------------------------------------------------------
    // [APP0 - "JFXX\0"]
    // ------------------------------------------------------------------------
    if (m == kJpegMarkerAPP0 && remain >= 5 && std::memcmp(p, "JFXX", 5) == 0) {
      if (_jpeg.flags & kJpegDecoderDoneJFXX)
        return DebugUtils::errored(kErrorJFXXAlreadyDefined);

      if (remain < 6)
        return DebugUtils::errored(kErrorJFXXInvalidSize);

      uint32_t format = p[5];
      uint32_t thumbW = 0;
      uint32_t thumbH = 0;
      uint32_t thumbSize = 0;

      switch (format) {
        case kJpegThumbnailJpeg:
          // Cannot overflow as the payload size is just 16-bit uint.
          thumbSize = uint32_t(remain - 6);
          break;

        case kJpegThumbnailPal8:
          thumbW = p[6];
          thumbH = p[7];
          thumbSize = 768 + thumbW * thumbH;
          break;

        case kJpegThumbnailRgb24:
          thumbW = p[6];
          thumbH = p[7];
          thumbSize = thumbW * thumbH * 3;
          break;

        default:
          return DebugUtils::errored(kErrorJFXXInvalidThumbnailFormat);
      }

      if (thumbSize + 6 > remain)
        return DebugUtils::errored(kErrorJFXXInvalidSize);

      _thumb.format = format;
      _thumb.w = uint8_t(thumbW);
      _thumb.h = uint8_t(thumbH);
      _thumb.index = _bufferIndex + 10;
      _thumb.size = thumbSize;

      _jpeg.flags |= kJpegDecoderDoneJFXX |
                     kJpegDecoderHasThumbnail;
    }

    // ------------------------------------------------------------------------
    // [APP1 - "EXIF\0\0"]
    // ------------------------------------------------------------------------
    /*
    // TODO: This would require some work to make this possible.
    if (m == kJpegMarkerAPP1 && remain >= 6 && std::memcmp(p, "Exif\0", 6) == 0) {
      // These should be only one EXIF marker in the whole JPEG image, not sure
      // what to do if there is more...
      if (!(_jpeg.flags & kJpegDecoderDoneExif)) {
        p += 6;
        remain -= 6;

        // Need at least more 8 bytes required by TIFF header.
        if (remain < 8)
          return DebugUtils::errored(kErrorExifInvalidHeader);

        // Check if the EXIF marker has a proper TIFF header.
        uint32_t byteOrder;
        uint32_t doByteSwap;

        if (std::memcmp(p, gJpegExifLE, 4))
          byteOrder = Globals::kByteOrderLE;
        else if (std::memcmp(p, gJpegExifBE, 4))
          byteOrder = Globals::kByteOrderBE;
        else
          return DebugUtils::errored(kErrorExifInvalidHeader);

        doByteSwap = byteOrder != Globals::kByteOrderNative;
        _jpeg.flags |= kJpegDecoderDoneExif;
      }
    }
    */

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [COM - Comment]
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerCOM) {
    GET_PAYLOAD_SIZE(2);

    consumedBytes = size;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [EOI - End of Image]
  // --------------------------------------------------------------------------
  if (m == kJpegMarkerEOI) {
    _jpeg.flags |= kJpegDecoderDoneEOI;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Invalid / Unknown]
  // --------------------------------------------------------------------------

  return DebugUtils::errored(kErrorCodecInvalidData);

#undef GET_PAYLOAD_SIZE
}

// ============================================================================
// [b2d::JpegDecoderImpl - Process Stream]
// ============================================================================

//! \internal
struct JpegDecoderRun {
  //! Component linked with the run.
  JpegDecoderComponent* comp;

  //! Current data pointer (advanced during decoding).
  uint8_t* data;
  //! Dequantization table pointer.
  const JpegBlock<uint16_t>* qTable;

  //! Count of 8x8 blocks required by a single MCU, calculated as `sfW * sfH`.
  uint32_t count;
  //! Stride.
  uint32_t stride;
  //! Horizontal/Vertical advance per MCU.
  uint32_t advance[2];

  //! Offsets of all blocks of this component that are part of a single MCU.
  intptr_t offset[16];
};

// Called after a restart marker (RES) has been reached.
static B2D_INLINE Error JpegDecoder_handleRestart(JpegDecoderImpl* self, JpegDecoderStream& stream, const uint8_t* pEnd) noexcept {
  if (stream.restartCounter == 0 || --stream.restartCounter != 0)
    return kErrorOk;

  JpegDecoderInfo& jpeg = self->_jpeg;
  JpegEntropyReader reader(stream);

  // I think this shouldn't be necessary to refill the code buffer/size as all
  // bytes should have been consumed. However, since the spec is so vague, I'm
  // not sure if this is necessary, recommended, or forbidden :(
  reader.refill();

  if (!reader.atEnd() || (size_t)(pEnd - reader._ptr) < 2 || !JpegUtils::isRST(reader._ptr[1]))
    return DebugUtils::errored(kErrorCodecInvalidHuffman);

  // Skip the marker and flush entropy bits.
  reader.flush();
  reader.advance(2);
  reader.done(stream);

  stream.eobRun = 0;
  stream.restartCounter = jpeg.restartInterval;

  JpegDecoderComponent* comp = self->_comp;
  comp[0].dcPred = 0;
  comp[1].dcPred = 0;
  comp[2].dcPred = 0;
  comp[3].dcPred = 0;
  return kErrorOk;
}

//! \internal
//!
//! Decode a baseline 8x8 block.
static Error JpegDecoder_readBaselineBlock(JpegDecoderImpl* self, JpegDecoderStream& stream, JpegDecoderComponent* comp, int16_t* dst) noexcept {
  const JpegDecoderHuff* dcTable = &self->_huffDC[comp->dcId];
  const JpegDecoderHuff* acTable = &self->_huffAC[comp->acId];

  JpegEntropyReader reader(stream);
  reader.refill();

  // --------------------------------------------------------------------------
  // [Decode DC - Maximum Bytes Consumed: 4 (unescaped)]
  // --------------------------------------------------------------------------

  uint32_t s;
  int32_t dcPred = comp->dcPred;
  B2D_PROPAGATE(reader.readCode(s, dcTable));

  if (s) {
    reader.refillIf32Bit();
    B2D_PROPAGATE(reader.requireBits(s));

    int32_t dcVal = reader.readSigned(s);
    dcPred += dcVal;
    comp->dcPred = dcPred;
  }
  dst[0] = int16_t(dcPred);

  // --------------------------------------------------------------------------
  // [Decode AC - Maximum Bytes Consumed: 4 * 63 (unescaped)]
  // --------------------------------------------------------------------------

  uint32_t k = 1;
  const int16_t* acFast = self->_fastAC[comp->acId];

  do {
    reader.refill();

    uint32_t c = reader.peek<uint32_t>(JpegDecoderHuff::kFastBits);
    int32_t ac = acFast[c];

    // Fast AC.
    if (ac) {
      s = (ac & 15);       // Size.
      k += (ac >> 4) & 15; // Skip.
      ac >>= 8;
      reader.drop(s);
      dst[gJpegDeZigZagTable[k++]] = int16_t(ac);
    }
    else {
      B2D_PROPAGATE(reader.readCode(ac, acTable));
      s = ac & 15;
      ac >>= 4;

      if (s == 0) {
        // End block.
        if (ac != 0xF)
          break;
        k += 16;
      }
      else {
        k += ac;

        reader.refillIf32Bit();
        B2D_PROPAGATE(reader.requireBits(s));

        ac = reader.readSigned(s);
        dst[gJpegDeZigZagTable[k++]] = int16_t(ac);
      }
    }
  } while (k < 64);

  reader.done(stream);
  return kErrorOk;
}

//! \internal
//!
//! Decode a progressive 8x8 block (AC or DC coefficients, but never both).
static Error JpegDecoder_readProgressiveBlock(JpegDecoderImpl* self, JpegDecoderStream& stream, JpegDecoderComponent* comp, int16_t* dst) noexcept {
  JpegEntropyReader reader(stream);
  reader.refill();

  uint32_t k     = uint32_t(self->_sos.ssStart);
  uint32_t kEnd  = uint32_t(self->_sos.ssEnd) + 1;
  uint32_t shift = self->_sos.saLowBit;

  // --------------------------------------------------------------------------
  // [Decode DC - Maximum Bytes Consumed: 4 (unescaped)]
  // --------------------------------------------------------------------------

  if (k == 0) {
    const JpegDecoderHuff* dcTable = &self->_huffDC[comp->dcId];
    uint32_t s;

    if (self->_sos.saHighBit == 0) {
      // Initial scan for DC coefficient.
      int32_t dcPred = comp->dcPred;
      B2D_PROPAGATE(reader.readCode(s, dcTable));

      if (s) {
        reader.refillIf32Bit();
        B2D_PROPAGATE(reader.requireBits(s));

        int32_t dcVal = reader.readSigned(s);
        dcPred += dcVal;
        comp->dcPred = dcPred;
      }

      dst[0] = int16_t(Support::shl(dcPred, shift));
    }
    else {
      // Refinement scan for DC coefficient.
      B2D_PROPAGATE(reader.requireBits(1));

      s = reader.readBit<uint32_t>();
      dst[0] += int16_t(s << shift);
    }

    k++;
  }

  // --------------------------------------------------------------------------
  // [Decode AC - Maximum Bytes Consumed: max(4 * 63, 8) (unescaped)]
  // --------------------------------------------------------------------------

  if (k < kEnd) {
    const JpegDecoderHuff* acTable = &self->_huffAC[comp->acId];
    const int16_t* acFast = self->_fastAC[comp->acId];

    if (self->_sos.saHighBit == 0) {
      // Initial scan for AC coefficients.
      if (stream.eobRun) {
        stream.eobRun--;
        return kErrorOk;
      }

      do {
        // Fast AC.
        reader.refill();
        int32_t r = acFast[reader.peek(JpegDecoderHuff::kFastBits)];

        if (r) {
          int32_t s = r & 15;
          k += (r >> 4) & 15;
          reader.drop(s);

          uint32_t zig = gJpegDeZigZagTable[k++];
          dst[zig] = int16_t(Support::shl(r >> 8, shift));
        }
        else {
          B2D_PROPAGATE(reader.readCode(r, acTable));
          reader.refillIf32Bit();

          int32_t s = r & 15;
          r >>= 4;

          if (s == 0) {
            if (r < 15) {
              uint32_t eobRun = 0;
              if (r) {
                B2D_PROPAGATE(reader.requireBits(r));
                eobRun = reader.readUnsigned(r);
              }
              stream.eobRun = eobRun + (1u << r) - 1;
              break;
            }
            k += 16;
          }
          else {
            k += r;
            r = reader.readSigned(s);

            uint32_t zig = gJpegDeZigZagTable[k++];
            dst[zig] = int16_t(Support::shl(r, shift));
          }
        }
      } while (k < kEnd);
    }
    else {
      // Refinement scan for AC coefficients.
      int32_t bit = int32_t(1) << shift;
      if (stream.eobRun) {
        do {
          int16_t* p = &dst[gJpegDeZigZagTable[k++]];
          int32_t pVal = *p;

          if (pVal) {
            B2D_PROPAGATE(reader.requireBits(1));
            uint32_t b = reader.readBit<uint32_t>();

            reader.refill();
            if (b && (pVal & bit) == 0)
              *p = int16_t(pVal + (pVal > 0 ? bit : -bit));
          }
        } while (k < kEnd);
        stream.eobRun--;
      }
      else {
        do {
          int32_t r, s;

          reader.refill();
          B2D_PROPAGATE(reader.readCode(r, acTable));

          reader.refillIf32Bit();
          s = r & 15;
          r >>= 4;

          if (s == 0) {
            if (r < 15) {
              uint32_t eobRun = 0;
              if (r) {
                B2D_PROPAGATE(reader.requireBits(r));
                eobRun = reader.readUnsigned(r);
              }
              stream.eobRun = eobRun + (1u << r) - 1;
              r = 64; // Force end of block.
            }
            // r=15 s=0 already does the right thing (write 16 0s).
          }
          else {
            if (B2D_UNLIKELY(s != 1))
              return DebugUtils::errored(kErrorCodecInvalidHuffman);

            B2D_PROPAGATE(reader.requireBits(1));
            uint32_t sign = reader.readBit<uint32_t>();
            s = sign ? bit : -bit;
          }

          // Advance by `r`.
          while (k < kEnd) {
            int16_t* p = &dst[gJpegDeZigZagTable[k++]];
            int32_t pVal = *p;

            if (pVal) {
              uint32_t b;

              reader.refill();
              B2D_PROPAGATE(reader.requireBits(1));

              b = reader.readBit<uint32_t>();
              if (b && (pVal & bit) == 0)
                *p = int16_t(pVal + (pVal > 0 ? bit : -bit));
            }
            else {
              if (r == 0) {
                *p = int16_t(s);
                break;
              }
              r--;
            }
          }
        } while (k < kEnd);
      }
    }
  }

  reader.done(stream);
  return kErrorOk;
}

Error JpegDecoderImpl::processStream(const uint8_t* p, size_t remain, size_t& consumedBytes) noexcept {
  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + remain;

  // --------------------------------------------------------------------------
  // [Initialize]
  // --------------------------------------------------------------------------

  // Just needed to determine the logic.
  uint32_t sofMarker = _jpeg.sofMarker;

  // Whether the stream is baseline or progressive. Progressive streams use
  // multiple SOS markers to progressively update the image being decoded.
  bool isBaseline = sofMarker != kJpegMarkerSOF2;

  // If this is a basline stream then the unit-size is 1 byte, because the
  // block of coefficients is immediately IDCTed to pixel values after it is
  // decoded. However, progressive decoding cannot use this space optimization
  // as coefficients are updated progressively.
  uint32_t unitSize = isBaseline ? 1 : 2;

  // Initialize the entropy stream.
  JpegDecoderStream stream;
  stream.reset(p, pEnd);
  stream.restartCounter = _jpeg.restartInterval;

  uint32_t i;
  uint32_t scCount = _sos.scCount;

  uint32_t mcuX = 0;
  uint32_t mcuY = 0;

  // TODO: This is not right, we must calculate MCU W/H every time.
  uint32_t mcuW = _jpeg.mcuCountW;
  uint32_t mcuH = _jpeg.mcuCountH;

  // A single component's decoding doesn't use interleaved MCUs.
  if (scCount == 1) {
    JpegDecoderComponent* comp = _sos.scComp[0];
    mcuW = (comp->pxW + kJpegBlockSize - 1) / kJpegBlockSize;
    mcuH = (comp->pxH + kJpegBlockSize - 1) / kJpegBlockSize;
  }

  // printf("SOS %s MCU{%ux%u} scCount=%u\n", isBaseline ? "BASELINE" : "PROGRESSIVE", mcuW, mcuH, scCount);

  // Initialize decoder runs (each run specifies one component per scan).
  JpegDecoderRun runs[4];
  for (i = 0; i < scCount; i++) {
    JpegDecoderRun* run = &runs[i];
    JpegDecoderComponent* comp = _sos.scComp[i];

    uint32_t sfW = scCount > 1 ? uint32_t(comp->sfW) : uint32_t(1);
    uint32_t sfH = scCount > 1 ? uint32_t(comp->sfH) : uint32_t(1);

    uint32_t count = 0;
    uint32_t offset = 0;

    if (isBaseline) {
      uint32_t stride = comp->osW * unitSize;

      for (uint32_t y = 0; y < sfH; y++) {
        for (uint32_t x = 0; x < sfW; x++) {
          run->offset[count++] = offset + x * unitSize * kJpegBlockSize;
        }
        offset += stride * kJpegBlockSize;
      }

      run->comp = comp;
      run->data = comp->data;
      run->qTable = &_qTable[comp->quantId];

      run->count = count;
      run->stride = stride;
      run->advance[0] = sfW * unitSize * kJpegBlockSize;
      run->advance[1] = run->advance[0] + (sfH * kJpegBlockSize - 1) * stride;
    }
    else {
      uint32_t blockSize = unitSize * kJpegBlockSizeSq;
      uint32_t blockStride = comp->blW * blockSize;

      for (uint32_t y = 0; y < sfH; y++) {
        for (uint32_t x = 0; x < sfW; x++) {
          run->offset[count++] = offset + x * blockSize;
        }
        offset += blockStride;
      }

      run->comp = comp;
      run->data = reinterpret_cast<uint8_t*>(comp->coeff);
      run->qTable = nullptr;

      run->count = count;
      run->stride = 0;

      run->advance[0] = sfW * blockSize;
      run->advance[1] = sfH * blockStride - (mcuW - 1) * run->advance[0];

      // printf("  COMP #%u [%ux%u] {count=%u advance[0]=%u advance[1]=%u} SOS{start=%u end=%u}\n", comp->compId, sfW, sfH, run->count, run->advance[0], run->advance[1], _sos.ssStart, _sos.ssEnd);
    }
  }

  // --------------------------------------------------------------------------
  // [SOF0/1 - Baseline / Extended]
  // --------------------------------------------------------------------------

  if (sofMarker == kJpegMarkerSOF0 || sofMarker == kJpegMarkerSOF1) {
    JpegBlock<int16_t> tmpBlock;

    for (;;) {
      // Increment it here so we can use `mcuX == mcuW` in the inner loop.
      mcuX++;

      // Decode all blocks required by a single MCU.
      for (i = 0; i < scCount; i++) {
        JpegDecoderRun* run = &runs[i];
        uint8_t* blockData = run->data;
        uint32_t blockCount = run->count;

        for (uint32_t n = 0; n < blockCount; n++) {
          tmpBlock.reset();
          B2D_PROPAGATE(JpegDecoder_readBaselineBlock(this, stream, run->comp, tmpBlock.data));
          _bJpegOps.idct8(blockData + run->offset[n], run->stride, tmpBlock.data, run->qTable->data);
        }

        run->data = blockData + run->advance[mcuX == mcuW];
      }

      // Advance.
      if (mcuX == mcuW) {
        if (++mcuY == mcuH)
          break;
        mcuX = 0;
      }

      // Restart.
      B2D_PROPAGATE(JpegDecoder_handleRestart(this, stream, pEnd));
    }
  }

  // --------------------------------------------------------------------------
  // [SOF2 - Progressive]
  // --------------------------------------------------------------------------

  else if (sofMarker == kJpegMarkerSOF2) {
    for (;;) {
      // Increment it here so we can use `mcuX == mcuW` in the inner loop.
      mcuX++;

      // Decode all blocks required by a single MCU.
      for (i = 0; i < scCount; i++) {
        JpegDecoderRun* run = &runs[i];

        uint8_t* blockData = run->data;
        uint32_t blockCount = run->count;

        for (uint32_t n = 0; n < blockCount; n++) {
          B2D_PROPAGATE(JpegDecoder_readProgressiveBlock(this, stream, run->comp,
            reinterpret_cast<int16_t*>(blockData + run->offset[n])));
        }

        run->data = blockData + run->advance[mcuX == mcuW];
      }

      // Advance.
      if (mcuX == mcuW) {
        if (++mcuY == mcuH)
          break;
        mcuX = 0;
      }

      // Restart.
      B2D_PROPAGATE(JpegDecoder_handleRestart(this, stream, pEnd));
    }
  }

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

  else {
    B2D_NOT_REACHED();
  }

  p = stream.ptr;

  // Skip zeros at the end of the entropy stream that was not consumed `refill()`
  while (p != pEnd && p[0] == 0x00)
    p++;

  consumedBytes = (size_t)(p - pBegin);
  return kErrorOk;
}

// ============================================================================
// [b2d::JpegDecoderImpl - Process MCUs]
// ============================================================================

Error JpegDecoderImpl::processMCUs() noexcept {
  if (_jpeg.sofMarker == kJpegMarkerSOF2) {
    uint32_t numComponents = _imageInfo.numPlanes();

    // Dequantize & IDCT.
    for (uint32_t n = 0; n < numComponents; n++) {
      JpegDecoderComponent& comp = _comp[n];

      uint32_t w = (comp.pxW + 7) >> 3;
      uint32_t h = (comp.pxH + 7) >> 3;
      const JpegBlock<uint16_t>* qTable = &_qTable[comp.quantId];

      for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
          int16_t *data = comp.coeff + 64 * (i + j * comp.blW);
          _bJpegOps.idct8(comp.data + comp.osW * j * 8 + i * 8, comp.osW, data, qTable->data);
        }
      }
    }
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::JpegDecoderImpl - ConvertToRGB]
// ============================================================================

struct JpegDecoderUpsample {
  uint8_t* line[2];

  uint32_t hs, vs;  // Expansion factor in each axis.
  uint32_t w_lores; // Horizontal pixels pre-expansion.
  uint32_t ystep;   // How far through vertical expansion we are.
  uint32_t ypos;    // Which pre-expansion row we're on.

  // Static JFIF-centered resampling (across block boundaries).
  uint8_t* (*upsample)(uint8_t* out, uint8_t* in0, uint8_t* in1, uint32_t w, uint32_t hs);
};

Error JpegDecoderImpl::convertToRgb(ImageBuffer& dst) noexcept {
  uint32_t w = uint32_t(_imageInfo.width());
  uint32_t h = uint32_t(_imageInfo.height());

  B2D_ASSERT(uint32_t(dst.width()) >= w);
  B2D_ASSERT(uint32_t(dst.height()) >= h);

  uint8_t* dstLine = dst.pixelData();
  intptr_t dstStride = dst.stride();

  MemBufferTmp<1024 * 3 + 16> tmpMem;

  // Allocate a line buffer that's big enough for upsampling off the edges with
  // upsample factor of 4.
  uint32_t numComponents = _imageInfo.numPlanes();
  uint32_t lineStride = Support::alignUp(w + 3, 16);
  uint8_t* lineBuffer = static_cast<uint8_t*>(tmpMem.alloc(lineStride * numComponents));

  if (B2D_UNLIKELY(!lineBuffer))
    return DebugUtils::errored(kErrorNoMemory);

  JpegDecoderUpsample upsample[4];
  uint32_t y, k;

  uint8_t* pPlane[4];
  uint8_t* pBuffer[4];

  for (k = 0; k < numComponents; k++) {
    JpegDecoderComponent& comp = _comp[k];
    JpegDecoderUpsample* r = &upsample[k];

    pBuffer[k] = lineBuffer + k * lineStride;

    r->hs      = int(_jpeg.mcuSfW) / comp.sfW;
    r->vs      = int(_jpeg.mcuSfH) / comp.sfH;
    r->ystep   = r->vs >> 1;
    r->w_lores = (w + r->hs - 1) / r->hs;
    r->ypos    = 0;
    r->line[0] = comp.data;
    r->line[1] = comp.data;

    if      (r->hs == 1 && r->vs == 1) r->upsample = _bJpegOps.upsample1x1;
    else if (r->hs == 1 && r->vs == 2) r->upsample = _bJpegOps.upsample1x2;
    else if (r->hs == 2 && r->vs == 1) r->upsample = _bJpegOps.upsample2x1;
    else if (r->hs == 2 && r->vs == 2) r->upsample = _bJpegOps.upsample2x2;
    else                               r->upsample = _bJpegOps.upsampleAny;
  }

  // Now go ahead and resample.
  for (y = 0; y < h; y++, dstLine += dstStride) {
    for (k = 0; k < numComponents; k++) {
      JpegDecoderComponent& comp = _comp[k];
      JpegDecoderUpsample* r = &upsample[k];

      int y_bot = r->ystep >= (r->vs >> 1);
      pPlane[k] = r->upsample(pBuffer[k], r->line[y_bot], r->line[1 - y_bot], r->w_lores, r->hs);

      if (++r->ystep >= r->vs) {
        r->ystep = 0;
        r->line[0] = r->line[1];
        if (++r->ypos < comp.pxH)
          r->line[1] += comp.osW;
      }
    }

    uint8_t* pY = pPlane[0];
    if (numComponents == 3) {
      _bJpegOps.convYCbCr8ToRGB32(dstLine, pY, pPlane[1], pPlane[2], w);
    }
    else {
      for (uint32_t x = 0; x < w; x++) {
        Support::writeU32a(dstLine + x * 4, 0xFF000000u + uint32_t(pY[x]) * 0x010101u);
      }
    }
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::JpegEncoderImpl - Construction / Destruction]
// ============================================================================

JpegEncoderImpl::JpegEncoderImpl() noexcept {}
JpegEncoderImpl::~JpegEncoderImpl() noexcept {}

// ============================================================================
// [b2d::JpegEncoderImpl - Interface]
// ============================================================================

Error JpegEncoderImpl::reset() noexcept {
  Base::reset();
  return kErrorOk;
}

Error JpegEncoderImpl::encode(Buffer& dst, const Image& src) noexcept {
  // TODO: Encoder
  return DebugUtils::errored(kErrorNotImplemented);
}

// ============================================================================
// [b2d::JpegCodecImpl - Construction / Destruction]
// ============================================================================

JpegCodecImpl::JpegCodecImpl() noexcept {
  _features = ImageCodec::kFeatureDecoder  |
              ImageCodec::kFeatureEncoder  |
              ImageCodec::kFeatureLossy    ;

  _formats  = ImageCodec::kFormatGray8     |
              ImageCodec::kFormatRGB24     |
              ImageCodec::kFormatYCbCr24   ;

  setupStrings("B2D\0" "JPEG\0" "image/jpeg\0" "jpg|jpeg|jif|jfi|jfif\0");
}

JpegCodecImpl::~JpegCodecImpl() noexcept {}

// ============================================================================
// [b2d::JpegCodecImpl - Interface]
// ============================================================================

int JpegCodecImpl::inspect(const uint8_t* data, size_t size) const noexcept {
  // JPEG minimum size and signature (SOI).
  if (size < 2 || data[0] != 0xFF || data[1] != kJpegMarkerSOI) return 0;

  // JPEG signature has to be followed by a marker that starts with 0xFF.
  if (size > 2 && data[2] != 0xFF) return 0;

  // Don't return 100, maybe there are other codecs with a higher precedence.
  return 95;
}

ImageDecoderImpl* JpegCodecImpl::createDecoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<JpegDecoderImpl>(), ImageDecoder::none().impl()); }
ImageEncoderImpl* JpegCodecImpl::createEncoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<JpegEncoderImpl>(), ImageEncoder::none().impl()); }

// ============================================================================
// [b2d::Jpeg - Runtime Handlers]
// ============================================================================

void ImageCodecOnInitJpeg(Array<ImageCodec>* codecList) noexcept {
  JpegOps& ops = _bJpegOps;

  #if B2D_ARCH_SSE2
  ops.idct8             = JpegInternal::idct8_sse2;
  ops.convYCbCr8ToRGB32 = JpegInternal::convYCbCr8ToRGB32_sse2;
  #else
  ops.idct8             = JpegInternal::idct8_ref;
  ops.convYCbCr8ToRGB32 = JpegInternal::convYCbCr8ToRGB32_ref;
  #endif

  ops.upsample1x1       = JpegInternal::upsample1x1_ref;
  ops.upsample1x2       = JpegInternal::upsample1x2_ref;
  ops.upsample2x1       = JpegInternal::upsample2x1_ref;
  ops.upsample2x2       = JpegInternal::upsample2x2_ref;
  ops.upsampleAny       = JpegInternal::upsampleAny_ref;

  codecList->append(ImageCodec(AnyInternal::newImplT<JpegCodecImpl>()));
}

B2D_END_NAMESPACE
