// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// The PNG codec is based on stb_image <https://github.com/nothings/stb>
// released into PUBLIC DOMAIN. Blend2D's PNG codec can be distributed
// under Blend2D's ZLIB license or under STB's PUBLIC DOMAIN as well.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/deflate_p.h"
#include "../codec/pngcodec_p.h"
#include "../codec/pngutils_p.h"
#include "../core/allocator.h"
#include "../core/math.h"
#include "../core/membuffer.h"
#include "../core/pixelutils_p.h"
#include "../core/runtime.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Png - Constants]
// ============================================================================

// PNG file signature (8 bytes).
static const uint8_t bPngSignature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
// Count of samples per "ColorType".
static const uint8_t bPngColorTypeToSamples[7] = { 0x01, 0, 0x03, 0x01, 0x02, 0, 0x04 };
// Allowed bits-per-sample per "ColorType"
static const uint8_t bPngColorTypeBitDepths[7] = { 0x1F, 0, 0x18, 0x0F, 0x18, 0, 0x18 };

// ============================================================================
// [b2d::Png - Helpers]
// ============================================================================

static B2D_INLINE bool bPngCheckColorTypeAndBitDepth(uint32_t colorType, uint32_t depth) noexcept {
  // TODO: 16-BPC.
  if (depth == 16)
    return false;

  return colorType < B2D_ARRAY_SIZE(bPngColorTypeBitDepths) &&
         Support::isPowerOf2(depth) &&
         (bPngColorTypeBitDepths[colorType] & depth) != 0;
}

static B2D_INLINE void bPngCreateGrayscalePalette(uint32_t* pal, uint32_t depth) noexcept {
  static const uint32_t scaleTable[9] = { 0, 0xFF, 0x55, 0, 0x11, 0, 0, 0, 0x01 };
  B2D_ASSERT(depth < B2D_ARRAY_SIZE(scaleTable));

  uint32_t scale = uint32_t(scaleTable[depth]) * 0x00010101;
  uint32_t count = 1u << depth;
  uint32_t value = 0xFF000000;

  for (uint32_t i = 0; i < count; i++, value += scale)
    pal[i] = value;
}

// ============================================================================
// [b2d::Png - Interlace / Deinterlace]
// ============================================================================

// A single PNG interlace/deinterlace step related to the full image size.
struct PngInterlaceStep {
  uint32_t used;
  uint32_t width;
  uint32_t height;
  uint32_t bpl;

  uint32_t offset;
  uint32_t size;
};

// PNG deinterlace table data.
struct PngInterlaceTable {
  uint8_t xOff;
  uint8_t yOff;
  uint8_t xPow;
  uint8_t yPow;
};

// Passes start from zero to stay compatible with interlacing tables, however,
// this representation is not visually compatible with PNG spec, where passes
// are indexed from `1` (that's the only difference).
//
//        8x8 block
//   +-----------------+
//   | 0 5 3 5 1 5 3 5 |
//   | 6 6 6 6 6 6 6 6 |
//   | 4 5 4 5 4 5 4 5 |
//   | 6 6 6 6 6 6 6 6 |
//   | 2 5 3 5 2 5 3 5 |
//   | 6 6 6 6 6 6 6 6 |
//   | 4 5 4 5 4 5 4 5 |
//   | 6 6 6 6 6 6 6 6 |
//   +-----------------+
static const PngInterlaceTable bPngInterlaceAdam7[7] = {
  { 0, 0, 3, 3 },
  { 4, 0, 3, 3 },
  { 0, 4, 2, 3 },
  { 2, 0, 2, 2 },
  { 0, 2, 1, 2 },
  { 1, 0, 1, 1 },
  { 0, 1, 0, 1 }
};

// No interlacing.
static const PngInterlaceTable bPngInterlaceNone[1] = {
  { 0, 0, 0, 0 }
};

static B2D_INLINE uint32_t bPngCalculateInterlaceSteps(
  PngInterlaceStep* dst, const PngInterlaceTable* table, uint32_t numSteps,
  uint32_t sampleDepth, uint32_t sampleCount,
  uint32_t w, uint32_t h) noexcept {

  // Byte-offset of each chunk.
  uint32_t offset = 0;

  for (uint32_t i = 0; i < numSteps; i++, dst++) {
    const PngInterlaceTable& tab = table[i];

    uint32_t sx = 1 << tab.xPow;
    uint32_t sy = 1 << tab.yPow;
    uint32_t sw = (w + sx - tab.xOff - 1) >> tab.xPow;
    uint32_t sh = (h + sy - tab.yOff - 1) >> tab.yPow;

    // If the reference image contains fewer than five columns or fewer than
    // five rows, some passes will be empty, decoders must handle this case.
    uint32_t used = sw != 0 && sh != 0;

    // NOTE: No need to check for overflow at this point as we have already
    // calculated the total BPL of the whole image, and since interlacing is
    // splitting it into multiple images, it can't overflow the base size.
    uint32_t bpl = ((sw * sampleDepth + 7) / 8) * sampleCount + 1;
    uint32_t size = used ? bpl * sh : uint32_t(0);

    dst->used = used;
    dst->width = sw;
    dst->height = sh;
    dst->bpl = bpl;

    dst->offset = offset;
    dst->size = size;

    // Here we should be safe...
    Support::OverflowFlag of;
    offset = Support::safeAdd(offset, size, &of);

    if (B2D_UNLIKELY(of))
      return 0;
  }

  return offset;
}

#define COMB_BYTE_1BPP(B0, B1, B2, B3, B4, B5, B6, B7) \
  uint8_t(((B0) & 0x80) + ((B1) & 0x40) + ((B2) & 0x20) + ((B3) & 0x10) + \
          ((B4) & 0x08) + ((B5) & 0x04) + ((B6) & 0x02) + ((B7) & 0x01) )

#define COMB_BYTE_2BPP(B0, B1, B2, B3) \
  uint8_t(((B0) & 0xC0) + ((B1) & 0x30) + ((B2) & 0x0C) + ((B3) & 0x03))

#define COMB_BYTE_4BPP(B0, B1) \
  uint8_t(((B0) & 0xF0) + ((B1) & 0x0F))

