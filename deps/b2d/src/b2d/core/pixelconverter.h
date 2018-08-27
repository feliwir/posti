// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_PIXELCONVERTER_H
#define _B2D_CORE_PIXELCONVERTER_H

// [Dependencies]
#include "../core/argb.h"
#include "../core/image.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::PixelConverter]
// ============================================================================

//! Pixel converter.
//!
//! Provides interface to convert pixels between Blend2D native pixel formats
//! and foreign pixel formats specified by `PixelConverter::Layout`.
//!
//! To use pixel converter you must first initialize it by calling `initImport()`
//! or `initExport()`. These functions will setup the best appropriate conversion
//! function that is then called by both `convertSpan()` and `convertRect()`.
class PixelConverter {
public:
  B2D_NONCOPYABLE(PixelConverter)

  //! Conversion mode - one side must always be compatible with Blend2D pixel format.
  enum Mode : uint32_t {
    kModeImport = 0,
    kModeExport = 1
  };

  //! Pixel layout can be used to describe an external pixel format useful for import and export.
  //!
  //! Supported depths are:
  //!   - 1  - Indexed
  //!   - 2  - Indexed
  //!   - 4  - Indexed
  //!   - 8  - [A]RGB
  //!   - 16 - [A]RGB
  //!   - 24 - [A]RGB
  //!   - 32 - [A]RGB
  enum Layout : uint32_t {
    //! Pixel BPP mask.
    kLayoutDepthMask = 0x000001FFu,

    //! Pixel is stored in little-endian byte-order.
    kLayoutLE = 0x00001000u,
    //! Pixel is stored in big-endian byte-order.
    kLayoutBE = 0x00002000u,
    //! Pixel is stored in native byte-order.
    kLayoutNativeBO = B2D_ARCH_LE ? kLayoutLE : kLayoutBE,
    //! Pixel layout mask (both byte-order values).
    kLayoutBOMask = kLayoutLE | kLayoutBE,

    //! Pixel is an index to a palette[] table.
    kLayoutIsIndexed = 0x00040000u,
    //! Pixel components (RGB or Y) are premultiplied by alpha.
    kLayoutIsPremultiplied = 0x00080000u,

    //! Used internally, set when the foreign layout has byte aligned 8-bit components.
    _kLayoutIsByteAligned = 0x00100000u
  };

  //! Pixel converter function.
  typedef void (B2D_CDECL* ConvertFunc)(
    const PixelConverter* cvt,
    uint8_t* dstData, intptr_t dstStride,
    const uint8_t* srcData, intptr_t srcStride,
    uint32_t w, uint32_t h, uint32_t gapSize);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE PixelConverter() noexcept : _convert(nullptr) {}
  B2D_INLINE ~PixelConverter() noexcept { reset(); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize the context.
  B2D_API Error _init(uint32_t mode, uint32_t pixelFormat, uint32_t layout, const void* layoutData) noexcept;
  //! Reset the context, called by destructor automatically.
  B2D_API Error reset() noexcept;

  //! Get whether the context has been successfully initialized.
  B2D_INLINE bool isInitialized() const noexcept { return _convert != nullptr; }

  //! Initialize converter to convert from an external `layout` to Blend2D's `pixelFormat`.
  B2D_INLINE Error initImport(uint32_t pixelFormat, uint32_t layout, const void* layoutData) noexcept {
    return _init(kModeImport, pixelFormat, layout, layoutData);
  }

  //! Initialize converter to convert from Blend2D's `pixelFormat` to an external `layout`.
  B2D_INLINE Error initExport(uint32_t pixelFormat, uint32_t layout, const void* layoutData) noexcept {
    return _init(kModeExport, pixelFormat, layout, layoutData);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T>
  B2D_INLINE T& data() noexcept {
    static_assert(sizeof(T) <= sizeof(_data),
                  "Size of T cannot be greater than the size of the internal data");

    return *reinterpret_cast<T*>(_data.buffer);
  }

  template<typename T>
  B2D_INLINE const T& data() const noexcept {
    static_assert(sizeof(T) <= sizeof(_data),
                  "Size of T cannot be greater than the size of the internal data");

    return *reinterpret_cast<const T*>(_data.buffer);
  }

  // --------------------------------------------------------------------------
  // [Convert]
  // --------------------------------------------------------------------------

  B2D_INLINE void convertSpan(void* dstData, const void* srcData, uint32_t w) const noexcept {
    _convert(this, static_cast<      uint8_t*>(dstData), 0,
                   static_cast<const uint8_t*>(srcData), 0, w, 1, 0);
  }

  //! Convert pixels from source layout into the destination layout.
  B2D_INLINE void convertRect(void*       dstData, intptr_t dstStride,
                              const void* srcData, intptr_t srcStride,
                              uint32_t w, uint32_t h, uint32_t gapSize = 0) const noexcept {
    _convert(this, static_cast<      uint8_t*>(dstData), dstStride,
                   static_cast<const uint8_t*>(srcData), srcStride, w, h, gapSize);
  }

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Used by conversion functions to fill a destination gap caused by alignment, etc...
  static B2D_INLINE uint8_t* _fillGap(uint8_t* data, size_t size) noexcept {
    // Should not be unrolled.
    size_t i = size;
    while (i) {
      *data++ = 0;
      i--;
    }
    return data;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Conversion function.
  ConvertFunc _convert;

  //! Internal data that can be used by converters.
  struct alignas(sizeof(uintptr_t)) Data {
    struct NativeFromIndexed {
      uint32_t layout;
      const uint32_t* pal;
    };

    struct NativeFromXRGB {
      B2D_INLINE bool testRGB(uint32_t rMask, uint32_t gMask, uint32_t bMask) noexcept {
        return (masks[1] == rMask) & (masks[2] == gMask) & (masks[3] == bMask);
      }

      B2D_INLINE bool testARGB(uint32_t aMask, uint32_t rMask, uint32_t gMask, uint32_t bMask) noexcept {
        return (masks[0] == aMask) & (masks[1] == rMask) & (masks[2] == gMask) & (masks[3] == bMask);
      }

      uint32_t layout;
      uint32_t fillMask;
      uint8_t shift[4];
      uint32_t masks[4];
      uint32_t scale[4];
      uint32_t simdData[4];
    };

    struct XRGBFromNative {
      uint32_t layout;
      uint32_t fillMask;
      uint8_t shift[4];
      uint32_t masks[4];
      uint32_t simdData[4];
    };

    uint8_t buffer[64];
  } _data;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_PIXELCONVERTER_H
