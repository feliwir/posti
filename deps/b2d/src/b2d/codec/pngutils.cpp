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
#include "../core/pixelutils_p.h"
#include "../core/math.h"
#include "../core/membuffer.h"
#include "../core/runtime.h"
#include "../core/simd.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

PngOps _bPngOps;

namespace PngInternal {

// ============================================================================
// [b2d::PngInternal - InverseFilter (Ref)]
// ============================================================================

Error inverseFilterRef(uint8_t* p, uint32_t bpp, uint32_t bpl, uint32_t h) noexcept {
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
      case kPngFilterNone:
        p += bpl;
        break;

      case kPngFilterSub: {
        for (i = bpl - bpp; i != 0; i--, p++)
          p[bpp] = PngUtils::sum(p[bpp], p[0]);

        p += bpp;
        break;
      }

      case kPngFilterUp: {
        for (i = bpl; i != 0; i--, p++, u++)
          p[0] = PngUtils::sum(p[0], u[0]);
        break;
      }

      case kPngFilterAvg: {
        for (i = 0; i < bpp; i++)
          p[i] = PngUtils::sum(p[i], u[i] >> 1);

        u += bpp;
        for (i = bpl - bpp; i != 0; i--, p++, u++)
          p[bpp] = PngUtils::sum(p[bpp], PngUtils::average(p[0], u[0]));

        p += bpp;
        break;
      }

      case kPngFilterPaeth: {
        for (i = 0; i < bpp; i++)
          p[i] = PngUtils::sum(p[i], u[i]);

        for (i = bpl - bpp; i != 0; i--, p++, u++)
          p[bpp] = PngUtils::sum(p[bpp], PngUtils::paeth(p[0], u[bpp], u[0]));

        p += bpp;
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