// Deinterlace a PNG image that has depth less than 8 bits. This is a bit
// tricky as one byte describes two or more pixels that can be fetched from
// 1st to 6th progressive images. Basically each bit depth is implemented
// separatery as generic case would be very inefficient. Also, the destination
// image is handled pixel-by-pixel fetching data from all possible scanlines
// as necessary - this is a bit different compared with `bPngDeinterlaceBytes()`.
template<uint32_t N>
static B2D_INLINE void bPngDeinterlaceBits(
  uint8_t* dstLine, intptr_t dstStride, const PixelConverter& cvt,
  uint8_t* tmpLine, intptr_t tmpStride, const uint8_t* data, const PngInterlaceStep* steps,
  uint32_t w, uint32_t h) noexcept {

  const uint8_t* d0 = data + steps[0].offset;
  const uint8_t* d1 = data + steps[1].offset;
  const uint8_t* d2 = data + steps[2].offset;
  const uint8_t* d3 = data + steps[3].offset;
  const uint8_t* d4 = data + steps[4].offset;
  const uint8_t* d5 = data + steps[5].offset;

  B2D_ASSERT(h != 0);

  // We store only to odd scanlines.
  uint32_t y = (h + 1) / 2;
  uint32_t n = 0;

  for (;;) {
    uint8_t* tmpData = tmpLine + n * tmpStride;
    uint32_t x = w;

    // ------------------------------------------------------------------------
    // [1-BPP]
    // ------------------------------------------------------------------------

    if (N == 1) {
      switch (n) {
        // [a b b b a b b b]
        // [0 5 3 5 1 5 3 5]
        case 0: {
          uint32_t a = 0, b;

          d0 += 1;
          d1 += (x >= 5);
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 32) {
            // Fetched every second iteration.
            if (!(a & 0x80000000))
              a = (uint32_t(*d0++)) + (uint32_t(*d1++) << 8) + 0x08000000u;

            b = (uint32_t(*d3++)      ) +
                (uint32_t(d5[0]) <<  8) +
                (uint32_t(d5[1]) << 16) ;
            d5 += 2;

            tmpData[0] = COMB_BYTE_1BPP(a     , b >>  9, b >> 2, b >> 10, a >> 12, b >> 11, b >> 5, b >> 12);
            tmpData[1] = COMB_BYTE_1BPP(a << 1, b >>  5, b     , b >>  6, a >> 11, b >>  7, b >> 3, b >>  8);
            tmpData[2] = COMB_BYTE_1BPP(a << 2, b >> 17, b << 2, b >> 18, a >> 10, b >> 19, b >> 1, b >> 20);
            tmpData[3] = COMB_BYTE_1BPP(a << 3, b >> 13, b << 4, b >> 14, a >>  9, b >> 15, b << 1, b >> 16);
            tmpData += 4;

            a <<= 4;
            x -= 32;
          }

          if (!x)
            break;

          if (!(a & 0x80000000)) {
            a = uint32_t(*d0++);
            if (x >= 5)
              a += uint32_t(*d1++) << 8;
          }

          b = 0;
          if (x >=  3) b  = uint32_t(*d3++);
          if (x >=  2) b += uint32_t(*d5++) <<  8;
          if (x >= 18) b += uint32_t(*d5++) << 16;

          tmpData[0] = COMB_BYTE_1BPP(a     , b >>  9, b >> 2, b >> 10, a >> 12, b >> 11, b >> 5, b >> 12); if (x <=  8) break;
          tmpData[1] = COMB_BYTE_1BPP(a << 1, b >>  5, b     , b >>  6, a >> 11, b >>  7, b >> 3, b >>  8); if (x <= 16) break;
          tmpData[2] = COMB_BYTE_1BPP(a << 2, b >> 17, b << 2, b >> 18, a >> 10, b >> 19, b >> 1, b >> 20); if (x <= 24) break;
          tmpData[3] = COMB_BYTE_1BPP(a << 3, b >> 13, b << 4, b >> 14, a >>  9, b >> 15, b << 1, b >> 16);

          break;
        }

        // [a b a b a b a b]
        // [2 5 3 5 2 5 3 5]
        case 2: {
          uint32_t a, b;

          d2 += 1;
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 32) {
            a = uint32_t(*d2++) + (uint32_t(*d3++) << 8);
            b = uint32_t(d5[0]) + (uint32_t(d5[1]) << 8);
            d5 += 2;

            tmpData[0] = COMB_BYTE_1BPP(a     , b >>  1, a >> 10, b >>  2, a >>  3, b >>  3, a >> 13, b >>  4);
            tmpData[1] = COMB_BYTE_1BPP(a << 2, b <<  3, a >>  8, b <<  2, a >>  1, b <<  1, a >> 11, b      );
            tmpData[2] = COMB_BYTE_1BPP(a << 4, b >>  9, a >>  6, b >> 10, a <<  1, b >> 11, a >>  9, b >> 12);
            tmpData[3] = COMB_BYTE_1BPP(a << 6, b >>  5, a >>  4, b >>  6, a <<  3, b >>  7, a >>  7, b >>  8);
            tmpData += 4;

            x -= 32;
          }

          if (!x)
            break;

          a = uint32_t(*d2++);
          b = 0;

          if (x >=  3) a += uint32_t(*d3++) << 8;
          if (x >=  2) b  = uint32_t(*d5++);
          if (x >= 18) b += uint32_t(*d5++) << 8;

          tmpData[0] = COMB_BYTE_1BPP(a     , b >>  1, a >> 10, b >>  2, a >>  3, b >>  3, a >> 13, b >>  4); if (x <=  8) break;
          tmpData[1] = COMB_BYTE_1BPP(a << 2, b <<  3, a >>  8, b <<  2, a >>  1, b <<  1, a >> 11, b      ); if (x <= 16) break;
          tmpData[2] = COMB_BYTE_1BPP(a << 4, b >>  9, a >>  6, b >> 10, a <<  1, b >> 11, a >>  9, b >> 12); if (x <= 24) break;
          tmpData[3] = COMB_BYTE_1BPP(a << 6, b >>  5, a >>  4, b >>  6, a <<  3, b >>  7, a >>  7, b >>  8);

          break;
        }

        // [a b a b a b a b]
        // [4 5 4 5 4 5 4 5]
        case 1:
        case 3: {
          uint32_t a, b;

          d4 += 1;
          d5 += (x >= 2);

          while (x >= 16) {
            a = uint32_t(*d4++);
            b = uint32_t(*d5++);

            tmpData[0] = COMB_BYTE_1BPP(a     , b >> 1, a >> 1, b >> 2, a >> 2, b >> 3, a >> 3, b >> 4);
            tmpData[1] = COMB_BYTE_1BPP(a << 4, b << 3, a << 3, b << 2, a << 2, b << 1, a << 1, b     );
            tmpData += 2;

            x -= 16;
          }

          if (!x)
            break;

          a = uint32_t(*d4++);
          b = 0;

          if (x >= 2)
            b = uint32_t(*d5++);

          tmpData[0] = COMB_BYTE_1BPP(a     , b >> 1, a >> 1, b >> 2, a >> 2, b >> 3, a >> 3, b >> 4); if (x <= 8) break;
          tmpData[1] = COMB_BYTE_1BPP(a << 4, b << 3, a << 3, b << 2, a << 2, b << 1, a << 1, b     );

          break;
        }
      }
    }

    // ------------------------------------------------------------------------
    // [2-BPP]
    // ------------------------------------------------------------------------

    else if (N == 2) {
      switch (n) {
        // [aa bb bb bb][aa bb bb bb]
        // [00 55 33 55][11 55 33 55]
        case 0: {
          uint32_t a = 0, b;

          d0 += 1;
          d1 += (x >= 5);
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 16) {
            // Fetched every second iteration.
            if (!(a & 0x80000000))
              a = (uint32_t(*d0++)     ) +
                  (uint32_t(*d1++) << 8) + 0x08000000;

            b = (uint32_t(*d3++)      ) +
                (uint32_t(d5[0]) <<  8) +
                (uint32_t(d5[1]) << 16) ;
            d5 += 2;

            tmpData[0] = COMB_BYTE_2BPP(a     , b >> 10, b >> 4, b >> 12);
            tmpData[1] = COMB_BYTE_2BPP(a >> 8, b >>  6, b >> 2, b >>  8);
            tmpData[2] = COMB_BYTE_2BPP(a << 2, b >> 18, b     , b >> 20);
            tmpData[3] = COMB_BYTE_2BPP(a >> 6, b >> 14, b << 2, b >> 16);
            tmpData += 4;

            a <<= 4;
            x -= 16;
          }

          if (!x)
            break;

          if (!(a & 0x80000000)) {
            a = (uint32_t(*d0++));
            if (x >= 5)
              a += (uint32_t(*d1++) << 8);
          }

          b = 0;
          if (x >=  3) b  = (uint32_t(*d3++)      );
          if (x >=  2) b += (uint32_t(*d5++) <<  8);
          if (x >= 10) b += (uint32_t(*d5++) << 16);

          tmpData[0] = COMB_BYTE_2BPP(a     , b >> 10, b >> 4, b >> 12); if (x <=  4) break;
          tmpData[1] = COMB_BYTE_2BPP(a >> 8, b >>  6, b >> 2, b >>  8); if (x <=  8) break;
          tmpData[2] = COMB_BYTE_2BPP(a << 2, b >> 18, b     , b >> 20); if (x <= 12) break;
          tmpData[3] = COMB_BYTE_2BPP(a >> 6, b >> 14, b << 2, b >> 16);

          break;
        }

        // [aa bb aa bb][aa bb aa bb]
        // [22 55 33 55][22 55 33 55]
        case 2: {
          uint32_t a, b;

          d2 += 1;
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 16) {
            a = uint32_t(*d2++) + (uint32_t(*d3++) << 8);
            b = uint32_t(*d5++);

            tmpData[0] = COMB_BYTE_2BPP(a     , b >>  2, a >> 12, b >>  4);
            tmpData[1] = COMB_BYTE_2BPP(a << 2, b <<  2, a >> 10, b      );

            b = uint32_t(*d5++);

            tmpData[2] = COMB_BYTE_2BPP(a << 4, b >>  2, a >>  8, b >>  4);
            tmpData[3] = COMB_BYTE_2BPP(a << 6, b <<  2, a >>  6, b      );
            tmpData += 4;

            x -= 16;
          }

          if (!x)
            break;

          a = uint32_t(*d2++);
          b = 0;

          if (x >=  3) a  = (uint32_t(*d3++) << 8);
          if (x >=  2) b  = (uint32_t(*d5++)     );
          if (x >= 10) b += (uint32_t(*d5++) << 8);

          tmpData[0] = COMB_BYTE_2BPP(a     , b >>  2, a >> 12, b >>  4); if (x <=  4) break;
          tmpData[1] = COMB_BYTE_2BPP(a << 2, b <<  2, a >> 10, b      ); if (x <=  8) break;
          tmpData[2] = COMB_BYTE_2BPP(a << 4, b >> 10, a >>  8, b >> 12); if (x <= 12) break;
          tmpData[3] = COMB_BYTE_2BPP(a << 6, b >>  6, a >>  6, b >>  8);

          break;
        }

        // [aa bb aa bb][aa bb aa bb]
        // [44 55 44 55][44 55 44 55]
        case 1:
        case 3: {
          uint32_t a, b;

          d4 += 1;
          d5 += (x >= 2);

          while (x >= 8) {
            a = uint32_t(*d4++);
            b = uint32_t(*d5++);

            tmpData[0] = COMB_BYTE_2BPP(a     , b >> 2, a >> 2, b >> 4);
            tmpData[1] = COMB_BYTE_2BPP(a << 4, b << 2, a << 2, b     );
            tmpData += 2;

            x -= 8;
          }

          if (!x)
            break;

          a = uint32_t(*d4++);
          b = 0;

          if (x >= 2)
            b = uint32_t(*d5++);

          tmpData[0] = COMB_BYTE_2BPP(a     , b >> 10, b >> 4, b >> 12); if (x <=  4) break;
          tmpData[1] = COMB_BYTE_2BPP(a >> 8, b >>  6, b >> 2, b >>  8);

          break;
        }
      }
    }

    // ------------------------------------------------------------------------
    // [4-BPP]
    // ------------------------------------------------------------------------

    else if (N == 4) {
      switch (n) {
        // [aaaa bbbb][bbbb bbbb][aaaa bbbb][bbbb bbbb]
        // [0000 5555][3333 5555][1111 5555][3333 5555]
        case 0: {
          uint32_t a = 0, b;

          d0 += 1;
          d1 += (x >= 5);
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 8) {
            // Fetched every second iteration.
            if (!(a & 0x80000000))
              a = (uint32_t(*d0++)      ) +
                  (uint32_t(*d1++) <<  8) + 0x08000000;

            b = (uint32_t(*d3++)      ) +
                (uint32_t(d5[0]) <<  8) +
                (uint32_t(d5[1]) << 16) ;
            d5 += 2;

            tmpData[0] = COMB_BYTE_4BPP(a     , b >> 12);
            tmpData[1] = COMB_BYTE_4BPP(b     , b >>  8);
            tmpData[2] = COMB_BYTE_4BPP(a >> 8, b >> 20);
            tmpData[3] = COMB_BYTE_4BPP(b << 4, b >> 16);
            tmpData += 4;

            a <<= 4;
            x -= 8;
          }

          if (!x)
            break;

          if (!(a & 0x80000000)) {
            a = (uint32_t(*d0++));
            if (x >= 5)
              a += (uint32_t(*d1++) << 8);
          }

          b = 0;
          if (x >= 3) b  = (uint32_t(*d3++)      );
          if (x >= 2) b += (uint32_t(*d5++) <<  8);
          if (x >= 6) b += (uint32_t(*d5++) << 16);

          tmpData[0] = COMB_BYTE_4BPP(a     , b >> 12); if (x <= 2) break;
          tmpData[1] = COMB_BYTE_4BPP(b     , b >>  8); if (x <= 4) break;
          tmpData[2] = COMB_BYTE_4BPP(a >> 8, b >> 20); if (x <= 6) break;
          tmpData[3] = COMB_BYTE_4BPP(b << 4, b >> 16);

          break;
        }

        // [aaaa bbbb][aaaa bbbb][aaaa bbbb][aaaa bbbb]
        // [2222 5555][3333 5555][2222 5555][3333 5555]
        case 2: {
          uint32_t a, b;

          d2 += 1;
          d3 += (x >= 3);
          d5 += (x >= 2);

          while (x >= 8) {
            a = uint32_t(*d2++) + (uint32_t(*d3++) << 8);
            b = uint32_t(*d5++);
            tmpData[0] = COMB_BYTE_4BPP(a     , b >>  4);
            tmpData[1] = COMB_BYTE_4BPP(a >> 8, b      );

            b = uint32_t(*d5++);
            tmpData[2] = COMB_BYTE_4BPP(a << 4, b >>  4);
            tmpData[3] = COMB_BYTE_4BPP(a >> 4, b      );
            tmpData += 4;

            x -= 8;
          }

          if (!x)
            break;

          a = uint32_t(*d2++);
          b = 0;

          if (x >= 3) a += (uint32_t(*d3++) << 8);
          if (x >= 2) b  = (uint32_t(*d5++)     );
          tmpData[0] = COMB_BYTE_4BPP(a     , b >> 4); if (x <= 2) break;
          tmpData[1] = COMB_BYTE_4BPP(a >> 8, b     ); if (x <= 4) break;

          if (x >= 5) b = uint32_t(*d5++);
          tmpData[2] = COMB_BYTE_4BPP(a << 4, b >> 4); if (x <= 6) break;
          tmpData[3] = COMB_BYTE_4BPP(a >> 4, b     );

          break;
        }

        // [aaaa bbbb aaaa bbbb][aaaa bbbb aaaa bbbb]
        // [4444 5555 4444 5555][4444 5555 4444 5555]
        case 1:
        case 3: {
          uint32_t a, b;

          d4 += 1;
          d5 += (x >= 2);

          while (x >= 4) {
            a = uint32_t(*d4++);
            b = uint32_t(*d5++);

            tmpData[0] = COMB_BYTE_4BPP(a     , b >> 4);
            tmpData[1] = COMB_BYTE_4BPP(a << 4, b     );
            tmpData += 2;

            x -= 4;
          }

          if (!x)
            break;

          a = uint32_t(*d4++);
          b = 0;

          if (x >= 2)
            b = uint32_t(*d5++);

          tmpData[0] = COMB_BYTE_4BPP(a     , b >> 4); if (x <= 2) break;
          tmpData[1] = COMB_BYTE_4BPP(a << 4, b     );

          break;
        }
      }
    }

    // Don't change to `||`, both have to be executed!
    if ((--y == 0) | (++n == 4)) {
      cvt.convertRect(dstLine, dstStride * 2, tmpLine, tmpStride, w, n);
      dstLine += dstStride * 8;

      if (y == 0)
        break;
      n = 0;
    }
  }
}

