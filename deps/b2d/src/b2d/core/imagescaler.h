// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_IMAGESCALER_H
#define _B2D_CORE_IMAGESCALER_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/geomtypes.h"
#include "../core/math.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ImageScaler]
// ============================================================================

//! Low-level image scaler.
//!
//! Consider using `Image::scale()` if you prefer high-level API.
class ImageScaler {
public:
  B2D_NONCOPYABLE(ImageScaler)

  class Params;
  typedef Error (B2D_CDECL* CustomFunc)(double* dst, const double* t, size_t n, const void* params);

  //! Filter ID.
  enum Filter : uint32_t {
    kFilterNone      = 0,      //!< No filter or uninitialized.

    kFilterNearest   = 1,      //!< Nearest neighbor filter (radius 1.0).
    kFilterBilinear  = 2,      //!< Bilinear filter (radius 1.0).
    kFilterBicubic   = 3,      //!< Bicubic filter (radius 2.0).
    kFilterBell      = 4,      //!< Bell filter (radius 1.5).
    kFilterGauss     = 5,      //!< Gauss filter (radius 2.0).
    kFilterHermite   = 6,      //!< Hermite filter (radius 1.0).
    kFilterHanning   = 7,      //!< Hanning filter (radius 1.0).
    kFilterCatrom    = 8,      //!< Catrom filter (radius 2.0).
    kFilterBessel    = 9,      //!< Bessel filter (radius 3.2383).
    kFilterSinc      = 10,     //!< Sinc filter (radius 2.0, adjustable through `ImageScaler::Params`).
    kFilterLanczos   = 11,     //!< Lanczos filter (radius 2.0, adjustable through `ImageScaler::Params`).
    kFilterBlackman  = 12,     //!< Blackman filter (radius 2.0, adjustable through `ImageScaler::Params`).
    kFilterMitchell  = 13,     //!< Mitchell filter (radius 2.0, parameters 'b' and 'c' passed through `ImageScaler::Params`).
    kFilterCustom    = 14,     //!< Custom filter where weights are calculated by a function passed through `ImageScaler::Params`.

    kFilterCount     = 15      //!< Count of filter options.
  };

  enum Dir : uint32_t {
    kDirHorz = 0,
    kDirVert = 1
  };

  // --------------------------------------------------------------------------
  // [Params]
  // --------------------------------------------------------------------------

  class Params {
  public:
    enum Slot : uint32_t {
      kSlotRadius    = 0,
      kSlotMitchellB = 1,
      kSlotMitchellC = 2,
      kSlotCount     = 4
    };

    struct Custom {
      CustomFunc func;
      const void* data;
    };

    B2D_INLINE Params(uint32_t filter = kFilterNone) noexcept
      : _filter(filter),
        _custom { nullptr, nullptr },
        _values { 2.0, 1.0 / 3.0, 1.0 / 3.0, 0.0 } {}
    B2D_INLINE Params(const Params& other) noexcept = default;

    static B2D_INLINE Params makeSinc(double radius) noexcept {
      Params params { kFilterSinc };
      params.setRadius(radius);
      return params;
    }

    static B2D_INLINE Params makeLanczos(double radius) noexcept {
      Params params { kFilterLanczos };
      params.setRadius(radius);
      return params;
    }

    static B2D_INLINE Params makeBlackman(double radius) noexcept {
      Params params { kFilterBlackman };
      params.setRadius(radius);
      return params;
    }

    static B2D_INLINE Params makeMitchell(double b, double c) noexcept {
      Params params { kFilterMitchell };
      params.setMitchellB(b);
      params.setMitchellC(c);
      return params;
    }

    static B2D_INLINE Params makeCustom(CustomFunc func, const void* data) noexcept {
      Params params { kFilterCustom };
      params.setCustomFunc(func);
      params.setCustomData(data);
      return params;
    }

    B2D_INLINE void resetCustomParams() noexcept {
      _custom.func = nullptr;
      _custom.data = nullptr;
    }

    B2D_INLINE void resetParamsToDefaults() noexcept {
      _values[0] = 2.0;
      _values[1] = 1.0 / 3.0;
      _values[2] = 1.0 / 3.0;
      _values[3] = 0.0;
    }

    B2D_INLINE uint32_t filter() const noexcept { return _filter; }
    B2D_INLINE void setFilter(uint32_t filter) noexcept { _filter = filter; }

    B2D_INLINE CustomFunc customFunc() const noexcept { return _custom.func; }
    B2D_INLINE void setCustomFunc(CustomFunc func) noexcept { _custom.func = func; }

    B2D_INLINE const void* customData() const noexcept { return _custom.data; }
    B2D_INLINE void setCustomData(const void* data) noexcept { _custom.data = data; }

    B2D_INLINE double radius() const noexcept { return _values[kSlotRadius]; }
    B2D_INLINE void setRadius(double value) noexcept { _values[kSlotRadius] = value; }

    B2D_INLINE double mitchellB() const noexcept { return _values[kSlotMitchellB]; }
    B2D_INLINE void setMitchellB(double value) noexcept { _values[kSlotMitchellB] = value; }

    B2D_INLINE double mitchellC() const noexcept { return _values[kSlotMitchellC]; }
    B2D_INLINE void setMitchellC(double value) noexcept { _values[kSlotMitchellC] = value; }

    uint32_t _filter;
    Custom _custom;
    double _values[kSlotCount];
  };

  struct Record {
    uint32_t pos;
    uint32_t count;
  };

  struct Data {
    Allocator* allocator;                //!< Memory allocator.
    size_t size;                         //!< Size of this `Data`.

    int dstSize[2];                      //!< Destination image size.
    int srcSize[2];                      //!< Source image size.
    int kernelSize[2];
    int isUnbound[2];

    double scale[2];
    double factor[2];
    double radius[2];

    int32_t* weightList[2];
    Record* recordList[2];
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ImageScaler() noexcept
    : _data(nullptr) {}

  B2D_INLINE ImageScaler(ImageScaler&& other) noexcept
    : _data(other._data) { other._data = nullptr; }

  B2D_INLINE ~ImageScaler() noexcept { reset(Globals::kResetHard); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isInitialized() const noexcept { return _data != nullptr; }

  B2D_INLINE int dstWidth() const noexcept { return _data->dstSize[kDirHorz]; }
  B2D_INLINE int dstHeight() const noexcept { return _data->dstSize[kDirVert]; }

  B2D_INLINE int srcWidth() const noexcept { return _data->srcSize[kDirHorz]; }
  B2D_INLINE int srcHeight() const noexcept { return _data->srcSize[kDirVert]; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;
  B2D_API Error create(const IntSize& to, const IntSize& from, const Params& params, Allocator* allocator = nullptr) noexcept;

  B2D_API Error processHorzData(uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride, uint32_t pixelFormat) const noexcept;
  B2D_API Error processVertData(uint8_t* dstLine, intptr_t dstStride, const uint8_t* srcLine, intptr_t srcStride, uint32_t pixelFormat) const noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Data* _data;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_IMAGESCALER_H
