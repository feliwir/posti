// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../codec/pngcodec_p.h"
#include "../core/simd.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

namespace PngInternal {

// ============================================================================
// [b2d::PngInternal - InverseFilter (SSE2)]
// ============================================================================

#define B2D_PNG_SLL_ADDB_1X(P0, T0, Shift) \
  do { \
    T0 = SIMD::vslli128b<Shift>(P0); \
    P0 = SIMD::vaddi8(P0, T0); \
  } while (0)

#define B2D_PNG_SLL_ADDB_2X(P0, T0, P1, T1, Shift) \
  do { \
    T0 = SIMD::vslli128b<Shift>(P0); \
    T1 = SIMD::vslli128b<Shift>(P1); \
    P0 = SIMD::vaddi8(P0, T0); \
    P1 = SIMD::vaddi8(P1, T1); \
  } while (0)

#define B2D_PNG_PAETH(Dst, A, B, C) \
  do { \
    SIMD::I128 MinAB = SIMD::vmini16(A, B); \
    SIMD::I128 MaxAB = SIMD::vmaxi16(A, B); \
    SIMD::I128 DivAB = SIMD::vmulhu16(SIMD::vsubi16(MaxAB, MinAB), rcp3); \
    \
    MinAB = SIMD::vsubi16(MinAB, C); \
    MaxAB = SIMD::vsubi16(MaxAB, C); \
    \
    Dst = SIMD::vaddi16(C  , SIMD::vnand(SIMD::vsrai16<15>(SIMD::vaddi16(DivAB, MinAB)), MaxAB)); \
    Dst = SIMD::vaddi16(Dst, SIMD::vnand(SIMD::vsrai16<15>(SIMD::vsubi16(DivAB, MaxAB)), MinAB)); \
  } while (0)

Error inverseFilterSSE2(uint8_t* p, uint32_t bpp, uint32_t bpl, uint32_t h) noexcept {
  B2D_ASSERT(bpp > 0);
  B2D_ASSERT(bpl > 1);
  B2D_ASSERT(h   > 0);

  uint32_t y = h;
  uint8_t* u = nullptr;

  // Subtract one BYTE that is used to store the `filter` ID.
  bpl--;

  // First row uses a special filter that doesn't access the previous row,
  // which is assumed to contain all zeros.
  uint32_t filterType = *p++;
  if (B2D_UNLIKELY(filterType >= kPngFilterCount))
    return DebugUtils::errored(kErrorPngInvalidFilter);
  filterType = PngUtils::firstRowFilterReplacement(filterType);

  for (;;) {
    uint32_t i;

    switch (filterType) {
      // ----------------------------------------------------------------------
      // [None]
      // ----------------------------------------------------------------------

      case kPngFilterNone:
        p += bpl;
        break;

      // ----------------------------------------------------------------------
      // [Sub]
      // ----------------------------------------------------------------------

      // This is one of the easiest filters to parallelize. Although it looks
      // like the data dependency is too high, it's simply additions, which are
      // really easy to parallelize. The following formula:
      //
      //     Y1' = BYTE(Y1 + Y0')
      //     Y2' = BYTE(Y2 + Y1')
      //     Y3' = BYTE(Y3 + Y2')
      //     Y4' = BYTE(Y4 + Y3')
      //
      // Expanded to (with byte casts removed, as they are implicit in our case):
      //
      //     Y1' = Y1 + Y0'
      //     Y2' = Y2 + Y1 + Y0'
      //     Y3' = Y3 + Y2 + Y1 + Y0'
      //     Y4' = Y4 + Y3 + Y2 + Y1 + Y0'
      //
      // Can be implemented like this by taking advantage of SIMD:
      //
      //     +-----------+-----------+-----------+-----------+----->
      //     |    Y1     |    Y2     |    Y3     |    Y4     | ...
      //     +-----------+-----------+-----------+-----------+----->
      //                   Shift by 1 and PADDB
      //     +-----------+-----------+-----------+-----------+
      //     |           |    Y1     |    Y2     |    Y3     | ----+
      //     +-----------+-----------+-----------+-----------+     |
      //                                                           |
      //     +-----------+-----------+-----------+-----------+     |
      //     |    Y1     |   Y1+Y2   |   Y2+Y3   |   Y3+Y4   | <---+
      //     +-----------+-----------+-----------+-----------+
      //                   Shift by 2 and PADDB
      //     +-----------+-----------+-----------+-----------+
      //     |           |           |    Y1     |   Y1+Y2   | ----+
      //     +-----------+-----------+-----------+-----------+     |
      //                                                           |
      //     +-----------+-----------+-----------+-----------+     |
      //     |    Y1     |   Y1+Y2   | Y1+Y2+Y3  |Y1+Y2+Y3+Y4| <---+
      //     +-----------+-----------+-----------+-----------+
      //
      // The size of the register doesn't matter here. The Y0' dependency has
      // been omitted to make the flow cleaner, however, it can be added to Y1
      // before processing or it can be shifted to the first cell so the first
      // addition would be performed against [Y0', Y1, Y2, Y3].

      case kPngFilterSub: {
        i = bpl - bpp;

        if (i >= 32) {
          // Align to 16-BYTE boundary.
          uint32_t j = uint32_t(Support::alignUpDiff(p + bpp, 16));
          for (i -= j; j != 0; j--, p++)
            p[bpp] = PngUtils::sum(p[bpp], p[0]);

          if (bpp == 1) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t2;

            // Process 64 BYTEs at a time.
            p0 = SIMD::vcvtu32i128(p[0]);
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 1));
              p1 = SIMD::vloadi128a(p + 17);
              p2 = SIMD::vloadi128a(p + 33);
              p3 = SIMD::vloadi128a(p + 49);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 1);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 2);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 4);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 8);
              SIMD::vstorei128a(p + 1, p0);

              p0 = SIMD::vsrli128b<15>(p0);
              t2 = SIMD::vsrli128b<15>(p2);
              p1 = SIMD::vaddi8(p1, p0);
              p3 = SIMD::vaddi8(p3, t2);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 1);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 2);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 4);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 8);
              SIMD::vstorei128a(p + 17, p1);

              p1 = SIMD::vunpackhi8(p1, p1);
              p1 = SIMD::vunpackhi16(p1, p1);
              p1 = SIMD::vswizi32<3, 3, 3, 3>(p1);

              p2 = SIMD::vaddi8(p2, p1);
              p3 = SIMD::vaddi8(p3, p1);

              SIMD::vstorei128a(p + 33, p2);
              SIMD::vstorei128a(p + 49, p3);
              p0 = SIMD::vsrli128b<15>(p3);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 1));

              B2D_PNG_SLL_ADDB_1X(p0, t0, 1);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 2);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 4);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 8);

              SIMD::vstorei128a(p + 1, p0);
              p0 = SIMD::vsrli128b<15>(p0);

              p += 16;
              i -= 16;
            }
          }
          else if (bpp == 2) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t2;

            // Process 64 BYTEs at a time.
            p0 = SIMD::vcvtu32i128(Support::readU16a(p));
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 2));
              p1 = SIMD::vloadi128a(p + 18);
              p2 = SIMD::vloadi128a(p + 34);
              p3 = SIMD::vloadi128a(p + 50);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 2);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 4);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 8);
              SIMD::vstorei128a(p + 2, p0);

              p0 = SIMD::vsrli128b<14>(p0);
              t2 = SIMD::vsrli128b<14>(p2);
              p1 = SIMD::vaddi8(p1, p0);
              p3 = SIMD::vaddi8(p3, t2);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 2);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 4);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 8);
              SIMD::vstorei128a(p + 18, p1);

              p1 = SIMD::vunpackhi16(p1, p1);
              p1 = SIMD::vswizi32<3, 3, 3, 3>(p1);

              p2 = SIMD::vaddi8(p2, p1);
              p3 = SIMD::vaddi8(p3, p1);

              SIMD::vstorei128a(p + 34, p2);
              SIMD::vstorei128a(p + 50, p3);
              p0 = SIMD::vsrli128b<14>(p3);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 2));
              B2D_PNG_SLL_ADDB_1X(p0, t0, 2);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 4);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 8);

              SIMD::vstorei128a(p + 2, p0);
              p0 = SIMD::vsrli128b<14>(p0);

              p += 16;
              i -= 16;
            }
          }
          else if (bpp == 3) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t2;
            SIMD::I128 ext3b = SIMD::vseti128i32(0x01000001);

            // Process 64 BYTEs at a time.
            p0 = SIMD::vcvtu32i128(Support::readU32u(p) & 0x00FFFFFFu);
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 3));
              p1 = SIMD::vloadi128a(p + 19);
              p2 = SIMD::vloadi128a(p + 35);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 3);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 6);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t2, 12);

              p3 = SIMD::vloadi128a(p + 51);
              t0 = SIMD::vsrli128b<13>(p0);
              t2 = SIMD::vsrli128b<13>(p2);

              p1 = SIMD::vaddi8(p1, t0);
              p3 = SIMD::vaddi8(p3, t2);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 3);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 6);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t2, 12);
              SIMD::vstorei128a(p + 3, p0);

              p0 = SIMD::vswizi32<3, 3, 3, 3>(p1);
              p0 = SIMD::vsrli32<8>(p0);
              p0 = _mm_mul_epu32(p0, ext3b);

              p0 = SIMD::vswizli16<0, 2, 1, 0>(p0);
              p0 = SIMD::vswizhi16<1, 0, 2, 1>(p0);

              SIMD::vstorei128a(p + 19, p1);
              p2 = SIMD::vaddi8(p2, p0);
              p0 = SIMD::vswizi32<1, 3, 2, 1>(p0);

              SIMD::vstorei128a(p + 35, p2);
              p0 = SIMD::vaddi8(p0, p3);

              SIMD::vstorei128a(p + 51, p0);
              p0 = SIMD::vsrli128b<13>(p0);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 3));

              B2D_PNG_SLL_ADDB_1X(p0, t0, 3);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 6);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 12);

              SIMD::vstorei128a(p + 3, p0);
              p0 = SIMD::vsrli128b<13>(p0);

              p += 16;
              i -= 16;
            }
          }
          else if (bpp == 4) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t1, t2;

            // Process 64 BYTEs at a time.
            p0 = SIMD::vcvtu32i128(Support::readU32a(p));
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 4));
              p1 = SIMD::vloadi128a(p + 20);
              p2 = SIMD::vloadi128a(p + 36);
              p3 = SIMD::vloadi128a(p + 52);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t1, 4);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t1, 8);
              SIMD::vstorei128a(p + 4, p0);

              p0 = SIMD::vsrli128b<12>(p0);
              t2 = SIMD::vsrli128b<12>(p2);

              p1 = SIMD::vaddi8(p1, p0);
              p3 = SIMD::vaddi8(p3, t2);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t1, 4);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t1, 8);

              p0 = SIMD::vswizi32<3, 3, 3, 3>(p1);
              SIMD::vstorei128a(p + 20, p1);

              p2 = SIMD::vaddi8(p2, p0);
              p0 = SIMD::vaddi8(p0, p3);

              SIMD::vstorei128a(p + 36, p2);
              SIMD::vstorei128a(p + 52, p0);
              p0 = SIMD::vsrli128b<12>(p0);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 4));

              B2D_PNG_SLL_ADDB_1X(p0, t0, 4);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 8);
              SIMD::vstorei128a(p + 4, p0);
              p0 = SIMD::vsrli128b<12>(p0);

              p += 16;
              i -= 16;
            }
          }
          else if (bpp == 6) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t1;

            p0 = SIMD::vloadi128_64(p);
            p0 = SIMD::vslli64<16>(p0);
            p0 = SIMD::vsrli64<16>(p0);

            // Process 64 BYTEs at a time.
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 6));
              p1 = SIMD::vloadi128a(p + 22);
              p2 = SIMD::vloadi128a(p + 38);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t1, 6);
              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t1, 12);

              p3 = SIMD::vloadi128a(p + 54);
              SIMD::vstorei128a(p + 6, p0);

              p0 = SIMD::vsrli128b<10>(p0);
              t1 = SIMD::vsrli128b<10>(p2);

              p1 = SIMD::vaddi8(p1, p0);
              p3 = SIMD::vaddi8(p3, t1);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t1, 6);
              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t1, 12);
              p0 = SIMD::vduphi64(p1);

              p0 = SIMD::vswizli16<1, 3, 2, 1>(p0);
              p0 = SIMD::vswizhi16<2, 1, 3, 2>(p0);

              SIMD::vstorei128a(p + 22, p1);
              p2 = SIMD::vaddi8(p2, p0);
              p0 = SIMD::vswizi32<1, 3, 2, 1>(p0);

              SIMD::vstorei128a(p + 38, p2);
              p0 = SIMD::vaddi8(p0, p3);

              SIMD::vstorei128a(p + 54, p0);
              p0 = SIMD::vsrli128b<10>(p0);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 6));

              B2D_PNG_SLL_ADDB_1X(p0, t0, 6);
              B2D_PNG_SLL_ADDB_1X(p0, t0, 12);

              SIMD::vstorei128a(p + 6, p0);
              p0 = SIMD::vsrli128b<10>(p0);

              p += 16;
              i -= 16;
            }
          }
          else if (bpp == 8) {
            SIMD::I128 p0, p1, p2, p3;
            SIMD::I128 t0, t1, t2;

            // Process 64 BYTEs at a time.
            p0 = SIMD::vloadi128_64(p);
            while (i >= 64) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 8));
              p1 = SIMD::vloadi128a(p + 24);
              p2 = SIMD::vloadi128a(p + 40);
              p3 = SIMD::vloadi128a(p + 56);

              B2D_PNG_SLL_ADDB_2X(p0, t0, p2, t1, 8);
              SIMD::vstorei128a(p + 8, p0);

              p0 = SIMD::vsrli128b<8>(p0);
              t2 = SIMD::vduphi64(p2);
              p1 = SIMD::vaddi8(p1, p0);

              B2D_PNG_SLL_ADDB_2X(p1, t0, p3, t1, 8);
              p0 = SIMD::vduphi64(p1);
              p3 = SIMD::vaddi8(p3, t2);
              SIMD::vstorei128a(p + 24, p1);

              p2 = SIMD::vaddi8(p2, p0);
              p0 = SIMD::vaddi8(p0, p3);

              SIMD::vstorei128a(p + 40, p2);
              SIMD::vstorei128a(p + 56, p0);
              p0 = SIMD::vsrli128b<8>(p0);

              p += 64;
              i -= 64;
            }

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              p0 = SIMD::vaddi8(p0, *reinterpret_cast<SIMD::I128*>(p + 8));
              B2D_PNG_SLL_ADDB_1X(p0, t0, 8);

              SIMD::vstorei128a(p + 8, p0);
              p0 = SIMD::vsrli128b<8>(p0);

              p += 16;
              i -= 16;
            }
          }
        }

        for (; i != 0; i--, p++)
          p[bpp] = PngUtils::sum(p[bpp], p[0]);

        p += bpp;
        break;
      }

      // ----------------------------------------------------------------------
      // [Up]
      // ----------------------------------------------------------------------

      // This is actually the easiest filter that doesn't require any kind of
      // specialization for a particular BPP. Even C++ compiler like GCC is
      // able to parallelize a naive implementation. However, MSC compiler does
      // not parallelize the naive implementation so the SSE2 implementation
      // provided greatly boosted the performance on Windows.
      //
      //     +-----------+-----------+-----------+-----------+
      //     |    Y1     |    Y2     |    Y3     |    Y4     |
      //     +-----------+-----------+-----------+-----------+
      //                           PADDB
      //     +-----------+-----------+-----------+-----------+
      //     |    U1     |    U2     |    U3     |    U4     | ----+
      //     +-----------+-----------+-----------+-----------+     |
      //                                                           |
      //     +-----------+-----------+-----------+-----------+     |
      //     |   Y1+U1   |   Y2+U2   |   Y3+U3   |   Y4+U4   | <---+
      //     +-----------+-----------+-----------+-----------+

      case kPngFilterUp: {
        i = bpl;

        if (i >= 24) {
          // Align to 16-BYTE boundary.
          uint32_t j = uint32_t(Support::alignUpDiff(p, 16));
          for (i -= j; j != 0; j--, p++, u++)
            p[0] = PngUtils::sum(p[0], u[0]);

          // Process 64 BYTEs at a time.
          while (i >= 64) {
            SIMD::I128 u0 = SIMD::vloadi128u(u);
            SIMD::I128 u1 = SIMD::vloadi128u(u + 16);

            SIMD::I128 p0 = SIMD::vloadi128a(p);
            SIMD::I128 p1 = SIMD::vloadi128a(p + 16);

            SIMD::I128 u2 = SIMD::vloadi128u(u + 32);
            SIMD::I128 u3 = SIMD::vloadi128u(u + 48);

            p0 = SIMD::vaddi8(p0, u0);
            p1 = SIMD::vaddi8(p1, u1);

            SIMD::I128 p2 = SIMD::vloadi128a(p + 32);
            SIMD::I128 p3 = SIMD::vloadi128a(p + 48);

            p2 = SIMD::vaddi8(p2, u2);
            p3 = SIMD::vaddi8(p3, u3);

            SIMD::vstorei128a(p     , p0);
            SIMD::vstorei128a(p + 16, p1);
            SIMD::vstorei128a(p + 32, p2);
            SIMD::vstorei128a(p + 48, p3);

            p += 64;
            u += 64;
            i -= 64;
          }

          // Process 8 BYTEs at a time.
          while (i >= 8) {
            SIMD::I128 u0 = SIMD::vloadi128_64(u);
            SIMD::I128 p0 = SIMD::vloadi128_64(p);

            p0 = SIMD::vaddi8(p0, u0);
            SIMD::vstorei64(p, p0);

            p += 8;
            u += 8;
            i -= 8;
          }
        }

        for (; i != 0; i--, p++, u++)
          p[0] = PngUtils::sum(p[0], u[0]);
        break;
      }

      // ----------------------------------------------------------------------
      // [Avg]
      // ----------------------------------------------------------------------

      // This filter is extremely difficult for low BPP values as there is
      // a huge sequential data dependency, I didn't succeeded to solve it.
      // 1-3 BPP implementations are pretty bad and I would like to hear about
      // a way to improve those. The implementation for 4 BPP and more is
      // pretty good, as these is less data dependency between individual bytes.
      //
      // Sequental Approach:
      //
      //     Y1' = byte((2*Y1 + U1 + Y0') >> 1)
      //     Y2' = byte((2*Y2 + U2 + Y1') >> 1)
      //     Y3' = byte((2*Y3 + U3 + Y2') >> 1)
      //     Y4' = byte((2*Y4 + U4 + Y3') >> 1)
      //     Y5' = ...
      //
      // Expanded, `U1 + Y0'` replaced with `U1`:
      //
      //     Y1' = byte((2*Y1 + U1) >> 1)
      //     Y2' = byte((2*Y2 + U2 +
      //           byte((2*Y1 + U1) >> 1)) >> 1)
      //     Y3' = byte((2*Y3 + U3 +
      //           byte((2*Y2 + U2 +
      //           byte((2*Y1 + U1) >> 1)) >> 1)) >> 1)
      //     Y4' = byte((2*Y4 + U4 +
      //           byte((2*Y3 + U3 +
      //           byte((2*Y2 + U2 +
      //           byte((2*Y1 + U1) >> 1)) >> 1)) >> 1)) >> 1)
      //     Y5' = ...

      case kPngFilterAvg: {
        for (i = 0; i < bpp; i++)
          p[i] = PngUtils::sum(p[i], u[i] >> 1);

        i = bpl - bpp;
        u += bpp;

        if (i >= 32) {
          // Align to 16-BYTE boundary.
          uint32_t j = uint32_t(Support::alignUpDiff(p + bpp, 16));
          SIMD::I128 zero = SIMD::vzeroi128();

          for (i -= j; j != 0; j--, p++, u++)
            p[bpp] = PngUtils::sum(p[bpp], PngUtils::average(p[0], u[0]));

          if (bpp == 1) {
            // This is one of the most difficult AVG filters. 1-BPP has a huge
            // sequential dependency, which is nearly impossible to parallelize.
            // The code below is the best I could have written, it's a mixture
            // of C++ and SIMD. Maybe using a pure C would be even better than
            // this code, but, I tried to take advantage of 8 BYTE fetches at
            // least. Unrolling the loop any further doesn't lead to an
            // improvement.
            //
            // I know that the code looks terrible, but it's a bit faster than
            // a pure specialized C++ implementation I used to have before.
            uint32_t t0 = p[0];
            uint32_t t1;

            // Process 8 BYTEs at a time.
            while (i >= 8) {
              SIMD::I128 p0 = SIMD::vloadi128_64(p + 1);
              SIMD::I128 u0 = SIMD::vloadi128_64(u);

              p0 = SIMD::vunpackli8(p0, zero);
              u0 = SIMD::vunpackli8(u0, zero);

              p0 = SIMD::vslli16<1>(p0);
              p0 = SIMD::vaddi16(p0, u0);

              t1 = SIMD::vcvti128u32(p0);
              p0 = SIMD::vsrli128b<4>(p0);
              t0 = ((t0 + t1) >> 1) & 0xFF; t1 >>= 16;
              p[1] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF;
              t1 = SIMD::vcvti128u32(p0);
              p0 = SIMD::vsrli128b<4>(p0);
              p[2] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF; t1 >>= 16;
              p[3] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF;
              t1 = SIMD::vcvti128u32(p0);
              p0 = SIMD::vsrli128b<4>(p0);
              p[4] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF; t1 >>= 16;
              p[5] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF;
              t1 = SIMD::vcvti128u32(p0);
              p[6] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF; t1 >>= 16;
              p[7] = uint8_t(t0);

              t0 = ((t0 + t1) >> 1) & 0xFF;
              p[8] = uint8_t(t0);

              p += 8;
              u += 8;
              i -= 8;
            }
          }
          // TODO: Not complete / Not working.
          /*
          else if (bpp == 2) {
          }
          else if (bpp == 3) {
          }
          */
          else if (bpp == 4) {
            SIMD::I128 m00FF = SIMD::vseti128i32(0x00FF00FF);
            SIMD::I128 m01FF = SIMD::vseti128i32(0x01FF01FF);
            SIMD::I128 t1 = SIMD::vunpackli8(SIMD::vcvtu32i128(Support::readU32a(p)), zero);

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              SIMD::I128 p0, p1;
              SIMD::I128 u0, u1;

              p0 = SIMD::vloadi128a(p + 4);
              u0 = SIMD::vloadi128u(u);

              p1 = p0;                          // HI | Move Ln
              p0 = SIMD::vunpackli8(p0, zero);   // LO | Unpack Ln

              u1 = u0;                          // HI | Move Up
              p0 = SIMD::vslli16<1>(p0);          // LO | << 1

              u0 = SIMD::vunpackli8(u0, zero);   // LO | Unpack Up
              p0 = SIMD::vaddi16(p0, t1);         // LO | Add Last

              p1 = SIMD::vunpackhi8(p1, zero);   // HI | Unpack Ln
              p0 = SIMD::vaddi16(p0, u0);         // LO | Add Up
              p0 = SIMD::vand(p0, m01FF);       // LO | & 0x01FE

              u1 = SIMD::vunpackhi8(u1, zero);   // HI | Unpack Up
              t1 = SIMD::vslli128b<8>(p0);           // LO | Get Last
              p0 = SIMD::vslli16<1>(p0);          // LO | << 1

              p1 = SIMD::vslli16<1>(p1);          // HI | << 1
              p0 = SIMD::vaddi16(p0, t1);         // LO | Add Last
              p0 = SIMD::vsrli16<2>(p0);          // LO | >> 2

              p1 = SIMD::vaddi16(p1, u1);         // HI | Add Up
              p0 = SIMD::vand(p0, m00FF);       // LO | & 0x00FF
              t1 = SIMD::vsrli128b<8>(p0);           // LO | Get Last

              p1 = SIMD::vaddi16(p1, t1);         // HI | Add Last
              p1 = SIMD::vand(p1, m01FF);       // HI | & 0x01FE

              t1 = SIMD::vslli128b<8>(p1);           // HI | Get Last
              p1 = SIMD::vslli16<1>(p1);          // HI | << 1

              t1 = SIMD::vaddi16(t1, p1);         // HI | Add Last
              t1 = SIMD::vsrli16<2>(t1);          // HI | >> 2
              t1 = SIMD::vand(t1, m00FF);       // HI | & 0x00FF

              p0 = SIMD::vpackzzwb(p0, t1);
              t1 = SIMD::vsrli128b<8>(t1);           // HI | Get Last
              SIMD::vstorei128a(p + 4, p0);

              p += 16;
              u += 16;
              i -= 16;
            }
          }
          else if (bpp == 6) {
            SIMD::I128 t1 = SIMD::vloadi128_64(p);

            // Process 16 BYTEs at a time.
            while (i >= 16) {
              SIMD::I128 p0, p1, p2;
              SIMD::I128 u0, u1, u2;

              u0 = SIMD::vloadi128u(u);
              t1 = SIMD::vunpackli8(t1, zero);
              p0 = SIMD::vloadi128a(p + 6);

              p1 = SIMD::vsrli128b<6>(p0);           // P1 | Extract
              u1 = SIMD::vsrli128b<6>(u0);           // P1 | Extract

              p2 = SIMD::vsrli128b<12>(p0);          // P2 | Extract
              u2 = SIMD::vsrli128b<12>(u0);          // P2 | Extract

              p0 = SIMD::vunpackli8(p0, zero);   // P0 | Unpack
              u0 = SIMD::vunpackli8(u0, zero);   // P0 | Unpack

              p1 = SIMD::vunpackli8(p1, zero);   // P1 | Unpack
              u1 = SIMD::vunpackli8(u1, zero);   // P1 | Unpack

              p2 = SIMD::vunpackli8(p2, zero);   // P2 | Unpack
              u2 = SIMD::vunpackli8(u2, zero);   // P2 | Unpack

              u0 = SIMD::vaddi16(u0, t1);         // P0 | Add Last
              u0 = SIMD::vsrli16<1>(u0);          // P0 | >> 1
              p0 = SIMD::vaddi8(p0, u0);         // P0 | Add (Up+Last)/2

              u1 = SIMD::vaddi16(u1, p0);         // P1 | Add P0
              u1 = SIMD::vsrli16<1>(u1);          // P1 | >> 1
              p1 = SIMD::vaddi8(p1, u1);         // P1 | Add (Up+Last)/2

              u2 = SIMD::vaddi16(u2, p1);         // P2 | Add P1
              u2 = SIMD::vsrli16<1>(u2);          // P2 | >> 1
              p2 = SIMD::vaddi8(p2, u2);         // P2 | Add (Up+Last)/2

              p0 = SIMD::vslli128b<4>(p0);
              p0 = SIMD::vpackzzwb(p0, p1);
              p0 = SIMD::vslli128b<2>(p0);
              p0 = SIMD::vsrli128b<4>(p0);

              p2 = SIMD::vpackzzwb(p2, p2);
              p2 = SIMD::vslli128b<12>(p2);
              p0 = SIMD::vor(p0, p2);

              SIMD::vstorei128a(p + 6, p0);
              t1 = SIMD::vsrli128b<10>(p0);

              p += 16;
              u += 16;
              i -= 16;
            }
          }
          else if (bpp == 8) {
            // Process 16 BYTEs at a time.
            SIMD::I128 t1 = SIMD::vunpackli8(SIMD::vloadi128_64(p), zero);

            while (i >= 16) {
              SIMD::I128 p0, p1;
              SIMD::I128 u0, u1;

              u0 = SIMD::vloadi128u(u);
              p0 = SIMD::vloadi128a(p + 8);

              u1 = u0;                          // HI | Move Up
              p1 = p0;                          // HI | Move Ln
              u0 = SIMD::vunpackli8(u0, zero);   // LO | Unpack Up
              p0 = SIMD::vunpackli8(p0, zero);   // LO | Unpack Ln

              u0 = SIMD::vaddi16(u0, t1);         // LO | Add Last
              p1 = SIMD::vunpackhi8(p1, zero);   // HI | Unpack Ln
              u0 = SIMD::vsrli16<1>(u0);          // LO | >> 1
              u1 = SIMD::vunpackhi8(u1, zero);   // HI | Unpack Up

              p0 = SIMD::vaddi8(p0, u0);         // LO | Add (Up+Last)/2
              u1 = SIMD::vaddi16(u1, p0);         // HI | Add LO
              u1 = SIMD::vsrli16<1>(u1);          // HI | >> 1
              p1 = SIMD::vaddi8(p1, u1);         // HI | Add (Up+LO)/2

              p0 = SIMD::vpackzzwb(p0, p1);
              t1 = p1;                          // HI | Get Last
              SIMD::vstorei128a(p + 8, p0);

              p += 16;
              u += 16;
              i -= 16;
            }
          }
        }

        for (; i != 0; i--, p++, u++)
          p[bpp] = PngUtils::sum(p[bpp], PngUtils::average(p[0], u[0]));

        p += bpp;
        break;
      }

      // ----------------------------------------------------------------------
      // [Paeth]
      // ----------------------------------------------------------------------

      case kPngFilterPaeth: {
        if (bpp == 1) {
          // There is not much to optimize for 1BPP. The only thing this code
          // does is to keep `p0` and `u0` values from the current iteration
          // to the next one (they become `pz` and `uz`).
          uint32_t pz = 0;
          uint32_t uz = 0;
          uint32_t u0;

          for (i = 0; i < bpl; i++) {
            u0 = u[i];
            pz = (uint32_t(p[i]) + PngUtils::paeth(pz, u0, uz)) & 0xFF;

            p[i] = uint8_t(pz);
            uz = u0;
          }

          p += bpl;
        }
        else {
          for (i = 0; i < bpp; i++)
            p[i] = PngUtils::sum(p[i], u[i]);

          i = bpl - bpp;

          if (i >= 32) {
            // Align to 16-BYTE boundary.
            uint32_t j = uint32_t(Support::alignUpDiff(p + bpp, 16));

            SIMD::I128 zero = SIMD::vzeroi128();
            SIMD::I128 rcp3 = SIMD::vseti128i16(0xAB << 7);

            for (i -= j; j != 0; j--, p++, u++)
              p[bpp] = PngUtils::sum(p[bpp], PngUtils::paeth(p[0], u[bpp], u[0]));

            // TODO: Not complete.
            /*
            if (bpp == 2) {
            }
            */
            if (bpp == 3) {
              SIMD::I128 pz = SIMD::vunpackli8(SIMD::vcvtu32i128(Support::readU32u(p) & 0x00FFFFFFu), zero);
              SIMD::I128 uz = SIMD::vunpackli8(SIMD::vcvtu32i128(Support::readU32u(u) & 0x00FFFFFFu), zero);
              SIMD::I128 mask = SIMD::vseti128i32(0, 0, 0x0000FFFF, 0xFFFFFFFF);

              // Process 8 BYTEs at a time.
              while (i >= 8) {
                SIMD::I128 p0, p1;
                SIMD::I128 u0, u1;

                u0 = SIMD::vloadi128_64(u + 3);
                p0 = SIMD::vloadi128_64(p + 3);

                u0 = SIMD::vunpackli8(u0, zero);
                p0 = SIMD::vunpackli8(p0, zero);
                u1 = SIMD::vsrli128b<6>(u0);

                B2D_PNG_PAETH(uz, pz, u0, uz);
                uz = SIMD::vand(uz, mask);
                p0 = SIMD::vaddi8(p0, uz);

                B2D_PNG_PAETH(uz, p0, u1, u0);
                uz = SIMD::vand(uz, mask);
                uz = SIMD::vslli128b<6>(uz);
                p0 = SIMD::vaddi8(p0, uz);

                p1 = SIMD::vsrli128b<6>(p0);
                u0 = SIMD::vsrli128b<6>(u1);

                B2D_PNG_PAETH(u0, p1, u0, u1);
                u0 = SIMD::vslli128b<12>(u0);

                p0 = SIMD::vaddi8(p0, u0);
                pz = SIMD::vsrli128b<10>(p0);
                uz = SIMD::vsrli128b<4>(u1);

                p0 = SIMD::vpackzzwb(p0, p0);
                SIMD::vstorei64(p + 3, p0);

                p += 8;
                u += 8;
                i -= 8;
              }
            }
            else if (bpp == 4) {
              SIMD::I128 pz = SIMD::vunpackli8(_mm_cvtsi32_si128(Support::readU32a(p)), zero);
              SIMD::I128 uz = SIMD::vunpackli8(_mm_cvtsi32_si128(Support::readU32u(u)), zero);
              SIMD::I128 mask = SIMD::vseti128i32(0, 0, 0xFFFFFFFF, 0xFFFFFFFF);

              // Process 16 BYTEs at a time.
              while (i >= 16) {
                SIMD::I128 p0, p1;
                SIMD::I128 u0, u1;

                p0 = SIMD::vloadi128a(p + 4);
                u0 = SIMD::vloadi128u(u + 4);

                p1 = SIMD::vunpackhi8(p0, zero);
                p0 = SIMD::vunpackli8(p0, zero);
                u1 = SIMD::vunpackhi8(u0, zero);
                u0 = SIMD::vunpackli8(u0, zero);

                B2D_PNG_PAETH(uz, pz, u0, uz);
                uz = SIMD::vand(uz, mask);
                p0 = SIMD::vaddi8(p0, uz);
                uz = SIMD::vswapi64(u0);

                B2D_PNG_PAETH(u0, p0, uz, u0);
                u0 = SIMD::vslli128b<8>(u0);
                p0 = SIMD::vaddi8(p0, u0);
                pz = SIMD::vsrli128b<8>(p0);

                B2D_PNG_PAETH(uz, pz, u1, uz);
                uz = SIMD::vand(uz, mask);
                p1 = SIMD::vaddi8(p1, uz);
                uz = SIMD::vswapi64(u1);

                B2D_PNG_PAETH(u1, p1, uz, u1);
                u1 = SIMD::vslli128b<8>(u1);
                p1 = SIMD::vaddi8(p1, u1);
                pz = SIMD::vsrli128b<8>(p1);

                p0 = SIMD::vpackzzwb(p0, p1);
                SIMD::vstorei128a(p + 4, p0);

                p += 16;
                u += 16;
                i -= 16;
              }
            }
            else if (bpp == 6) {
              SIMD::I128 pz = SIMD::vunpackli8(SIMD::vloadi128_64(p), zero);
              SIMD::I128 uz = SIMD::vunpackli8(SIMD::vloadi128_64(u), zero);

              // Process 16 BYTEs at a time.
              while (i >= 16) {
                SIMD::I128 p0, p1, p2;
                SIMD::I128 u0, u1, u2;

                p0 = SIMD::vloadi128a(p + 6);
                u0 = SIMD::vloadi128u(u + 6);

                p1 = SIMD::vsrli128b<6>(p0);
                p0 = SIMD::vunpackli8(p0, zero);
                u1 = SIMD::vsrli128b<6>(u0);
                u0 = SIMD::vunpackli8(u0, zero);

                B2D_PNG_PAETH(uz, pz, u0, uz);
                p0 = SIMD::vaddi8(p0, uz);
                p2 = SIMD::vsrli128b<6>(p1);
                u2 = SIMD::vsrli128b<6>(u1);
                p1 = SIMD::vunpackli8(p1, zero);
                u1 = SIMD::vunpackli8(u1, zero);

                B2D_PNG_PAETH(u0, p0, u1, u0);
                p1 = SIMD::vaddi8(p1, u0);
                p2 = SIMD::vunpackli8(p2, zero);
                u2 = SIMD::vunpackli8(u2, zero);

                B2D_PNG_PAETH(u0, p1, u2, u1);
                p2 = SIMD::vaddi8(p2, u0);

                p0 = SIMD::vslli128b<4>(p0);
                p0 = SIMD::vpackzzwb(p0, p1);
                p0 = SIMD::vslli128b<2>(p0);
                p0 = SIMD::vsrli128b<4>(p0);

                p2 = SIMD::vdupli64(p2);
                u2 = SIMD::vdupli64(u2);

                pz = SIMD::vswizi32<3, 3, 1, 0>(SIMD::vunpackhi32(p1, p2));
                uz = SIMD::vswizi32<3, 3, 1, 0>(SIMD::vunpackhi32(u1, u2));

                p2 = SIMD::vpackzzwb(p2, p2);
                p2 = SIMD::vslli128b<12>(p2);

                p0 = SIMD::vor(p0, p2);
                SIMD::vstorei128a(p + 6, p0);

                p += 16;
                u += 16;
                i -= 16;
              }
            }
            else if (bpp == 8) {
              SIMD::I128 pz = SIMD::vunpackli8(SIMD::vloadi128_64(p), zero);
              SIMD::I128 uz = SIMD::vunpackli8(SIMD::vloadi128_64(u), zero);

              // Process 16 BYTEs at a time.
              while (i >= 16) {
                SIMD::I128 p0, p1;
                SIMD::I128 u0, u1;

                p0 = SIMD::vloadi128a(p + 8);
                u0 = SIMD::vloadi128u(u + 8);

                p1 = SIMD::vunpackhi8(p0, zero);
                p0 = SIMD::vunpackli8(p0, zero);
                u1 = SIMD::vunpackhi8(u0, zero);
                u0 = SIMD::vunpackli8(u0, zero);

                B2D_PNG_PAETH(uz, pz, u0, uz);
                p0 = SIMD::vaddi8(p0, uz);

                B2D_PNG_PAETH(pz, p0, u1, u0);
                pz = SIMD::vaddi8(pz, p1);
                uz = u1;

                p0 = SIMD::vpackzzwb(p0, pz);
                SIMD::vstorei128a(p + 8, p0);

                p += 16;
                u += 16;
                i -= 16;
              }
            }
          }

          for (; i != 0; i--, p++, u++)
            p[bpp] = PngUtils::sum(p[bpp], PngUtils::paeth(p[0], u[bpp], u[0]));

          p += bpp;
        }
        break;
      }

      case kPngFilterAvg0: {
        for (i = bpl - bpp; i != 0; i--, p++)
          p[bpp] = PngUtils::sum(p[bpp], p[0] >> 1);

        p += bpp;
        break;
      }
    }

    if (--y == 0)
      break;

    u = p - bpl;
    filterType = *p++;

    if (B2D_UNLIKELY(filterType >= kPngFilterCount))
      return DebugUtils::errored(kErrorPngInvalidFilter);
  }

  return kErrorOk;
}

} // PngInternal namespace

B2D_END_NAMESPACE