// Copy `N` bytes from unaligned `src` into aligned `dst`. Allows us to handle
// some special cases if the CPU supports unaligned reads/writes from/to memory.
template<uint32_t N>
static B2D_INLINE const uint8_t* bPngCopyBytes(uint8_t* dst, const uint8_t* src) noexcept {
  if (N == 2) {
    Support::writeU16a(dst, Support::readU16u(src));
  }
  else if (N == 4) {
    Support::writeU32a(dst, Support::readU32u(src));
  }
  else if (N == 8) {
    Support::writeU32a(dst + 0, Support::readU32u(src + 0));
    Support::writeU32a(dst + 4, Support::readU32u(src + 4));
  }
  else {
    if (N >= 1) dst[0] = src[0];
    if (N >= 2) dst[1] = src[1];
    if (N >= 3) dst[2] = src[2];
    if (N >= 4) dst[3] = src[3];
    if (N >= 5) dst[4] = src[4];
    if (N >= 6) dst[5] = src[5];
    if (N >= 7) dst[6] = src[6];
    if (N >= 8) dst[7] = src[7];
  }
  return src + N;
}

template<uint32_t N>
static B2D_INLINE void bPngDeinterlaceBytes(
  uint8_t* dstLine, intptr_t dstStride, const PixelConverter& cvt,
  uint8_t* tmpLine, intptr_t tmpStride, const uint8_t* data, const PngInterlaceStep* steps,
  uint32_t w, uint32_t h) noexcept {

  const uint8_t* d0 = data + steps[0].offset;
  const uint8_t* d1 = data + steps[1].offset;
  const uint8_t* d2 = data + steps[2].offset;
  const uint8_t* d3 = data + steps[3].offset;
  const uint8_t* d4 = data + steps[4].offset;
  const uint8_t* d5 = data + steps[5].offset;

  B2D_ASSERT(h != 0);

  // We store only to odd scanlines.
  uint32_t y = (h + 1) / 2;
  uint32_t n = 0;
  uint32_t xMax = w * N;

  for (;;) {
    uint8_t* tmpData = tmpLine + n * tmpStride;
    uint32_t x;

    switch (n) {
      // [05351535]
      case 0: {
        d0 += 1;
        d1 += (w >= 5);
        d3 += (w >= 3);
        d5 += (w >= 2);

        for (x = 0 * N; x < xMax; x += 8 * N) d0 = bPngCopyBytes<N>(tmpData + x, d0);
        for (x = 4 * N; x < xMax; x += 8 * N) d1 = bPngCopyBytes<N>(tmpData + x, d1);
        for (x = 2 * N; x < xMax; x += 4 * N) d3 = bPngCopyBytes<N>(tmpData + x, d3);
        for (x = 1 * N; x < xMax; x += 2 * N) d5 = bPngCopyBytes<N>(tmpData + x, d5);

        break;
      }

      // [25352535]
      case 2: {
        d2 += 1;
        d3 += (w >= 3);
        d5 += (w >= 2);

        for (x = 0 * N; x < xMax; x += 4 * N) d2 = bPngCopyBytes<N>(tmpData + x, d2);
        for (x = 2 * N; x < xMax; x += 4 * N) d3 = bPngCopyBytes<N>(tmpData + x, d3);
        for (x = 1 * N; x < xMax; x += 2 * N) d5 = bPngCopyBytes<N>(tmpData + x, d5);

        break;
      }

      // [45454545]
      case 1:
      case 3: {
        d4 += 1;
        d5 += (w >= 2);

        for (x = 0 * N; x < xMax; x += 2 * N) d4 = bPngCopyBytes<N>(tmpData + x, d4);
        for (x = 1 * N; x < xMax; x += 2 * N) d5 = bPngCopyBytes<N>(tmpData + x, d5);

        break;
      }
    }

    // Don't change to `||`, both have to be executed!
    if ((--y == 0) | (++n == 4)) {
      cvt.convertRect(dstLine, dstStride * 2, tmpLine, tmpStride, w, n);
      dstLine += dstStride * 8;

      if (y == 0)
        break;
      n = 0;
    }
  }
}

