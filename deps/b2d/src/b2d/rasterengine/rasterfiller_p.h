// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERFILLER_P_H
#define _B2D_RASTERENGINE_RASTERFILLER_P_H

// [Dependencies]
#include "../core/context2d.h"
#include "../core/geomtypes.h"
#include "../core/zone.h"
#include "../pipe2d/filldata_p.h"
#include "../pipe2d/pipesignature_p.h"
#include "../rasterengine/edgestorage_p.h"
#include "../rasterengine/rasterfetchdata_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterFiller]
// ============================================================================

class RasterFiller {
public:
  typedef Pipe2D::PipeSignature PipeSignature;
  typedef Pipe2D::FillFunc FillFunc;
  typedef Pipe2D::FillData FillData;

  typedef Error (B2D_CDECL* WorkFunc)(RasterFiller* ctx, RasterWorker* worker, const RasterFetchData* fetchData);

  B2D_INLINE RasterFiller() noexcept :
    _workFunc(nullptr),
    _fillFunc(nullptr),
    _fillSignature { 0 },
    _edgeStorage(nullptr) {}

  B2D_INLINE bool isValid() const noexcept { return _fillSignature.value() != 0; }

  B2D_INLINE void reset() noexcept {
    _fillSignature.reset();
  }

  B2D_INLINE const FillData& fillData() const noexcept { return _fillData; }
  B2D_INLINE const PipeSignature& fillSignature() const noexcept { return _fillSignature; }

  B2D_INLINE void initBoxAA8bpc(uint32_t alpha, int x0, int y0, int x1, int y1) noexcept {
    _workFunc = fillRectImpl;
    _fillSignature.addFillType(
      _fillData.initBoxAA8bpc(alpha, x0, y0, x1, y1));
  }

  B2D_INLINE void initBoxAU8bpc24x8(uint32_t alpha, int x0, int y0, int x1, int y1) noexcept {
    _workFunc = fillRectImpl;
    _fillSignature.addFillType(
      _fillData.initBoxAU8bpc24x8(alpha, x0, y0, x1, y1));
  }

  B2D_INLINE void initAnalytic(uint32_t alpha, EdgeStorage<int>* edgeStorage, uint32_t fillRule) noexcept {
    _workFunc = fillAnalyticImpl;
    _fillData.analytic.alpha.u = alpha;
    _fillData.analytic.fillRuleMask = (fillRule == FillRule::kNonZero) ? 0xFFFFFFFFu : 0x000001FFu;
    _fillSignature.addFillType(Pipe2D::kFillTypeAnalytic);
    _edgeStorage = edgeStorage;
  }

  B2D_INLINE void setFillFunc(FillFunc fillFunc) noexcept {
    _fillFunc = fillFunc;
  }

  B2D_INLINE Error doWork(RasterWorker* worker, const RasterFetchData* fetchData) noexcept {
    return _workFunc(this, worker, fetchData);
  }

  static Error B2D_CDECL fillRectImpl(RasterFiller* ctx, RasterWorker* worker, const RasterFetchData* fetchData) noexcept;
  static Error B2D_CDECL fillAnalyticImpl(RasterFiller* ctx, RasterWorker* worker, const RasterFetchData* fetchData) noexcept;

  WorkFunc _workFunc;
  FillFunc _fillFunc;
  FillData _fillData;
  PipeSignature _fillSignature;
  EdgeStorage<int>* _edgeStorage;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERFILLER_P_H
