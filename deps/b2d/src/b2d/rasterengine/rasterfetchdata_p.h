// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERFETCHDATA_P_H
#define _B2D_RASTERENGINE_RASTERFETCHDATA_P_H

// [Dependencies]
#include "../rasterengine/rasterglobals_p.h"
#include "../pipe2d/fetchdata_p.h"
#include "../pipe2d/pipesignature_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterEngine::RasterFetchData]
// ============================================================================

struct RasterFetchData {
  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void initGradient(
    const GradientImpl* gradientI,
    const GradientCache* cache,
    const Matrix2D& m,
    const Matrix2D& mInv,
    uint32_t components) noexcept {

    data.gradient.initTable(cache->data(), cache->size());

    Pipe2D::PipeSignature sig = { 0 };
    sig.addSrcPixelFormat(components & PixelFlags::kComponentAlpha ? PixelFormat::kPRGB32 : PixelFormat::kXRGB32);

    uint32_t type = gradientI->type();
    uint32_t extend = gradientI->extend();

    switch (type) {
      case Gradient::kTypeLinear:
        sig.addFetchType(
          data.gradient.initLinear(
            extend,
            gradientI->vertex(Gradient::kVertexIdLinearX0Y0),
            gradientI->vertex(Gradient::kVertexIdLinearX1Y1),
            m,
            mInv));
        break;

      case Gradient::kTypeRadial:
        sig.addFetchType(
          data.gradient.initRadial(
            extend,
            gradientI->vertex(Gradient::kVertexIdRadialCxCy),
            gradientI->vertex(Gradient::kVertexIdRadialFxFy),
            gradientI->scalar(Gradient::kScalarIdRadialCr),
            m,
            mInv));
        break;

      case Gradient::kTypeConical:
        sig.addFetchType(
          data.gradient.initConical(
            extend,
            gradientI->vertex(Gradient::kVertexIdConicalCxCy),
            gradientI->scalar(Gradient::kScalarIdConicalAngle),
            m,
            mInv));
        break;
    }

    refCount = 1;
    styleType = Context2D::kStyleTypeGradient;
    signaturePart.setValue(sig);
  }

  B2D_INLINE void initTextureBlitAA(
    const ImageImpl* imageI,
    const IntRect& area) noexcept {

    B2D_ASSERT(area.x() >= 0);
    B2D_ASSERT(area.y() >= 0);
    B2D_ASSERT(area.w() >  0);
    B2D_ASSERT(area.h() >  0);

    uint32_t srcBpp    = imageI->bpp();
    uint32_t srcFormat = imageI->pixelFormat();
    intptr_t srcStride = imageI->stride();

    data.texture.initSource(imageI->pixelData() + uint32_t(area.y()) * srcStride + uint32_t(area.x()) * srcBpp, imageI->stride(), uint32_t(area.w()), uint32_t(area.h()));
    refCount = 1;
    styleType = Context2D::kStyleTypePattern;
    signaturePart.reset();
    signaturePart.addFetchType(data.texture.initBlitAA());
    signaturePart.addSrcPixelFormat(srcFormat);
  }

  B2D_INLINE void initTextureFxFy(
    const ImageImpl* imageI,
    const IntRect& area,
    uint32_t extend,
    uint32_t filter,
    int64_t txFixed,
    int64_t tyFixed) noexcept {

    B2D_ASSERT(area.x() >= 0);
    B2D_ASSERT(area.y() >= 0);
    B2D_ASSERT(area.w() >  0);
    B2D_ASSERT(area.h() >  0);

    uint32_t srcBpp    = imageI->bpp();
    uint32_t srcFormat = imageI->pixelFormat();
    intptr_t srcStride = imageI->stride();

    data.texture.initSource(imageI->pixelData() + uint32_t(area.y()) * srcStride + uint32_t(area.x()) * srcBpp, imageI->stride(), uint32_t(area.w()), uint32_t(area.h()));
    refCount = 1;
    styleType = Context2D::kStyleTypePattern;
    signaturePart.reset();
    signaturePart.addFetchType(data.texture.initTranslateFxFy(extend, filter, txFixed, tyFixed));
    signaturePart.addSrcPixelFormat(srcFormat);
  }

  B2D_INLINE void initTextureAffine(
    const ImageImpl* imageI,
    const IntRect& area,
    uint32_t extend,
    uint32_t filter,
    const Matrix2D& m,
    const Matrix2D& mInv) noexcept {

    B2D_ASSERT(area.x() >= 0);
    B2D_ASSERT(area.y() >= 0);
    B2D_ASSERT(area.w() >  0);
    B2D_ASSERT(area.h() >  0);

    uint32_t srcBpp    = imageI->bpp();
    uint32_t srcFormat = imageI->pixelFormat();
    intptr_t srcStride = imageI->stride();

    data.texture.initSource(imageI->pixelData() + uint32_t(area.y()) * srcStride + uint32_t(area.x()) * srcBpp, imageI->stride(), uint32_t(area.w()), uint32_t(area.h()));
    refCount = 1;
    styleType = Context2D::kStyleTypePattern;
    signaturePart.reset();
    signaturePart.addFetchType(data.texture.initAffine(extend, filter, m, mInv));
    signaturePart.addSrcPixelFormat(srcFormat);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Pipe2D::FetchData data;

  union {
    void* voidRef;
    ImageImpl* imageImplRef;
    GradientCache* gradientCacheRef;
  };

  size_t refCount;                       //!< Reference count (not atomic, not needed here).
  uint32_t styleType;                    //!< Style type.
  Pipe2D::PipeSignature signaturePart;   //!< Signature part that contains SrcFormat, FetchType, and FetchExtra.
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERFETCHDATA_P_H