// ============================================================================
// [b2d::PngDecoderImpl - Construction / Destruction]
// ============================================================================

PngDecoderImpl::PngDecoderImpl() noexcept { reset(); }
PngDecoderImpl::~PngDecoderImpl() noexcept {}

// ============================================================================
// [b2d::PngDecoderImpl - Interface]
// ============================================================================

struct PngDecoderReadData {
  const uint8_t* p;
  size_t index;
};

static bool B2D_CDECL bPngDecoderReadFunc(PngDecoderReadData* rd, const uint8_t** pData, const uint8_t** pEnd) noexcept {
  const uint8_t* p = rd->p;
  size_t index = rd->index;

  // Ignore any repeated calls if we failed once. We have to do it here as
  // the Deflate context doesn't do that and can repeatedly call `readData`.
  if (p == nullptr)
    return false;

  uint32_t chunkTag;
  uint32_t chunkSize;

  // Spec says that IDAT size can be zero so loop until it's non-zero. Zero
  // IDATs are not so common and there was a security issue in an old libpng
  // related to zero size IDATs.
  do {
    chunkTag = Support::readU32uBE(p + index + 4);
    chunkSize = Support::readU32uBE(p + index + 0);

    // IDAT's have to be consecutive, if terminated it means that there is no
    // more data to be consumed by deflate.
    if (chunkTag != kPngTag_IDAT) {
      rd->p = nullptr;
      return false;
    }

    index += 12 + chunkSize;
  } while (chunkSize == 0);

  p += index - chunkSize - 4;
  rd->index = index;

  *pData = p;
  *pEnd = p + chunkSize;
  return true;
}

