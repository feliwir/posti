// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/pixelformat.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::PixelFormat - PixelFormatPropertiesT]
// ============================================================================

template<uint32_t ID>
struct PixelFormatPropertiesT {};

#define B2D_DEFINE_PIXEL_FORMAT(ID, BPP, FLAGS) \
template<>                                      \
struct PixelFormatPropertiesT<ID> {             \
  enum : uint32_t {                             \
    kId = ID,                                   \
    kBpp = BPP,                                 \
    kFlags = FLAGS                              \
  };                                            \
};

B2D_DEFINE_PIXEL_FORMAT(PixelFormat::kNone  , 0, 0);
B2D_DEFINE_PIXEL_FORMAT(PixelFormat::kPRGB32, 4, PixelFlags::kComponentARGB  | PixelFlags::kPremultiplied);
B2D_DEFINE_PIXEL_FORMAT(PixelFormat::kXRGB32, 4, PixelFlags::kComponentRGB   | PixelFlags::kUnusedBits);
B2D_DEFINE_PIXEL_FORMAT(PixelFormat::kA8    , 1, PixelFlags::kComponentAlpha);

#undef B2D_DEFINE_PIXEL_FORMAT

// ============================================================================
// [b2d::PixelFormat - Table]
// ============================================================================

const PixelFormatInfo PixelFormatInfo::table[PixelFormat::kCount] = {
  #define ROW(ID) {{{                  \
    PixelFormatPropertiesT<ID>::kId,   \
    PixelFormatPropertiesT<ID>::kBpp,  \
    PixelFormatPropertiesT<ID>::kFlags \
  }}}
  ROW(0),
  ROW(1),
  ROW(2),
  ROW(3)
  #undef ROW
};

B2D_END_NAMESPACE