Error PngDecoderImpl::reset() noexcept {
  Base::reset();

  std::memset(&_png, 0, sizeof(_png));
  return kErrorOk;
}

Error PngDecoderImpl::setup(const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  if (_bufferIndex != 0)
    return setError(DebugUtils::errored(kErrorInvalidState));

  // Signature, IHDR tag (8 bytes), IHDR data (13 bytes), IHDR CRC (4 bytes).
  if (size < kPngMinSize)
    return setError(DebugUtils::errored(kErrorCodecIncompleteData));

  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + size;

  // Check PNG signature.
  if (std::memcmp(p, bPngSignature, 8) != 0)
    return setError(DebugUtils::errored(kErrorCodecInvalidSignature));
  p += 8;

  // Expect IHDR or CgBI chunk.
  uint32_t chunkTag = Support::readU32uBE(p + 4);
  uint32_t chunkSize = Support::readU32uBE(p + 0);

  // --------------------------------------------------------------------------
  // [CgBI - BrokenByApple]
  // --------------------------------------------------------------------------

  // Support "Optimized for iPhone" violation of the PNG specification, see:
  //   1. http://www.jongware.com/pngdefry.html
  //   2. http://iphonedevwiki.net/index.php/CgBI_file_format
  if (chunkTag == kPngTag_CgBI) {
    const int kTagSize_CgBI = 16;

    if (chunkSize != 4)
      return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

    if (size < kPngMinSize + kTagSize_CgBI)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    // Skip "CgBI" chunk, data, and CRC.
    p += 12 + chunkSize;

    chunkTag = Support::readU32uBE(p + 4);
    chunkSize = Support::readU32uBE(p + 0);

    _png.chunkTags |= kPngTagMask_CgBI;
    _png.brokenByApple = true;
  }

  p += 8;

  // --------------------------------------------------------------------------
  // [IHDR]
  // --------------------------------------------------------------------------

  if (chunkTag != kPngTag_IHDR || chunkSize != 13)
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  // IHDR Data [13 bytes].
  uint32_t w = Support::readU32uBE(p + 0);
  uint32_t h = Support::readU32uBE(p + 4);

  uint32_t sampleDepth = p[8];
  uint32_t colorType   = p[9];
  uint32_t compression = p[10];
  uint32_t filter      = p[11];
  uint32_t progressive = p[12];

  p += 13;

  // Ignore CRC.
  p += 4;

  // N can't be zero or greater than `2^31 - 1`.
  if (w == 0 || h == 0)
    return setError(DebugUtils::errored(kErrorCodecInvalidSize));

  if (w >= 0x80000000 || h >= 0x80000000)
    return setError(DebugUtils::errored(kErrorCodecUnsupportedSize));

  if (!bPngCheckColorTypeAndBitDepth(colorType, sampleDepth))
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  // Compression and filter has to be zero, progressive can be [0, 1].
  if (compression != 0 || filter != 0 || progressive >= 2)
    return setError(DebugUtils::errored(kErrorCodecInvalidFormat));

  // Setup the Image+PNG information.
  _png.chunkTags |= kPngTagMask_IHDR;
  _png.colorType = uint8_t(colorType);
  _png.sampleDepth = uint8_t(sampleDepth);
  _png.sampleCount = bPngColorTypeToSamples[colorType];

  _imageInfo.setSize(int(w), int(h));
  _imageInfo.setDepth(sampleDepth * uint32_t(_png.sampleCount));
  _imageInfo.setProgressive(progressive);
  _imageInfo.setNumFrames(1);

  _bufferIndex = (size_t)(p - pBegin);
  return kErrorOk;
}

Error PngDecoderImpl::decode(Image& dst, const uint8_t* p, size_t size) noexcept {
  B2D_PROPAGATE(_error);

  if (_bufferIndex == 0)
    B2D_PROPAGATE(setup(p, size));

  if (_frameIndex)
    return DebugUtils::errored(kErrorCodecNoMoreFrames);

  const uint8_t* pBegin = p;
  const uint8_t* pEnd = p + size;

  // Make sure we start at the right position. PngDecoderImpl::setup() does
  // handle the signature and IHDR chunk and sets the index accordingly.
  if ((size_t)(pEnd - p) < _bufferIndex)
    return DebugUtils::errored(kErrorInvalidState);
  p += _bufferIndex;

  // Basic information.
  uint32_t w = _imageInfo.width();
  uint32_t h = _imageInfo.height();
  uint32_t colorType = _png.colorType;

  // Palette & ColorKey.
  uint32_t pal[256];
  uint32_t palSize = 0;

  bool hasColorKey = false;
  Argb64 colorKey(uint64_t(0));

  // --------------------------------------------------------------------------
  // [Decode Chunks]
  // --------------------------------------------------------------------------

  uint32_t i;
  uint32_t chunkTags = _png.chunkTags;

  size_t idatOff = 0; // First IDAT chunk offset.
  size_t idatSize = 0; // Size of all IDAT chunks' data.

  for (;;) {
    // Chunk type, size, and CRC require 12 bytes.
    if ((size_t)(pEnd - p) < 12)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    uint32_t chunkTag = Support::readU32uBE(p + 4);
    uint32_t chunkSize = Support::readU32uBE(p + 0);

    // Make sure that we have data of the whole chunk.
    if ((size_t)(pEnd - p) - 12 < chunkSize)
      return setError(DebugUtils::errored(kErrorCodecIncompleteData));

    // Advance tag+size.
    p += 8;

    // ------------------------------------------------------------------------
    // [IHDR - Once]
    // ------------------------------------------------------------------------

    if (chunkTag == kPngTag_IHDR) {
      // Multiple IHDR chunks are not allowed.
      return setError(DebugUtils::errored(kErrorPngDuplicatedIHDR));
    }

    // ------------------------------------------------------------------------
    // [PLTE - Once]
    // ------------------------------------------------------------------------

    else if (chunkTag == kPngTag_PLTE) {
      // 1. There must not be more than one PLTE chunk.
      // 2. It must precede the first IDAT chunk (also tRNS chunk).
      // 3. Contains 1...256 RGB palette entries.
      if ((chunkTags & (kPngTagMask_PLTE | kPngTagMask_tRNS | kPngTagMask_IDAT)) != 0)
        return setError(DebugUtils::errored(kErrorPngInvalidPLTE));

      if (chunkSize == 0 || chunkSize > 768 || (chunkSize % 3) != 0)
        return setError(DebugUtils::errored(kErrorPngInvalidPLTE));

      palSize = chunkSize / 3;
      chunkTags |= kPngTagMask_PLTE;

      for (i = 0; i < palSize; i++, p += 3)
        pal[i] = ArgbUtil::pack32(0xFF, p[0], p[1], p[2]);

      for (; i < 256; i++)
        pal[i] = ArgbUtil::pack32(0xFF, 0x00, 0x00, 0x00);
    }

    // ------------------------------------------------------------------------
    // [tRNS - Once]
    // ------------------------------------------------------------------------

    else if (chunkTag == kPngTag_tRNS) {
      // 1. There must not be more than one tRNS chunk.
      // 2. It must precede the first IDAT chunk, follow PLTE chunk, if any.
      // 3. It is prohibited for color types 4 and 6.
      if ((chunkTags & (kPngTagMask_tRNS | kPngTagMask_IDAT)) != 0)
        return setError(DebugUtils::errored(kErrorPngInvalidTRNS));

      if (colorType == kPngColorType4_LumA || colorType == kPngColorType6_RGBA)
        return setError(DebugUtils::errored(kErrorPngInvalidTRNS));

      // For color type 0 (grayscale), the tRNS chunk contains a single gray
      // level value, stored in the format:
      //   [0..1] Gray:  2 bytes, range 0 .. (2^depth)-1
      if (colorType == kPngColorType0_Lum) {
        if (chunkSize != 2)
          return setError(DebugUtils::errored(kErrorPngInvalidTRNS));

        uint32_t gray = Support::readU16uBE(p);

        colorKey.setValue(0, gray, gray, gray);
        hasColorKey = true;

        p += 2;
      }
      // For color type 2 (truecolor), the tRNS chunk contains a single RGB
      // color value, stored in the format:
      //   [0..1] Red:   2 bytes, range 0 .. (2^depth)-1
      //   [2..3] Green: 2 bytes, range 0 .. (2^depth)-1
      //   [4..5] Blue:  2 bytes, range 0 .. (2^depth)-1
      else if (colorType == kPngColorType2_RGB) {
        if (chunkSize != 6)
          return setError(DebugUtils::errored(kErrorPngInvalidTRNS));

        uint32_t r = Support::readU16uBE(p + 0);
        uint32_t g = Support::readU16uBE(p + 2);
        uint32_t b = Support::readU16uBE(p + 4);

        colorKey.setValue(0, r, g, b);
        hasColorKey = true;

        p += 6;
      }
      // For color type 3 (indexed color), the tRNS chunk contains a series
      // of one-byte alpha values, corresponding to entries in the PLTE chunk.
      else {
        B2D_ASSERT(colorType == kPngColorType3_Pal);
        // 1. Has to follow PLTE if color type is 3.
        // 2. The tRNS chunk can contain 1...palSize alpha values, but in
        //    general it can contain less than `palSize` values, in that case
        //    the remaining alpha values are assumed to be 255.
        if ((chunkTags & kPngTagMask_PLTE) == 0 || chunkSize == 0 || chunkSize > palSize)
          return setError(DebugUtils::errored(kErrorPngInvalidTRNS));

        for (i = 0; i < chunkSize; i++, p++) {
          // Premultiply now so we don't have to worry about it later.
          pal[i] = PixelUtils::prgb32_8888_from_argb32_8888(pal[i], p[i]);
        }
      }

      chunkTags |= kPngTagMask_tRNS;
    }

    // ------------------------------------------------------------------------
    // [IDAT - Many]
    // ------------------------------------------------------------------------

    else if (chunkTag == kPngTag_IDAT) {
      if (idatOff == 0) {
        idatOff = (size_t)(p - pBegin) - 8;
        chunkTags |= kPngTagMask_IDAT;

        Support::OverflowFlag of;
        idatSize = Support::safeAdd<size_t>(idatSize, chunkSize, &of);

        if (B2D_UNLIKELY(of))
          return setError(DebugUtils::errored(kErrorPngInvalidIDAT));
      }

      p += chunkSize;
    }

    // ------------------------------------------------------------------------
    // [IEND - Once]
    // ------------------------------------------------------------------------

    else if (chunkTag == kPngTag_IEND) {
      if (chunkSize != 0 || idatOff == 0)
        return setError(DebugUtils::errored(kErrorPngInvalidIEND));

      // Skip the CRC and break.
      p += 4;
      break;
    }

    // ------------------------------------------------------------------------
    // [Unrecognized]
    // ------------------------------------------------------------------------

    else {
      p += chunkSize;
    }

    // Skip chunk's CRC.
    p += 4;
  }

  // --------------------------------------------------------------------------
  // [Decode]
  // --------------------------------------------------------------------------

  // If we reached this point it means that the image is most probably valid.
  // The index of the first IDAT chunk is stored in `idatOff` and should
  // be non-zero.
  B2D_ASSERT(idatOff != 0);

  _bufferIndex = (size_t)(p - pBegin);
  _png.chunkTags = chunkTags;

  uint32_t pixelFormat = PixelFormat::kPRGB32;
  uint32_t sampleDepth = _png.sampleDepth;
  uint32_t sampleCount = _png.sampleCount;

  uint32_t progressive = _imageInfo.isProgressive();
  uint32_t numSteps = progressive ? 7 : 1;

  PngInterlaceStep steps[7];
  uint32_t outputSize = bPngCalculateInterlaceSteps(steps,
    progressive ? bPngInterlaceAdam7 : bPngInterlaceNone,
    numSteps, sampleDepth, sampleCount, w, h);

  if (outputSize == 0)
    return setError(DebugUtils::errored(kErrorCodecInvalidData));

  Buffer output;

  Error err = output.reserve(outputSize);
  if (err) return setError(err);

  PngDecoderReadData rd;
  rd.p = pBegin;
  rd.index = idatOff;

  err = Deflate::deflate(output, &rd, (Deflate::ReadFunc)bPngDecoderReadFunc, !_png.brokenByApple);
  if (err) return setError(err);

  uint8_t* data = const_cast<uint8_t*>(output.data());
  uint32_t bytesPerPixel = Math::max<uint32_t>((sampleDepth * sampleCount) / 8, 1);

  // If progressive `numSteps` is 7 and `steps` contains all windows.
  for (i = 0; i < numSteps; i++) {
    PngInterlaceStep& step = steps[i];

    if (!step.used)
      continue;

    err = _bPngOps.inverseFilter(data + step.offset, bytesPerPixel, step.bpl, step.height);
    if (err) return setError(err);
  }

  // --------------------------------------------------------------------------
  // [Convert / Deinterlace]
  // --------------------------------------------------------------------------

  err = dst.create(w, h, pixelFormat);
  if (err) return setError(err);

  ImageBuffer pixels;

  err = dst.lock(pixels, this);
  if (err) return setError(err);

  uint8_t* dstPixels = pixels.pixelData();
  intptr_t dstStride = pixels.stride();

  PixelConverter cvt;
  uint32_t masks[4];

  if (colorType == kPngColorType0_Lum && sampleDepth <= 8) {
    // Treat grayscale images up to 8bpp as indexed and create a dummy palette.
    bPngCreateGrayscalePalette(pal, sampleDepth);

    // Handle color-key properly.
    if (hasColorKey && colorKey.r() < (1u << sampleDepth))
      pal[colorKey.r()] = 0x00000000;

    cvt.initImport(pixelFormat, sampleDepth | PixelConverter::kLayoutIsIndexed, pal);
  }
  else if (colorType == kPngColorType3_Pal) {
    cvt.initImport(pixelFormat, sampleDepth | PixelConverter::kLayoutIsIndexed, pal);
  }
  else {
    uint32_t layout = (sampleDepth * sampleCount) | PixelConverter::kLayoutBE;

    if (colorType == kPngColorType0_Lum) {
      // TODO: 16-BPC.
    }
    else if (colorType == kPngColorType2_RGB) {
      masks[0] = 0;
      masks[1] = 0x00FF0000u;
      masks[2] = 0x0000FF00u;
      masks[3] = 0x000000FFu;
    }
    else if (colorType == kPngColorType4_LumA) {
      masks[0] = 0x000000FFu;
      masks[1] = 0x0000FF00u;
      masks[2] = 0x0000FF00u;
      masks[3] = 0x0000FF00u;
    }
    else if (colorType == kPngColorType6_RGBA) {
      masks[0] = 0x000000FFu;
      masks[1] = 0xFF000000u;
      masks[2] = 0x00FF0000u;
      masks[3] = 0x0000FF00u;
    }

    if (_png.brokenByApple) {
      std::swap(masks[1], masks[3]);
      layout |= PixelConverter::kLayoutIsPremultiplied;
    }

    cvt.initImport(pixelFormat, layout, masks);
  }

  if (progressive) {
    // PNG interlacing requires 7 steps, where 7th handles all even scanlines
    // (indexing from 1). This means that we can, in general, reuse the buffer
    // required by 7th step as a temporary to merge steps 1-6. To achieve this,
    // we need to:
    //
    //   1. Convert all even scanlines already ready by 7th step to `dst`. This
    //      makes the buffer ready to be reused.
    //   2. Merge pixels from steps 1-6 into that buffer.
    //   3. Convert all odd scanlines (from the reused buffer) to `dst`.
    //
    // We, in general, process 4 odd scanlines at time, so we need the 7th
    // buffer to have enough space to hold them as well, if not, we allocate
    // an extra buffer and use it instead. This approach is good as small
    // images would probably require an extra buffer, but larger images can
    // reuse the 7th.
    B2D_ASSERT(steps[6].width == w);
    B2D_ASSERT(steps[6].height == h / 2); // Half of the rows, rounded down.

    uint32_t depth = sampleDepth * sampleCount;
    uint32_t tmpHeight = Math::min<uint32_t>((h + 1) / 2, 4);
    uint32_t tmpBpl = steps[6].bpl;
    uint32_t tmpSize;

    if (steps[6].height)
      cvt.convertRect(dstPixels + dstStride, dstStride * 2, data + 1 + steps[6].offset, tmpBpl, w, steps[6].height);

    // Align `tmpBpl` so we can use aligned memory writes and reads while using it.
    tmpBpl = Support::alignUp(tmpBpl, 16);
    tmpSize = tmpBpl * tmpHeight;

    MemBuffer tmpAlloc;
    uint8_t* tmp;

    // Decide whether to alloc an extra buffer of to reuse 7th.
    if (steps[6].size < tmpSize + 15)
      tmp = static_cast<uint8_t*>(tmpAlloc.alloc(tmpSize + 15));
    else
      tmp = data + steps[6].offset;

    if (tmp) {
      tmp = Support::alignUp(tmp, 16);
      switch (depth) {
        case 1 : bPngDeinterlaceBits <1>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 2 : bPngDeinterlaceBits <2>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 4 : bPngDeinterlaceBits <4>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 8 : bPngDeinterlaceBytes<1>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 16: bPngDeinterlaceBytes<2>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 24: bPngDeinterlaceBytes<3>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
        case 32: bPngDeinterlaceBytes<4>(dstPixels, dstStride, cvt, tmp, tmpBpl, data, steps, w, h); break;
      }
    }
    else {
      err = DebugUtils::errored(kErrorNoMemory);
    }
  }
  else {
    B2D_ASSERT(steps[0].width == w);
    B2D_ASSERT(steps[0].height == h);

    cvt.convertRect(dstPixels, dstStride, data + 1, steps[0].bpl, w, h);
  }

  // --------------------------------------------------------------------------
  // [End]
  // --------------------------------------------------------------------------

  dst.unlock(pixels, this);
  return err;
}

// ============================================================================
// [b2d::PngEncoderImpl - Construction / Destruction]
// ============================================================================

PngEncoderImpl::PngEncoderImpl() noexcept {}
PngEncoderImpl::~PngEncoderImpl() noexcept {}

// ============================================================================
// [b2d::PngEncoderImpl - Interface]
// ============================================================================

Error PngEncoderImpl::reset() noexcept {
  Base::reset();
  return kErrorOk;
}

Error PngEncoderImpl::encode(Buffer& dst, const Image& src) noexcept {
  // TODO: PNG-Encoder.
  return DebugUtils::errored(kErrorNotImplemented);
}

// ============================================================================
// [b2d::PngCodecImpl - Construction / Destruction]
// ============================================================================

PngCodecImpl::PngCodecImpl() noexcept {
  _features = ImageCodec::kFeatureDecoder  |
              ImageCodec::kFeatureEncoder  |
              ImageCodec::kFeatureLossless ;

  _formats  = ImageCodec::kFormatIndexed1  |
              ImageCodec::kFormatIndexed2  |
              ImageCodec::kFormatIndexed4  |
              ImageCodec::kFormatIndexed8  |
              ImageCodec::kFormatGray1     |
              ImageCodec::kFormatGray2     |
              ImageCodec::kFormatGray4     |
              ImageCodec::kFormatGray8     |
              ImageCodec::kFormatGray16    |
              ImageCodec::kFormatRGB16     |
              ImageCodec::kFormatRGB24     |
              ImageCodec::kFormatRGB48     |
              ImageCodec::kFormatARGB32    |
              ImageCodec::kFormatARGB64    ;

  setupStrings("B2D\0" "PNG\0" "image/png\0" "png\0");
}

PngCodecImpl::~PngCodecImpl() noexcept {}

// ============================================================================
// [b2d::PngCodecImpl - Interface]
// ============================================================================

int PngCodecImpl::inspect(const uint8_t* data, size_t size) const noexcept {
  // PNG minimum size and signature.
  if (size < 8 || std::memcmp(data, bPngSignature, 8) != 0) return 0;

  // Don't return 100, maybe there are other codecs with a higher precedence.
  return 95;
}

ImageDecoderImpl* PngCodecImpl::createDecoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<PngDecoderImpl>(), ImageDecoder::none().impl()); }
ImageEncoderImpl* PngCodecImpl::createEncoderImpl() const noexcept { return AnyInternal::fallback(AnyInternal::newImplT<PngEncoderImpl>(), ImageEncoder::none().impl()); }

// ============================================================================
// [b2d::Png - Runtime Handlers]
// ============================================================================

void ImageCodecOnInitPng(Array<ImageCodec>* codecList) noexcept {
  PngOps& ops = _bPngOps;

  #if B2D_ARCH_SSE2
  ops.inverseFilter = PngInternal::inverseFilterSSE2;
  #else
  ops.inverseFilter = PngInternal::inverseFilterRef;
  #endif

  codecList->append(ImageCodec(AnyInternal::newImplT<PngCodecImpl>()));
}

B2D_END_NAMESPACE
