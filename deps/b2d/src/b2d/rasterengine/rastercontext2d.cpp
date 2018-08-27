// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/pixelutils_p.h"
#include "../core/runtime.h"
#include "../core/wrap.h"
#include "../core/zeroallocator_p.h"
#include "../rasterengine/alternatives_p.h"
#include "../rasterengine/edgebuilder_p.h"
#include "../rasterengine/edgesource_p.h"
#include "../rasterengine/rastercontext2d_p.h"
#include "../rasterengine/rasterfiller_p.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/glyphengine.h"
#include "../text/glyphoutlinedecoder.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

// ============================================================================
// [b2d::RasterContext2DImpl - Construction / Destruction]
// ============================================================================

RasterContext2DImpl::RasterContext2DImpl() noexcept
  : Context2DImpl(),
    _pipeRuntime(&Pipe2D::PipeRuntime::_global),
    _cmdZone(16384 - Zone::kBlockOverhead),
    _baseZone(8192 - Zone::kBlockOverhead, 16),
    _fetchPool(&_baseZone),
    _statePool(&_baseZone),
    _contextOriginId(Cookie::generateUnique64BitId()),
    _stateIdCounter(0),
    _baseSignature(0),
    _fpShiftI(0),
    _fpScaleI(0),
    _fpMaskI(0),
    _fpScaleD(0.0),
    _fpMinSafeCoordD(0.0),
    _fpMaxSafeCoordD(0.0),
    _toleranceD(0.0),
    _toleranceFixedD(0.0),
    _toleranceFixedSqD(0.0),
    _curState(nullptr),
    _metaClipBoxI(),
    _finalClipBoxI(),
    _finalClipBoxD(),
    _finalClipBoxFixedD(),
    _metaMatrix(),
    _userMatrix(),
    _finalMatrix(),
    _metaMatrixFixed(),
    _finalMatrixFixed(),
    _matrixInfo(),
    _intTranslation(0, 0),
    _intClientBox(0, 0, 0, 0),
    _worker(this) {

  _implType = Context2D::kImplTypePipe2D;
  _contextFlags = 0;

  _dstInfo.reset();
  _dstImage->_impl = nullptr;

  _coreParams.reset();
  _extParams.reset(0);
}

RasterContext2DImpl::~RasterContext2DImpl() noexcept {
  // It shouldn't happen that we are attempting to destroy a Context2D that
  // still has an attached surface and is active. We have a serious problem::
  // in such case.
  B2D_ASSERT(_dstImage->_impl == nullptr);

  // Analogically, the context should be reset as well.
  B2D_ASSERT(_curState == nullptr);
}

RasterContext2DImpl* RasterContext2DImpl::_create(Image& image, const InitParams* initParams) noexcept {
  RasterContext2DImpl* ctxI = new(std::nothrow) RasterContext2DImpl();

  if (B2D_UNLIKELY(!ctxI))
    return nullptr;

  if (_attach(ctxI, image, initParams) != kErrorOk) {
    delete ctxI;
    return nullptr;
  }

  return ctxI;
}

void RasterContext2DImpl::destroy() noexcept {
  if (_dstImage->_impl)
    _detach(this);

  delete this;
}

// ============================================================================
// [b2d::RasterContext2DImpl - Attach / Detach]
// ============================================================================

Error RasterContext2DImpl::_attach(RasterContext2DImpl* d, Image& image, const InitParams* initParams) noexcept {
  B2D_ASSERT(d->_dstImage->_impl == nullptr);

  void* pRuntime = nullptr;
  if (initParams->hasFlag(Context2D::kInitFlagIsolated)) {
    pRuntime = d->_baseZone.alloc(sizeof(Pipe2D::PipeRuntime));
    if (B2D_UNLIKELY(!pRuntime))
      return DebugUtils::errored(kErrorNoMemory);
  }

  B2D_PROPAGATE(image.lock(d->_worker._buffer, static_cast<void*>(d)));

  int iw = d->_worker._buffer._size.width();
  int ih = d->_worker._buffer._size.height();
  d->_worker._ctxData.dst.init(d->_worker._buffer.pixelData(), d->_worker._buffer.stride(), iw, ih);

  // TODO: Hardcoded.
  int fullAlphaI = 256;

  int fpShift = 8;
  int fpScaleI = 1 << fpShift;
  double fpScaleD = fpScaleI;

  d->_contextFlags = kSWContext2DIsIntTranslation;

  double dw = double(iw);
  double dh = double(ih);
  d->_targetSizeI.reset(iw, ih);
  d->_targetSizeD.reset(dw, dh);

  d->_dstInfo._is16Bit = false;
  d->_dstInfo._dstFlags = uint8_t(d->_worker._buffer.pixelFlags() & PixelFlags::kComponentMask);
  d->_dstInfo._fullAlphaI = uint32_t(fullAlphaI);
  d->_dstInfo._fullAlphaD = double(fullAlphaI);
  d->_dstImage->_impl = image._impl;

  d->_baseSignature.reset();
  d->_baseSignature.addDstPixelFormat(d->_worker._buffer.pixelFormat());

  d->_fpShiftI = fpShift;
  d->_fpScaleI = fpScaleI;
  d->_fpMaskI = fpScaleI - 1;
  d->_fpScaleD = fpScaleD;
  d->_fpMinSafeCoordD = std::floor(double(std::numeric_limits<int32_t>::min() + 1                    ) * fpScaleD);
  d->_fpMaxSafeCoordD = std::floor(double(std::numeric_limits<int32_t>::max() - 1 - Math::max(dw, dh)) * fpScaleD);

  d->_toleranceD = kToleranceDefault;
  d->_onUpdateTolerance();

  // Reset clipBox to the default state (respects the target size).
  d->_metaClipBoxI.reset(0, 0, iw, ih);
  d->_finalClipBoxI.reset(0, 0, iw, ih);
  d->_finalClipBoxD.reset(0.0, 0.0, dw, dh);
  d->_finalClipBoxFixedD.reset(0.0, 0.0, dw * fpScaleD, dh * fpScaleD);

  // Reset transformation matrices into the default state.
  d->_metaMatrix.reset();
  d->_userMatrix.reset();
  d->_finalMatrix.reset();
  d->_metaMatrixFixed.resetToScaling(fpScaleD);
  d->_finalMatrixFixed.resetToScaling(fpScaleD);

  d->_matrixInfo._metaMatrixType = Matrix2D::kTypeTranslation;
  d->_matrixInfo._finalMatrixType = Matrix2D::kTypeTranslation;

  d->_intClientBox.reset(0, 0, iw, ih);
  d->_intTranslation.reset();

  d->_coreParams.reset();
  d->_extParams.reset(uint32_t(fullAlphaI));
  d->_updateCompOpTable(CompOp::kSrcOver);

  SWContext2DUtil::initStyleToDefault(d->_style[0], uint32_t(fullAlphaI));
  SWContext2DUtil::initStyleToDefault(d->_style[1], uint32_t(fullAlphaI));

  d->_worker._fullAlpha = uint32_t(fullAlphaI);
  d->_worker.ensureEdgeStorage();

  // DEBUG: Create an isolated `PipeRuntime` If `kInitFlagIsolated` was set. It
  //        will be used to store all functions generated during the rendering
  //        and will be destroyed together with the `Context2DImpl`. As it always
  //        creates all JIT code it's only useful to debug the Pipe2D compiler.
  if (initParams->hasFlag(Context2D::kInitFlagIsolated)) {
    Pipe2D::PipeRuntime* rt = new(pRuntime) Pipe2D::PipeRuntime();
    rt->setOptLevel(initParams->optLevel());
    rt->setMaxPixelStep(initParams->maxPixels());
    d->_pipeRuntime = rt;
  }

  return kErrorOk;
}

Error RasterContext2DImpl::_detach(RasterContext2DImpl* ctxI) noexcept {
  B2D_ASSERT(ctxI->_dstImage->_impl != nullptr);

  ctxI->_dstImage->unlock(ctxI->_worker._buffer, static_cast<void*>(ctxI));
  ImageImpl* imgI = ctxI->_dstImage->impl();

  // If the image was dereferenced during rendering it's our responsibility to
  // destroy it. This is not useful from consumer perspective as the resulting
  // image can never be used, but it can happen in some cases (for example when
  // asynchronous rendering is terminated and the target image dereferenced).
  if (imgI->refCount() == 0)
    imgI->destroy();

  ctxI->_reset();
  return kErrorOk;
}

// ============================================================================
// [b2d::RasterContext2DImpl - Reset]
// ============================================================================

void RasterContext2DImpl::_reset() noexcept {
  Pipe2D::PipeRuntime* globalRuntime = &Pipe2D::PipeRuntime::_global;
  Pipe2D::PipeRuntime* currentRuntime = _pipeRuntime;

  // Only true if the context was created with `Context2D::kInitFlagIsolated`.
  if (currentRuntime != globalRuntime) {
    _pipeRuntime = globalRuntime;
    currentRuntime->~PipeRuntime();
  }

  _discardStates(nullptr);
  if (_contextFlags & kSWContext2DInfoFillFetchData) _discardStyle(_style[Context2D::kStyleSlotFill]);
  if (_contextFlags & kSWContext2DInfoStrokeFetchData) _discardStyle(_style[Context2D::kStyleSlotStroke]);

  _contextFlags = 0;
  _dstInfo.reset();
  _dstImage->_impl = nullptr;

  _baseZone.reset();
  _cmdZone.reset();
  _fetchPool.reset();
  _statePool.reset();

  _coreParams.reset();
  _extParams.reset(0);
  _compOpTable._packed = 0;
  _curState = nullptr;

  _worker._ctxData.reset();
}

// ============================================================================
// [b2d::RasterContext2DImpl - Flush]
// ============================================================================

Error RasterContext2DImpl::flush(uint32_t flags) noexcept {
  // TODO:
  return kErrorOk;
}

// ============================================================================
// [b2d::RasterContext2DImpl - Properties]
// ============================================================================

#define PARAM_R(T) (*static_cast<const T*>(value))
#define PARAM_W(T) (*static_cast<T*>(value))

Error RasterContext2DImpl::getProperty(uint32_t id, void* value) const noexcept {
  constexpr uint32_t kFillSlot = Context2D::kStyleSlotFill;
  constexpr uint32_t kStrokeSlot = Context2D::kStyleSlotStroke;

  switch (id) {
    case Context2D::kPropertyIdContextParams: {
      Context2DParams& params = PARAM_W(Context2DParams);
      params._coreParams = _coreParams;
      params._strokeParams = _stroker.params();
      return kErrorOk;
    }

    case Context2D::kPropertyIdContextHints    : PARAM_W(Context2DHints) = _coreParams._hints                  ; return kErrorOk;
    case Context2D::kPropertyIdRenderQuality   : PARAM_W(uint32_t)       = _coreParams._hints._renderQuality   ; return kErrorOk;
    case Context2D::kPropertyIdGradientFilter  : PARAM_W(uint32_t)       = _coreParams._hints._gradientFilter  ; return kErrorOk;
    case Context2D::kPropertyIdPatternFilter   : PARAM_W(uint32_t)       = _coreParams._hints._patternFilter   ; return kErrorOk;
    case Context2D::kPropertyIdTolerance       : PARAM_W(double)         = _toleranceD                         ; return kErrorOk;
    case Context2D::kPropertyIdCompOp          : PARAM_W(uint32_t)       = _coreParams._hints._compOp          ; return kErrorOk;
    case Context2D::kPropertyIdAlpha           : PARAM_W(double)         = _coreParams._globalAlpha            ; return kErrorOk;
    case Context2D::kPropertyIdFillAlpha       : PARAM_W(double)         = _coreParams._styleAlpha[kFillSlot]  ; return kErrorOk;
    case Context2D::kPropertyIdStrokeAlpha     : PARAM_W(double)         = _coreParams._styleAlpha[kStrokeSlot]; return kErrorOk;
    case Context2D::kPropertyIdFillRule        : PARAM_W(uint32_t)       = _coreParams._fillRule               ; return kErrorOk;
    case Context2D::kPropertyIdStrokeParams    : PARAM_W(StrokeParams)   = _stroker._params                    ; return kErrorOk;
    case Context2D::kPropertyIdStrokeWidth     : PARAM_W(double)         = _stroker._params.strokeWidth()      ; return kErrorOk;
    case Context2D::kPropertyIdStrokeMiterLimit: PARAM_W(double)         = _stroker._params.miterLimit()       ; return kErrorOk;
    case Context2D::kPropertyIdStrokeDashOffset: PARAM_W(double)         = _stroker._params.dashOffset()       ; return kErrorOk;
    case Context2D::kPropertyIdStrokeDashArray : return PARAM_W(Array<double>).assign(_stroker._params.dashArray());
    case Context2D::kPropertyIdStrokeJoin      : PARAM_W(uint32_t)       = _stroker._params.strokeJoin()       ; return kErrorOk;
    case Context2D::kPropertyIdStrokeStartCap  : PARAM_W(uint32_t)       = _stroker._params.startCap()         ; return kErrorOk;
    case Context2D::kPropertyIdStrokeEndCap    : PARAM_W(uint32_t)       = _stroker._params.endCap()           ; return kErrorOk;

    case Context2D::kPropertyIdStrokeLineCaps: {
      const StrokeParams& strokeParams = _stroker._params;

      uint32_t startCap = strokeParams.startCap();
      uint32_t endCap = strokeParams.endCap();

      // Return startCap in case the returned `Error` is unchecked.
      PARAM_W(uint32_t) = startCap;
      return startCap == endCap ? kErrorOk : DebugUtils::errored(kErrorInvalidState);
    }

    case Context2D::kPropertyIdStrokeTransformOrder: {
      PARAM_W(uint32_t) = _stroker._params.transformOrder();
      return kErrorOk;
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }
}

Error RasterContext2DImpl::setProperty(uint32_t id, const void* value) noexcept {
  uint32_t contextFlags = _contextFlags;

  switch (id) {
    case Context2D::kPropertyIdContextParams: {
      const Context2DParams& params = PARAM_R(Context2DParams);

      if (B2D_UNLIKELY(!params.isValid()))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      contextFlags &= ~( kSWContext2DNoPaintGlobalCompOp |
                         kSWContext2DNoPaintGlobalAlpha  |
                         kSWContext2DNoPaintFillAlpha    |
                         kSWContext2DNoPaintStrokeAlpha  |
                         kSWContext2DNoPaintStrokeParams |
                         kSWContext2DStateStrokeParams   |
                         kSWContext2DFlagCompOpSolid     );

      _coreParams = params._coreParams;

      double intAlphaD    = params._coreParams._globalAlpha * _dstInfo._fullAlphaD;
      double fillAlphaD   = params._coreParams._styleAlpha[Context2D::kStyleSlotFill];
      double strokeAlphaD = params._coreParams._styleAlpha[Context2D::kStyleSlotStroke];

      uint32_t globalAlphaI = uint32_t(Math::iround(intAlphaD));
      uint32_t fillAlphaI   = uint32_t(Math::iround(intAlphaD * fillAlphaD));
      uint32_t strokeAlphaI = uint32_t(Math::iround(intAlphaD * strokeAlphaD));

      _extParams._globalAlphaI = globalAlphaI;
      _style[Context2D::kStyleSlotFill]._alpha = fillAlphaI;
      _style[Context2D::kStyleSlotStroke]._alpha = strokeAlphaI;

      if (!globalAlphaI) contextFlags |= kSWContext2DNoPaintGlobalAlpha;
      if (!fillAlphaI  ) contextFlags |= kSWContext2DNoPaintFillAlpha;
      if (!strokeAlphaI) contextFlags |= kSWContext2DNoPaintStrokeAlpha;

      _updateCompOpTable(params.compOp());
      if (_compOpTable._simplify[PixelFlags::kComponentARGB] == CompOp::kDst)
        contextFlags |= kSWContext2DNoPaintGlobalCompOp;

      _stroker._params = params._strokeParams;
      _stroker._dirty = true;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdContextHints: {
      const Context2DHints& hints = PARAM_R(Context2DHints);

      if (B2D_UNLIKELY(!hints.isValid()))
        return DebugUtils::errored(kErrorInvalidArgument);

      contextFlags &= ~kSWContext2DNoPaintGlobalCompOp;
      _updateCompOpTable(hints._compOp);

      if (_compOpTable._simplify[PixelFlags::kComponentARGB] == CompOp::kDst)
        contextFlags |= kSWContext2DNoPaintGlobalCompOp;

      _coreParams._hints = hints;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdRenderQuality: {
      uint32_t quality = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(quality >= Context2D::kRenderQualityCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      _coreParams._hints._renderQuality = quality;
      return kErrorOk;
    }

    case Context2D::kPropertyIdGradientFilter: {
      uint32_t filter = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(filter >= Gradient::kFilterCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      _coreParams._hints._gradientFilter = filter;
      return kErrorOk;
    }

    case Context2D::kPropertyIdPatternFilter: {
      uint32_t filter = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(filter >= Pattern::kFilterCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      _coreParams._hints._patternFilter = filter;
      return kErrorOk;
    }

    case Context2D::kPropertyIdTolerance: {
      double tolerance = Math::bound(PARAM_R(double), kToleranceMinimum, kToleranceMaximum);
      if (B2D_UNLIKELY(std::isnan(tolerance)))
        return DebugUtils::errored(kErrorInvalidArgument);

      _onBeforeConfigChange();
      _toleranceD = tolerance;
      _onUpdateTolerance();

      _contextFlags = contextFlags & ~kSWContext2DStateConfig;
      return kErrorOk;
    }

    case Context2D::kPropertyIdCompOp: {
      uint32_t compOp = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(compOp >= CompOp::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      contextFlags &= ~kSWContext2DNoPaintGlobalCompOp;
      _updateCompOpTable(compOp);

      if (_compOpTable._simplify[PixelFlags::kComponentARGB] == CompOp::kDst)
        contextFlags |= kSWContext2DNoPaintGlobalCompOp;

      _coreParams._hints._compOp = compOp;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdAlpha: {
      double alpha = PARAM_R(double);

      if (B2D_UNLIKELY(!Math::isUnitInterval(alpha)))
        return DebugUtils::errored(kErrorInvalidArgument);

      contextFlags &= ~(kSWContext2DNoPaintGlobalAlpha |
                        kSWContext2DNoPaintFillAlpha   |
                        kSWContext2DNoPaintStrokeAlpha );

      double intAlphaD    = alpha * _dstInfo._fullAlphaD;
      double fillAlphaD   = intAlphaD * _coreParams._styleAlpha[Context2D::kStyleSlotFill];
      double strokeAlphaD = intAlphaD * _coreParams._styleAlpha[Context2D::kStyleSlotStroke];

      uint32_t globalAlphaI = uint32_t(Math::iround(intAlphaD));
      uint32_t fillAlphaI   = uint32_t(Math::iround(fillAlphaD));
      uint32_t strokeAlphaI = uint32_t(Math::iround(strokeAlphaD));

      _coreParams._globalAlpha = alpha;
      _extParams._globalAlphaI = globalAlphaI;

      _style[Context2D::kStyleSlotFill]._alpha = fillAlphaI;
      _style[Context2D::kStyleSlotStroke]._alpha = strokeAlphaI;

      if (!globalAlphaI) contextFlags |= kSWContext2DNoPaintGlobalAlpha;
      if (!fillAlphaI  ) contextFlags |= kSWContext2DNoPaintFillAlpha;
      if (!strokeAlphaI) contextFlags |= kSWContext2DNoPaintStrokeAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdFillAlpha: {
      double alpha = PARAM_R(double);

      if (B2D_UNLIKELY(!Math::isUnitInterval(alpha)))
        return DebugUtils::errored(kErrorInvalidArgument);

      contextFlags &= ~kSWContext2DNoPaintFillAlpha;
      uint32_t alphaI = uint32_t(Math::iround(_coreParams._globalAlpha * _dstInfo._fullAlphaD * alpha));

      _coreParams._styleAlpha[Context2D::kStyleSlotFill] = alpha;
      _style[Context2D::kStyleSlotFill]._alpha = alphaI;

      if (!alphaI) contextFlags |= kSWContext2DNoPaintFillAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeAlpha: {
      double alpha = PARAM_R(double);

      if (B2D_UNLIKELY(!Math::isUnitInterval(alpha)))
        return DebugUtils::errored(kErrorInvalidArgument);

      contextFlags &= ~kSWContext2DNoPaintStrokeAlpha;
      uint32_t alphaI = uint32_t(Math::iround(_coreParams._globalAlpha * _dstInfo._fullAlphaD * alpha));

      _coreParams._styleAlpha[Context2D::kStyleSlotStroke] = alpha;
      _style[Context2D::kStyleSlotStroke]._alpha = alphaI;

      if (!alphaI) contextFlags |= kSWContext2DNoPaintStrokeAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdFillRule: {
      uint32_t fillRule = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(fillRule >= FillRule::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      _coreParams._fillRule = fillRule;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeParams: {
      const StrokeParams& params = PARAM_R(StrokeParams);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      contextFlags &= ~(kSWContext2DNoPaintStrokeParams |
                        kSWContext2DStateStrokeParams   );

      _contextFlags = contextFlags;
      _stroker._params = params;
      _stroker._dirty = true;

      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeWidth: {
      double strokeWidth = PARAM_R(double);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      contextFlags &= ~(kSWContext2DNoPaintStrokeParams |
                        kSWContext2DStateStrokeParams   );

      _contextFlags = contextFlags;
      _stroker._params.setStrokeWidth(strokeWidth);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeMiterLimit: {
      double miterLimit = PARAM_R(double);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setMiterLimit(miterLimit);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeDashOffset: {
      double dashOffset = PARAM_R(double);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setDashOffset(dashOffset);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeDashArray: {
      const Array<double>& dashArray = PARAM_R(Array<double>);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setDashArray(dashArray);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeJoin: {
      uint32_t strokeJoin = PARAM_R(uint32_t);

      if (strokeJoin == _stroker._params.strokeJoin())
        return kErrorOk;

      if (B2D_UNLIKELY(strokeJoin >= StrokeJoin::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setStrokeJoin(strokeJoin);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeStartCap: {
      uint32_t startCap = PARAM_R(uint32_t);

      if (startCap == _stroker._params.startCap())
        return kErrorOk;

      if (B2D_UNLIKELY(startCap >= StrokeCap::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setStartCap(startCap);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeEndCap: {
      uint32_t endCap = PARAM_R(uint32_t);

      if (endCap == _stroker._params.endCap())
        return kErrorOk;

      if (B2D_UNLIKELY(endCap >= StrokeCap::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setEndCap(endCap);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeLineCaps: {
      uint32_t caps = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(caps >= StrokeCap::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0) {
        _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
        _saveStroke();
      }

      _stroker._params.setLineCaps(caps);
      _stroker._dirty = true;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeTransformOrder: {
      uint32_t transformOrder = PARAM_R(uint32_t);

      if (B2D_UNLIKELY(transformOrder >= StrokeTransformOrder::kCount))
        return DebugUtils::errored(kErrorInvalidArgument);

      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setTransformOrder(transformOrder);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    default: {
      return DebugUtils::errored(kErrorInvalidArgument);
    }
  }
}

Error RasterContext2DImpl::resetProperty(uint32_t id) noexcept {
  uint32_t contextFlags = _contextFlags;

  switch (id) {
    case Context2D::kPropertyIdContextParams: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      contextFlags &= ~(kSWContext2DNoPaintGlobalCompOp |
                        kSWContext2DNoPaintGlobalAlpha  |
                        kSWContext2DNoPaintFillAlpha    |
                        kSWContext2DNoPaintStrokeAlpha  |
                        kSWContext2DNoPaintStrokeParams |
                        kSWContext2DStateStrokeParams   );

      uint32_t alphaI = _dstInfo._fullAlphaI;
      _coreParams.reset();
      _extParams._globalAlphaI = alphaI;
      _style[Context2D::kStyleSlotFill]._alpha = alphaI;
      _style[Context2D::kStyleSlotStroke]._alpha = alphaI;
      _updateCompOpTable(CompOp::kSrcOver);

      _stroker._params.reset();
      _stroker._dirty = true;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdContextHints: {
      _coreParams._hints.reset();
      _updateCompOpTable(CompOp::kSrcOver);

      _contextFlags = contextFlags & ~kSWContext2DNoPaintGlobalCompOp;
      return kErrorOk;
    }

    case Context2D::kPropertyIdRenderQuality: {
      _coreParams._hints._renderQuality = Context2D::kRenderQualityDefault;
      return kErrorOk;
    }

    case Context2D::kPropertyIdGradientFilter: {
      _coreParams._hints._gradientFilter = Gradient::kFilterDefault;
      return kErrorOk;
    }

    case Context2D::kPropertyIdPatternFilter: {
      _coreParams._hints._patternFilter = Pattern::kFilterDefault;
      return kErrorOk;
    }

    case Context2D::kPropertyIdTolerance: {
      _onBeforeConfigChange();
      _toleranceD = kToleranceDefault;
      _onUpdateTolerance();

      _contextFlags = contextFlags & ~kSWContext2DStateConfig;
      return kErrorOk;
    }

    case Context2D::kPropertyIdCompOp: {
      _coreParams._hints._compOp = CompOp::kSrcOver;
      _updateCompOpTable(CompOp::kSrcOver);

      _contextFlags = contextFlags & ~kSWContext2DNoPaintGlobalCompOp;
      return kErrorOk;
    }

    case Context2D::kPropertyIdAlpha: {
      contextFlags &= ~(kSWContext2DNoPaintGlobalAlpha |
                        kSWContext2DNoPaintFillAlpha   |
                        kSWContext2DNoPaintStrokeAlpha );

      double intAlphaD    = _dstInfo._fullAlphaD;
      double fillAlphaD   = intAlphaD * _coreParams._styleAlpha[Context2D::kStyleSlotFill];
      double strokeAlphaD = intAlphaD * _coreParams._styleAlpha[Context2D::kStyleSlotStroke];

      _coreParams._globalAlpha = 1.0;
      _extParams._globalAlphaI = _dstInfo._fullAlphaI;

      uint32_t fillAlphaI   = uint32_t(Math::iround(fillAlphaD));
      uint32_t strokeAlphaI = uint32_t(Math::iround(strokeAlphaD));

      _style[Context2D::kStyleSlotFill]._alpha = fillAlphaI;
      _style[Context2D::kStyleSlotStroke]._alpha = strokeAlphaI;

      if (!fillAlphaI  ) contextFlags |= kSWContext2DNoPaintFillAlpha;
      if (!strokeAlphaI) contextFlags |= kSWContext2DNoPaintStrokeAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdFillAlpha: {
      contextFlags &= ~kSWContext2DNoPaintFillAlpha;
      uint32_t alphaI = uint32_t(Math::iround(_coreParams._globalAlpha * _dstInfo._fullAlphaD));

      _coreParams._styleAlpha[Context2D::kStyleSlotFill] = 1.0;
      _style[Context2D::kStyleSlotFill]._alpha = alphaI;

      if (!alphaI) contextFlags |= kSWContext2DNoPaintFillAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeAlpha: {
      contextFlags &= ~kSWContext2DNoPaintStrokeAlpha;
      uint32_t alphaI = uint32_t(Math::iround(_coreParams._globalAlpha * _dstInfo._fullAlphaD));

      _coreParams._styleAlpha[Context2D::kStyleSlotStroke] = 1.0;
      _style[Context2D::kStyleSlotStroke]._alpha = alphaI;

      if (!alphaI) contextFlags |= kSWContext2DNoPaintStrokeAlpha;

      _contextFlags = contextFlags;
      return kErrorOk;
    }

    case Context2D::kPropertyIdFillRule: {
      _coreParams._fillRule = FillRule::kDefault;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeParams: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.reset();
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~(kSWContext2DNoPaintStrokeParams | kSWContext2DStateStrokeParams);
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeWidth: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setStrokeWidth(1.0);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~(kSWContext2DNoPaintStrokeParams | kSWContext2DStateStrokeParams);
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeJoin: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setStartCap(StrokeJoin::kDefault);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeMiterLimit: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setMiterLimit(4.0);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~(kSWContext2DNoPaintStrokeParams | kSWContext2DStateStrokeParams);
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeDashOffset: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setDashOffset(1.0);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~(kSWContext2DNoPaintStrokeParams | kSWContext2DStateStrokeParams);
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeDashArray: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params._dashArray.reset();
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~(kSWContext2DNoPaintStrokeParams | kSWContext2DStateStrokeParams);
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeStartCap: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setStartCap(StrokeCap::kDefault);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeEndCap: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setEndCap(StrokeCap::kDefault);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeLineCaps: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setLineCaps(StrokeCap::kDefault);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    case Context2D::kPropertyIdStrokeTransformOrder: {
      if ((contextFlags & kSWContext2DStateStrokeParams) != 0)
        _saveStroke();

      _stroker._params.setLineCaps(StrokeTransformOrder::kDefault);
      _stroker._dirty = true;

      _contextFlags = contextFlags & ~kSWContext2DStateStrokeParams;
      return kErrorOk;
    }

    default: {
      return DebugUtils::errored(kErrorInvalidArgument);
    }
  }
}

#undef PARAM_W
#undef PARAM_R

// ============================================================================
// [b2d::RasterContext2DImpl - Style]
// ============================================================================

RasterFetchData* RasterContext2DImpl::_createFetchData(StyleData& style) noexcept {
  ObjectImpl* objI = style._object->impl();

  const Matrix2D& m = style._adjusted;
  Matrix2D mInv(NoInit);

  if (!Matrix2D::invert(mInv, m)) {
    // TODO: Failed to invert the matrix.
    return nullptr;
  }

  RasterFetchData* fetchData = _fetchPool.alloc();
  if (B2D_UNLIKELY(!fetchData)) return nullptr;

  switch (objI->typeId()) {
    case Any::kTypeIdLinearGradient:
    case Any::kTypeIdRadialGradient:
    case Any::kTypeIdConicalGradient: {
      GradientImpl* gradientI = style._gradient->impl();
      GradientCache* cache = gradientI->ensureCache32();

      if (B2D_LIKELY(cache)) {
        fetchData->initGradient(gradientI, cache, m, mInv, style._styleComponents);
        fetchData->gradientCacheRef = cache->addRef();
        fetchData->refCount = 1;

        return fetchData;
      }

      break;
    }

    case Any::kTypeIdPattern: {
      PatternImpl* patternI = style._pattern->impl();
      ImageImpl* imageI = patternI->_image.impl();

      // Zero area means to cover the whole image.
      IntRect area = patternI->_area;
      if (!area.isValid())
        area.reset(0, 0, imageI->w(), imageI->h());

      fetchData->initTextureAffine(imageI, area, patternI->extend(), style._filter, m, mInv);
      fetchData->imageImplRef = imageI->addRef();
      fetchData->refCount = 1;

      return fetchData;
    }
  }

  _fetchPool.free(fetchData);
  return nullptr;
}

Error RasterContext2DImpl::getStyleType(uint32_t slot, uint32_t& type) const noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);

  type = _style[slot]._styleType;
  return kErrorOk;
}

Error RasterContext2DImpl::getStyleArgb32(uint32_t slot, uint32_t& argb32) const noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);
  const StyleData& style = _style[slot];

  if (style._styleType != Context2D::kStyleTypeSolid)
    return DebugUtils::errored(kErrorInvalidState);

  argb32 = ArgbUtil::convert32From64(style._color().value());
  return kErrorOk;
}

Error RasterContext2DImpl::getStyleArgb64(uint32_t slot, uint64_t& argb64) const noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);
  const StyleData& style = _style[slot];

  if (style._styleType != Context2D::kStyleTypeSolid)
    return DebugUtils::errored(kErrorInvalidState);

  argb64 = style._color().value();
  return kErrorOk;
}

Error RasterContext2DImpl::getStyleObject(uint32_t slot, Object* obj) const noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);
  const StyleData& style = _style[slot];

  if (style._styleType <= Context2D::kStyleTypeSolid)
    return DebugUtils::errored(kErrorInvalidState);

  ObjectImpl* styleI = style._object->impl();
  ObjectImpl* objI = obj->impl();

  uint32_t styleTypeId = styleI->typeId();
  uint32_t objTypeId = objI->typeId();

  if (styleTypeId != objTypeId)
    return DebugUtils::errored(kErrorInvalidState);

  return AnyInternal::replaceImpl(obj, styleI);
}

Error RasterContext2DImpl::setStyleArgb32(uint32_t slot, uint32_t argb32) noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);
  StyleData& style = _style[slot];

  uint32_t contextFlags = _contextFlags;
  uint32_t slotFlags = (kSWContext2DStateBeginStyle | kSWContext2DInfoBeginFetchData) << slot;

  if (contextFlags & slotFlags)
    _willChangeStyle(style, slot);

  uint32_t styleComponents = PixelFlags::kComponentRGB;
  style._color().setValue(ArgbUtil::convert64From32(argb32));

  if (!ArgbUtil::isOpaque32(argb32)) {
    argb32 = PixelUtils::prgb32_8888_from_argb32_8888(argb32);
    styleComponents = PixelFlags::kComponentARGB;
  }

  _contextFlags = contextFlags & ~(slotFlags | (kSWContext2DNoPaintBeginStyle << slot));

  style._styleType = Context2D::kStyleTypeSolid;
  style._styleFormat = SWGlobals::pixelFormatFromComponentFlags(styleComponents);
  style._styleComponents = styleComponents;
  style._styleMaxPfAltId = 0xFF;
  style._solidData.prgb32 = argb32;
  style._fetchData = SWContext2DUtil::solidFetchSentinel();

  return kErrorOk;
}

Error RasterContext2DImpl::setStyleArgb64(uint32_t slot, uint64_t argb64) noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);
  StyleData& style = _style[slot];

  uint32_t contextFlags = _contextFlags;
  uint32_t slotFlags = (kSWContext2DStateBeginStyle | kSWContext2DInfoBeginFetchData) << slot;

  if (contextFlags & slotFlags)
    _willChangeStyle(style, slot);

  uint32_t argb32 = ArgbUtil::convert32From64(argb64);
  uint32_t styleComponents = PixelFlags::kComponentRGB;

  style._color().setValue(argb64);

  if (!ArgbUtil::isOpaque32(argb32)) {
    argb32 = PixelUtils::prgb32_8888_from_argb32_8888(argb32);
    styleComponents = PixelFlags::kComponentARGB;
  }

  _contextFlags = contextFlags & ~(slotFlags | (kSWContext2DNoPaintBeginStyle << slot));

  style._styleType = Context2D::kStyleTypeSolid;
  style._styleFormat = SWGlobals::pixelFormatFromComponentFlags(styleComponents);
  style._styleComponents = styleComponents;
  style._styleMaxPfAltId = 0xFF;
  style._solidData.prgb32 = argb32;
  style._fetchData = SWContext2DUtil::solidFetchSentinel();

  return kErrorOk;
}

Error RasterContext2DImpl::setStyleObject(uint32_t slot, const Object* obj) noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);

  uint32_t contextFlags = _contextFlags;
  uint32_t slotFlags = (kSWContext2DInfoBeginFetchData | kSWContext2DStateBeginStyle) << slot;

  StyleData& style = _style[slot];
  if (contextFlags & slotFlags)
    _willChangeStyle(style, slot);

  contextFlags &= ~(slotFlags | (kSWContext2DNoPaintBeginStyle << slot));
  slotFlags = kSWContext2DInfoBeginFetchData;

  style._packed = 0;
  style._fetchData = nullptr;

  ObjectImpl* styleI = obj->impl();
  switch (styleI->typeId()) {
    case Any::kTypeIdLinearGradient:
    case Any::kTypeIdRadialGradient:
    case Any::kTypeIdConicalGradient: {
      const GradientImpl* gradientI = styleI->as<GradientImpl>();
      const GradientStop* stops = gradientI->_stops;

      size_t size = gradientI->_size;
      uint32_t components = PixelFlags::kComponentRGB;

      style._styleType = Context2D::kStyleTypeGradient;
      style._styleMaxPfAltId = 0xFF;

      if (B2D_UNLIKELY(size == 0)) {
        // Border case - disable this style if the gradient doesn't have any stops.
        slotFlags |= kSWContext2DNoPaintBeginStyle;
      }
      else {
        components = SWContext2DUtil::analyzeGradient32(stops, size);
        if (components & PixelFlags::_kSolid) {
          // Use last color according to SVG spec.
          uint32_t argb32 = PixelUtils::prgb32_8888_from_argb32_8888(stops[size - 1].argb32().value());

          style._styleType = Context2D::kStyleTypeSolid;
          style._solidData.prgb32 = argb32;
          style._fetchData = SWContext2DUtil::solidFetchSentinel();

          components ^= PixelFlags::_kSolid;
        }
      }

      if (gradientI->matrixType() == Matrix2D::kTypeIdentity)
        style._adjusted = finalMatrix();
      else
        Matrix2D::multiply(style._adjusted, gradientI->matrix(), finalMatrix());

      style._styleFormat = SWGlobals::pixelFormatFromComponentFlags(components);
      style._styleComponents = components;
      style._filter = _coreParams._hints._gradientFilter;
      break;
    }

    case Any::kTypeIdPattern: {
      PatternImpl* patternI = styleI->as<PatternImpl>();
      if (B2D_UNLIKELY(patternI->_image.empty()))
        slotFlags |= kSWContext2DNoPaintBeginStyle;

      Matrix2D::multiply(style._adjusted, patternI->matrix(), finalMatrix());
      style._styleType = Context2D::kStyleTypePattern;
      style._styleComponents = patternI->_image.pixelFlags() & PixelFlags::kComponentMask;
      style._styleFormat = patternI->_image.pixelFormat();
      style._styleMaxPfAltId = 0;
      style._filter = _coreParams._hints._patternFilter;
      break;
    }

    default: {
      // TODO: We must return error instead of this.
      B2D_NOT_REACHED();
    }
  }

  uint32_t mAdjustedType = style._adjusted.type();
  if (B2D_UNLIKELY(mAdjustedType >= Matrix2D::kTypeInvalid))
    slotFlags |= kSWContext2DNoPaintBeginStyle;

  _contextFlags = contextFlags | (slotFlags << slot);
  style._adjustedMatrixType = uint8_t(mAdjustedType);
  style._object->_impl = styleI->addRef();

  return kErrorOk;
}

Error RasterContext2DImpl::resetStyle(uint32_t slot) noexcept {
  B2D_ASSERT(slot < Context2D::kStyleSlotCount);

  // TODO: Must reset to None.

  return kErrorOk;
}

// ============================================================================
// [b2d::RasterContext2DImpl - Matrix]
// ============================================================================

Error RasterContext2DImpl::getMatrix(Matrix2D& m) const noexcept {
  m = _userMatrix;
  return kErrorOk;
}

Error RasterContext2DImpl::setMatrix(const Matrix2D& m) noexcept {
  _onBeforeUserMatrixChange();
  _userMatrix = m;
  _onAfterUserMatrixChange();

  return kErrorOk;
}

Error RasterContext2DImpl::matrixOp(uint32_t op, const void* data) noexcept {
  _onBeforeUserMatrixChange();
  _userMatrix._matrixOp(op, data);
  _onAfterUserMatrixChange();

  return kErrorOk;
}

Error RasterContext2DImpl::resetMatrix() noexcept {
  _onBeforeUserMatrixChange();
  _userMatrix.reset();
  _onAfterUserMatrixChange();

  return kErrorOk;
}

Error RasterContext2DImpl::userToMeta() noexcept {
  constexpr uint32_t kUserAndMetaFlags = kSWContext2DStateMetaMatrix |
                                         kSWContext2DStateUserMatrix ;

  if (_contextFlags & kUserAndMetaFlags) {
    SWContext2DState* state = _curState;
    B2D_ASSERT(state != nullptr);

    // Always save both `metaMatrix` and `userMatrix` in case we have to save
    // the current state before we change the matrix. In this case the `altMatrix`
    // of the state would store the current `metaMatrix` and on state restore
    // the final matrix would be recalculated in-place.
    state->_altMatrix = metaMatrix();

    // Don't copy it if it was already saved, we would have copied an altered
    // userMatrix.
    if (_contextFlags & kSWContext2DStateUserMatrix)
      state->_userMatrix = userMatrix();
  }

  _contextFlags &= ~kUserAndMetaFlags;
  _metaMatrix = _finalMatrix;
  _metaMatrixFixed = _finalMatrixFixed;
  _userMatrix.reset();
  _matrixInfo._metaMatrixType = _matrixInfo._finalMatrixType;

  return kErrorOk;
}

// ============================================================================
// [b2d::RasterContext2DImpl - State]
// ============================================================================

// TODO: NOT USED.
void RasterContext2DImpl::_saveClip() noexcept {
  SWContext2DState* state = _curState;
  B2D_ASSERT(state != nullptr);

  uint32_t clipMode = _worker._clipMode;

  state->_finalClipBoxI = finalClipBoxI();
  state->_finalClipBoxD = finalClipBoxD();

  switch (clipMode) {
    case Context2D::kClipModeRect:
      break;

    case Context2D::kClipModeRegion:
      break;

    case Context2D::kClipModeMask:
      // TODO: RasterContext2DImpl - clip-mask.
      break;

    default:
      B2D_NOT_REACHED();
  }
}

void RasterContext2DImpl::_discardStates(SWContext2DState* topState) noexcept {
  SWContext2DState* curState = _curState;
  if (curState == topState) return;

  uint32_t contextFlags = _contextFlags;
  do {
    // NOTE: No need to handle kSWContext2DStateTransform as it doesn't need any
    // dynamically allocated objects to be properly saved / restored.

    if ((contextFlags & kSWContext2DStateClip) == 0) {
      switch (curState->_clipMode) {
        case Context2D::kClipModeRect:
          // Nothing to do here...
          break;

        case Context2D::kClipModeRegion:
          break;

        case Context2D::kClipModeMask:
          // TODO: RasterContext2DImpl - clip-mask.
          break;

        default:
          B2D_NOT_REACHED();
      }
    }

    if ((contextFlags & (kSWContext2DInfoFillFetchData | kSWContext2DStateFillStyle)) == kSWContext2DInfoFillFetchData) {
      uint32_t slot = Context2D::kStyleSlotFill;
      RasterFetchData* fetchData = curState->_style[slot]._fetchData;

      if (SWContext2DUtil::isFetchDataCreated(fetchData))
        _releaseFetchData(fetchData);
      curState->_style[slot]._object->impl()->release();
    }

    if ((contextFlags & (kSWContext2DInfoStrokeFetchData | kSWContext2DStateStrokeStyle)) == kSWContext2DInfoStrokeFetchData) {
      uint32_t slot = Context2D::kStyleSlotStroke;
      RasterFetchData* fetchData = curState->_style[slot]._fetchData;

      if (SWContext2DUtil::isFetchDataCreated(fetchData))
        _releaseFetchData(fetchData);
      curState->_style[slot]._object->impl()->release();
    }

    if ((contextFlags & kSWContext2DStateStrokeParams) == 0) {
      curState->_strokeParams.destroy();
    }

    SWContext2DState* prevState = curState->_prevState;
    contextFlags = curState->_prevContextFlags;

    _statePool.free(curState);
    curState = prevState;
  } while (curState != topState);

  // Make 'topState' the current state.
  _curState = topState;
  _contextFlags = contextFlags;
}

Error RasterContext2DImpl::save(Cookie* cookie) noexcept {
  SWContext2DState* newState = _statePool.alloc();
  if (B2D_UNLIKELY(!newState))
    return DebugUtils::errored(kErrorNoMemory);

  // --------------------------------------------------------------------------
  // [Save]
  // --------------------------------------------------------------------------

  newState->_prevState = _curState;
  newState->_stateId = kUnassignedStateId;
  _curState = newState;

  _onCoreStateSave(newState);
  _contextFlags |= kSWContext2DStateAllFlags;

  if (!cookie)
    return kErrorOk;

  // --------------------------------------------------------------------------
  // [Save With Cookie]
  // --------------------------------------------------------------------------

  // Setup `coookie` if provided.
  uint64_t stateId = generateStateId();
  newState->_stateId = stateId;

  cookie->reset(contextOriginId(), stateId);
  return kErrorOk;
}

Error RasterContext2DImpl::restore(const Cookie* cookie) noexcept {
  SWContext2DState* curState = _curState;
  if (B2D_UNLIKELY(!curState))
    return DebugUtils::errored(kErrorInvalidState);

  // By default there would be only one state to restore if `cookie` was not provided.
  uint32_t n = 1;

  // --------------------------------------------------------------------------
  // [Restore With Cookie]
  // --------------------------------------------------------------------------

  if (cookie) {
    // Verify context origin.
    if (B2D_UNLIKELY(cookie->data0() != contextOriginId()))
      return DebugUtils::errored(kErrorInvalidState);

    // Verify cookie payload and get the number of states we have to restore (if valid).
    n = numStatesToRestore(curState, cookie->data1());
    if (B2D_UNLIKELY(n == 0))
      return DebugUtils::errored(kErrorInvalidState);
  }
  else {
    // A state that has a `stateId` assigned cannot be restored without a matching cookie.
    if (curState->_stateId != kUnassignedStateId)
      return DebugUtils::errored(kErrorInvalidState);
  }

  // --------------------------------------------------------------------------
  // [Restore]
  // --------------------------------------------------------------------------

  for (;;) {
    uint32_t restoreFlags = _contextFlags;
    _onCoreStateRestore(curState);

    if ((restoreFlags & kSWContext2DStateConfig) == 0) {
      _toleranceD = curState->_toleranceD;
      _onUpdateTolerance();
    }

    if ((restoreFlags & kSWContext2DStateClip) == 0) {
      _finalClipBoxI = curState->_finalClipBoxI;
      _finalClipBoxD = curState->_finalClipBoxD;
      _finalClipBoxFixedD.reset(_finalClipBoxD.x0() * fpScaleD(),
                                _finalClipBoxD.y0() * fpScaleD(),
                                _finalClipBoxD.x1() * fpScaleD(),
                                _finalClipBoxD.y1() * fpScaleD());

      switch (curState->_clipMode) {
        case Context2D::kClipModeRect: {
          if (_worker._clipMode == Context2D::kClipModeMask) {
            // TODO: RasterContext2DImpl - clip-mask.
          }

          _worker._clipMode = curState->_clipMode;
          // TODO:
          break;
        }

        case Context2D::kClipModeRegion: {
          if (_worker._clipMode == Context2D::kClipModeMask) {
            // TODO: RasterContext2DImpl - clip-mask.
          }

          _worker._clipMode = curState->_clipMode;

          // TODO:
          break;
        }

        case Context2D::kClipModeMask: {
          // TODO: RasterContext2DImpl - clip-mask.
          break;
        }

        default:
          B2D_NOT_REACHED();
      }
    }

    if ((restoreFlags & kSWContext2DStateFillStyle) == 0) {
      StyleData& dst = _style[Context2D::kStyleSlotFill];
      StyleData& src = curState->_style[Context2D::kStyleSlotFill];

      if (restoreFlags & kSWContext2DInfoFillFetchData)
        _discardStyle(dst);

      dst._packed = src._packed;
      dst._solidData.prgb64 = src._solidData.prgb64;
      dst._fetchData = src._fetchData;

      dst._color() = src._color();
      dst._adjusted = src._adjusted;
    }

    if ((restoreFlags & kSWContext2DStateStrokeStyle) == 0) {
      StyleData& dst = _style[Context2D::kStyleSlotStroke];
      StyleData& src = curState->_style[Context2D::kStyleSlotStroke];

      if (restoreFlags & kSWContext2DInfoStrokeFetchData)
        _discardStyle(dst);

      dst._packed = src._packed;
      dst._solidData.prgb64 = src._solidData.prgb64;
      dst._fetchData = src._fetchData;

      dst._color() = src._color();
      dst._adjusted = src._adjusted;
    }

    if ((restoreFlags & kSWContext2DStateStrokeParams) == 0) {
      // NOTE: This code is unsafe, but since we know that `StrokeParams` is
      // movable it's just fine. We destroy `StrokeParams` first and then move
      // into that destroyed instance params from the state itself.
      _stroker._params.~StrokeParams();
      std::memcpy(&_stroker._params, &curState->_strokeParams, sizeof(StrokeParams));
      _stroker._dirty = true;
    }

    // UserMatrix state is unsed when MetaMatrix and/or UserMatrix were saved.
    if ((restoreFlags & kSWContext2DStateUserMatrix) == 0) {
      _userMatrix = curState->_userMatrix;

      if ((restoreFlags & kSWContext2DStateMetaMatrix) == 0) {
        _metaMatrix = curState->_altMatrix;
        _onUpdateFinalMatrix();
        _onUpdateMetaMatrixFixed();
        _onUpdateFinalMatrixFixed();
      }
      else {
        _finalMatrix = curState->_altMatrix;
        _onUpdateFinalMatrixFixed();
      }
    }

    _curState = curState->_prevState;
    _statePool.free(curState);

    if (--n == 0)
      break;
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::RasterContext2DImpl - Group]
// ============================================================================

Error RasterContext2DImpl::beginGroup() noexcept {
  // TODO:
  return DebugUtils::errored(kErrorNotImplemented);
}

Error RasterContext2DImpl::paintGroup() noexcept {
  // TODO:
  return DebugUtils::errored(kErrorNotImplemented);
}

// ============================================================================
// [b2d::RasterContext2DImpl - Fill]
// ============================================================================

static B2D_INLINE uint32_t RasterContext2DImpl_prepareFill(RasterContext2DImpl* ctxI, FillCmd* fillCmd, StyleData* styleData, uint32_t fillRule, uint32_t nopFlags) noexcept {
  const Alternatives::AltCompOpInfo& altCompOpInfo =
    Alternatives::altCompOpInfo[
      ctxI->simplifiedCompOpFromStyleComponents(styleData->_styleComponents)];

  fillCmd->reset(ctxI->baseSignature(), altCompOpInfo.suggestedCompOp(), styleData->_alpha, fillRule);
  fillCmd->setFetchDataFromStyle(styleData);
  fillCmd->baseSignature().addSrcPixelFormat(altCompOpInfo.suggestedPixelFormat(styleData->_styleFormat, styleData->_styleMaxPfAltId));

  uint32_t combinedFlags = ctxI->_contextFlags | altCompOpInfo.additionalContextFlags();

  // Preferred case.
  if ((combinedFlags & (nopFlags | kSWContext2DFlagCompOpSolid)) == 0)
    return FillCmd::kStatusValid;

  // Nothing to render as something regarding compositing, style, alpha, ... is INVALID or results in NOP.
  if (B2D_UNLIKELY(combinedFlags & nopFlags))
    return FillCmd::kStatusNop;

  // Replace the current fill `style` with a solid color as suggested by `AltCompOpInfo`.
  // TODO: We reset to zero atm to support CLEAR operator, this should be improved.
  fillCmd->_solidData.reset();
  fillCmd->_fetchData = SWContext2DUtil::solidFetchSentinel();

  fillCmd->baseSignature().resetSrcPixelFormat();
  fillCmd->baseSignature().addSrcPixelFormat(altCompOpInfo.suggestedPixelFormat(PixelFormat::kPRGB32));

  return FillCmd::kStatusSolid;
}

static B2D_INLINE Error RasterContext2DImpl_ensureFetchData(RasterContext2DImpl* ctxI, FillCmd* fillCmd) noexcept {
  RasterFetchData* fetchData = fillCmd->_fetchData;

  if (SWContext2DUtil::isFetchDataSolid(fetchData)) {
    fillCmd->baseSignature().addFetchType(Pipe2D::kFetchTypeSolid);
    fillCmd->baseSignature().addSrcPixelFormat(fillCmd->_styleFormat);
    fillCmd->_fetchData = const_cast<RasterFetchData*>(reinterpret_cast<const RasterFetchData*>(&fillCmd->_solidData));
  }
  else {
    if (!fetchData) {
      B2D_ASSERT(fillCmd->_styleData != nullptr);

      fetchData = ctxI->_createFetchData(*fillCmd->_styleData);
      if (B2D_UNLIKELY(!fetchData))
        return DebugUtils::errored(kErrorNoMemory);

      fillCmd->_styleData->_fetchData = fetchData;
    }

    fillCmd->baseSignature().add(fetchData->signaturePart);
    fillCmd->_fetchData = fetchData;
  }

  return kErrorOk;
}

static B2D_INLINE Error bRasterContext2DImplFill(RasterContext2DImpl* ctxI, FillCmd* fillCmd, RasterFiller& fillContext) noexcept {
  RasterWorker& worker = ctxI->_worker;
  Pipe2D::PipeSignature sig = { 0 };

  sig.add(fillCmd->baseSignature());
  sig.add(fillContext.fillSignature());
  sig.addClipMode(Context2D::kClipModeRect);

  fillContext.setFillFunc(ctxI->_pipeRuntime->getFunction(sig.value()));
  return fillContext.doWork(&worker, fillCmd->_fetchData);
}

static B2D_INLINE Error RasterContext2DImpl_fillClippedBoxAA(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const IntBox& box) noexcept {
  RasterFiller fillContext;
  fillContext.initBoxAA8bpc(fillCmd->alpha(), box.x0(), box.y0(), box.x1(), box.y1());

  B2D_PROPAGATE(RasterContext2DImpl_ensureFetchData(ctxI, fillCmd));
  return bRasterContext2DImplFill(ctxI, fillCmd, fillContext);
}

static B2D_INLINE Error RasterContext2DImpl_fillClippedBoxAU(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const IntBox& box) noexcept {
  RasterFiller fillContext;
  fillContext.initBoxAU8bpc24x8(fillCmd->alpha(), box.x0(), box.y0(), box.x1(), box.y1());

  if (!fillContext.isValid())
    return kErrorOk;

  B2D_PROPAGATE(RasterContext2DImpl_ensureFetchData(ctxI, fillCmd));
  return bRasterContext2DImplFill(ctxI, fillCmd, fillContext);
}

static B2D_INLINE Error RasterContext2DImpl_fillClippedEdges(RasterContext2DImpl* ctxI, FillCmd* fillCmd) noexcept {
  // No data or everything was clipped out (no edges at all).
  if (ctxI->_worker._edgeStorage.empty())
    return kErrorOk;

  Error err = RasterContext2DImpl_ensureFetchData(ctxI, fillCmd);
  if (B2D_UNLIKELY(err)) {
    ctxI->_worker._edgeStorage.clear();
    ctxI->_worker._workerZone.reset();
    return err;
  }

  RasterFiller fillContext;
  fillContext.initAnalytic(fillCmd->alpha(), &ctxI->_worker._edgeStorage, fillCmd->fillRule());

  return bRasterContext2DImplFill(ctxI, fillCmd, fillContext);
}

static B2D_NOINLINE Error RasterContext2DImpl_fillUnsafePolyData(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const Matrix2D& m, uint32_t mType, const Point* pts, size_t size) noexcept {
  if (mType <= Matrix2D::kTypeScaling) {
    EdgeSourcePoly2DScale source(EdgeTransformScale(m), pts, size);
    EdgeBuilderFromSource<EdgeSourcePoly2DScale> edgeBuilder(&ctxI->_worker._edgeStorage, &ctxI->_worker._workerZone, ctxI->finalClipBoxFixedD(), ctxI->toleranceFixedSqD(), source);

    B2D_PROPAGATE(edgeBuilder.build());
    B2D_PROPAGATE(edgeBuilder.flush(EdgeBuilderFlags::kFlushAll));
  }
  else {
    EdgeSourcePoly2DAffine source(EdgeTransformAffine(m), pts, size);
    EdgeBuilderFromSource<EdgeSourcePoly2DAffine> edgeBuilder(&ctxI->_worker._edgeStorage, &ctxI->_worker._workerZone, ctxI->finalClipBoxFixedD(), ctxI->toleranceFixedSqD(), source);

    B2D_PROPAGATE(edgeBuilder.build());
    B2D_PROPAGATE(edgeBuilder.flush(EdgeBuilderFlags::kFlushAll));
  }

  return RasterContext2DImpl_fillClippedEdges(ctxI, fillCmd);
}

static B2D_NOINLINE Error RasterContext2DImpl_fillUnsafePathData(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const Matrix2D& m, uint32_t mType, const Point* vtxData, const uint8_t* cmdData, size_t size) noexcept {
  if (mType <= Matrix2D::kTypeScaling) {
    EdgeSourcePath2DScale source(EdgeTransformScale(m), vtxData, cmdData, size);
    EdgeBuilderFromSource<EdgeSourcePath2DScale> edgeBuilder(&ctxI->_worker._edgeStorage, &ctxI->_worker._workerZone, ctxI->finalClipBoxFixedD(), ctxI->toleranceFixedSqD(), source);

    B2D_PROPAGATE(edgeBuilder.build());
    B2D_PROPAGATE(edgeBuilder.flush(EdgeBuilderFlags::kFlushAll));
  }
  else {
    EdgeSourcePath2DAffine source(EdgeTransformAffine(m), vtxData, cmdData, size);
    EdgeBuilderFromSource<EdgeSourcePath2DAffine> edgeBuilder(&ctxI->_worker._edgeStorage, &ctxI->_worker._workerZone, ctxI->finalClipBoxFixedD(), ctxI->toleranceFixedSqD(), source);

    B2D_PROPAGATE(edgeBuilder.build());
    B2D_PROPAGATE(edgeBuilder.flush(EdgeBuilderFlags::kFlushAll));
  }

  return RasterContext2DImpl_fillClippedEdges(ctxI, fillCmd);
}

static B2D_INLINE Error RasterContext2DImpl_fillUnsafePath(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const Matrix2D& m, uint32_t mType, const Path2D& path) noexcept {
  const Path2DImpl* pathI = path.impl();
  return RasterContext2DImpl_fillUnsafePathData(ctxI, fillCmd, m, mType, pathI->vertexData(), pathI->commandData(), pathI->size());
}

static B2D_INLINE Error RasterContext2DImpl_fillUnsafeBox(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const Matrix2D& m, uint32_t mType, const Box& box) noexcept {
  if (mType <= Matrix2D::kTypeSwap) {
    Box finalBox;
    m.mapBox(finalBox, box);

    if (!Box::intersect(finalBox, finalBox, ctxI->finalClipBoxFixedD()))
      return kErrorOk;

    IntBox finalBoxFixed {
      Math::itrunc(finalBox._x0),
      Math::itrunc(finalBox._y0),
      Math::itrunc(finalBox._x1),
      Math::itrunc(finalBox._y1)
    };
    return RasterContext2DImpl_fillClippedBoxAU(ctxI, fillCmd, finalBoxFixed);
  }
  else {
    Point poly[4] = {
      Point { box.x0(), box.y0() },
      Point { box.x1(), box.y0() },
      Point { box.x1(), box.y1() },
      Point { box.x0(), box.y1() }
    };
    return RasterContext2DImpl_fillUnsafePolyData(ctxI, fillCmd, m, mType, poly, 4);
  }
}

static B2D_INLINE Error RasterContext2DImpl_fillUnsafeRectI(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const IntRect& rect) noexcept {
  int rw = rect.width();
  int rh = rect.height();

  if (!ctxI->isIntTranslation()) {
    // Clipped out.
    if ((rw <= 0) | (rh <= 0))
      return kErrorOk;
    return RasterContext2DImpl_fillUnsafeBox(ctxI, fillCmd, ctxI->finalMatrixFixed(), ctxI->finalMatrixType(), Box(rect));
  }

  if (B2D_ARCH_BITS >= 64) {
    // We don't have to worry about 32-bit overflow if we use 64-bit arithmetic.
    int64_t x0 = int64_t(rect.x()) + int64_t(ctxI->_intTranslation.x());
    int64_t y0 = int64_t(rect.y()) + int64_t(ctxI->_intTranslation.y());
    int64_t x1 = int64_t(rw) + x0;
    int64_t y1 = int64_t(rh) + y0;

    x0 = Math::max<int64_t>(x0, ctxI->_finalClipBoxI.x0());
    y0 = Math::max<int64_t>(y0, ctxI->_finalClipBoxI.y0());
    x1 = Math::min<int64_t>(x1, ctxI->_finalClipBoxI.x1());
    y1 = Math::min<int64_t>(y1, ctxI->_finalClipBoxI.y1());

    // Clipped out or invalid IntRect.
    if ((x0 >= x1) | (y0 >= y1))
      return kErrorOk;

    return RasterContext2DImpl_fillClippedBoxAA(ctxI, fillCmd, IntBox { int(x0), int(y0), int(x1), int(y1) });
  }
  else {
    int rx = rect.x0();
    int ry = rect.y0();

    int x0 = Math::max(rx, ctxI->_intClientBox.x0());
    int y0 = Math::max(ry, ctxI->_intClientBox.y0());

    // Must be unsigned otherwise it could trigger undefined behavior (signed
    // overflow) based on user input, which is something we should never allow.
    unsigned xMoved = unsigned(x0) - unsigned(rx);
    unsigned yMoved = unsigned(y0) - unsigned(ry);

    // Clipped out or invalid IntRect.
    if ( (rx >= ctxI->_intClientBox.x1()) | (rh <= 0) | (xMoved >= unsigned(rw)) |
         (ry >= ctxI->_intClientBox.y1()) | (rw <= 0) | (yMoved >= unsigned(rh)) )
      return kErrorOk;

    x0 += ctxI->_intTranslation.x();
    y0 += ctxI->_intTranslation.y();

    rw -= xMoved;
    rh -= yMoved;

    return RasterContext2DImpl_fillClippedBoxAA(ctxI, fillCmd, IntBox { x0, y0, x0 + rw, y0 + rh });
  }
}

Error RasterContext2DImpl::fillArg(uint32_t argId, const void* data) noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotFill], fillRule(), kSWContext2DDisableFillOpFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  switch (argId) {
    case kGeomArgBox:
      fillCmd.setFillRule(FillRule::_kPickAny);
      return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), *static_cast<const Box*>(data));

    case kGeomArgRect:
      fillCmd.setFillRule(FillRule::_kPickAny);
      return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), Box(*static_cast<const Rect*>(data)));

    case kGeomArgIntBox:
      fillCmd.setFillRule(FillRule::_kPickAny);
      return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), Box(*static_cast<const IntBox*>(data)));

    case kGeomArgIntRect:
      fillCmd.setFillRule(FillRule::_kPickAny);
      return RasterContext2DImpl_fillUnsafeRectI(this, &fillCmd, *static_cast<const IntRect*>(data));

    case kGeomArgPolygon:
    case kGeomArgPolyline: {
      const CArray<Point>* array = static_cast<const CArray<Point>*>(data);
      if (B2D_UNLIKELY(array->size() < 3))
        return kErrorOk;

      return RasterContext2DImpl_fillUnsafePolyData(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), array->data(), array->size());
    }

    case kGeomArgPath2D: {
      const Path2D* path = static_cast<const Path2D*>(data);
      if (B2D_UNLIKELY(path->empty()))
        return kErrorOk;

      return RasterContext2DImpl_fillUnsafePath(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), *path);

    }
    default: {
      Path2D* path = &_worker._tmpPath[0];
      path->reset();
      B2D_PROPAGATE(path->_addArg(argId, data, nullptr, Path2D::kDirCW));

      return RasterContext2DImpl_fillUnsafePath(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), *path);
    }
  }
}

Error RasterContext2DImpl::fillAll() noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotFill], FillRule::_kPickAny, kSWContext2DDisableFillAllFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  return RasterContext2DImpl_fillClippedBoxAA(this, &fillCmd, finalClipBoxI());
}

Error RasterContext2DImpl::fillRectD(const Rect& rect) noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotFill], FillRule::_kPickAny, kSWContext2DDisableFillOpFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), Box(rect));
}

Error RasterContext2DImpl::fillRectI(const IntRect& rect) noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotFill], FillRule::_kPickAny, kSWContext2DDisableFillOpFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  return RasterContext2DImpl_fillUnsafeRectI(this, &fillCmd, rect);
}

Error RasterContext2DImpl::fillText(const Point& dst, const Font& font, const void* text, size_t size, uint32_t encoding) noexcept {
  if (font.isNone())
    return DebugUtils::errored(kErrorFontNotInitialized);

  Error err = kErrorOk;
  switch (encoding) {
    case Unicode::kEncodingUtf8 : err = _textBuffer.setUtf8Text(static_cast<const char*>(text), size); break;
    case Unicode::kEncodingUtf16: err = _textBuffer.setUtf16Text(static_cast<const uint16_t*>(text), size); break;
    case Unicode::kEncodingUtf32: err = _textBuffer.setUtf32Text(static_cast<const uint32_t*>(text), size); break;
    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  if (_textBuffer.empty())
    return kErrorOk;

  b2d::GlyphEngine glyphEngine;
  B2D_PROPAGATE(glyphEngine.createFromFont(font));
  B2D_PROPAGATE(_textBuffer.shape(glyphEngine));

  return fillGlyphRun(dst, font, _textBuffer.glyphRun());
}

Error RasterContext2DImpl::fillGlyphRun(const Point& dst, const Font& font, const GlyphRun& glyphRun) noexcept {
  if (glyphRun.empty())
    return kErrorOk;

  GlyphOutlineDecoder decoder;
  B2D_PROPAGATE(decoder.createFromFont(font));

  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotFill], FillRule::kNonZero, kSWContext2DDisableFillAllFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  Path2D& tmpPath = _worker._tmpPath[2];
  Error err = kErrorOk;

  Matrix2D m(finalMatrixFixed());
  m.translate(dst);

  EdgeSourcePath2D<> source { EdgeTransformNone {} };
  EdgeBuilderFromSource<EdgeSourcePath2D<>> edgeBuilder(&_worker._edgeStorage, &_worker._workerZone, finalClipBoxFixedD(), toleranceFixedSqD(), source);

  GlyphRunClosure closure;
  closure.ctxI = this;
  closure.edgeBuilder = &edgeBuilder;

  tmpPath.reset();
  decoder.decodeGlyphRun(glyphRun, m, tmpPath, [](Path2D& path, const GlyphOutlineSinkInfo& info, void* closure_) noexcept -> Error {
    B2D_UNUSED(info);

    GlyphRunClosure* closure = static_cast<GlyphRunClosure*>(closure_);
    EdgeBuilderFromSource<EdgeSourcePath2D<>>* edgeBuilder = closure->edgeBuilder;

    edgeBuilder->source().reset(path);
    Error err = edgeBuilder->build();

    path.reset();
    return err;
  }, &closure);

  if (!err)
    err = edgeBuilder.flush(EdgeBuilderFlags::kFlushAll);

  if (B2D_UNLIKELY(err)) {
    _worker._edgeStorage.clear();
    _worker._workerZone.reset();
    return err;
  }

  return RasterContext2DImpl_fillClippedEdges(this, &fillCmd);
}

// ============================================================================
// [b2d::RasterContext2DImpl - Stroke]
// ============================================================================

static B2D_INLINE Error RasterContext2DImpl_strokeUnsafePath(RasterContext2DImpl* ctxI, FillCmd* fillCmd, const Path2D& path) noexcept {
  Path2D* strokedPath = &ctxI->_worker._tmpPath[0];
  const Matrix2D* m = &ctxI->_finalMatrixFixed;
  uint32_t mType = ctxI->finalMatrixType();

  if (ctxI->_stroker._params.transformOrder() == StrokeTransformOrder::kAfter) {
    strokedPath->reset();
    B2D_PROPAGATE(ctxI->_stroker.stroke(*strokedPath, path));
  }
  else {
    // TODO: Not really super optimized.. :(
    Path2D& tmp0 = ctxI->_worker._tmpPath[0];
    Path2D& tmp2 = ctxI->_worker._tmpPath[2];
    B2D_PROPAGATE(ctxI->userMatrix().mapPath(tmp0, path));

    tmp2.reset();
    B2D_PROPAGATE(ctxI->_stroker.stroke(tmp2, tmp0));

    strokedPath = &tmp2;
    m = &ctxI->_metaMatrixFixed;
    mType = ctxI->metaMatrixType();
  }

  return RasterContext2DImpl_fillUnsafePath(ctxI, fillCmd, *m, mType, *strokedPath);
}

Error RasterContext2DImpl::strokeArg(uint32_t argId, const void* data) noexcept {
  if (argId == kGeomArgRect)
    return strokeRectD(*static_cast<const Rect*>(data));

  if (argId == kGeomArgIntRect)
    return strokeRectI(*static_cast<const IntRect*>(data));

  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotStroke], FillRule::kNonZero, kSWContext2DDisableStrokeFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  Path2D* path;
  if (argId == kGeomArgPath2D) {
    path = const_cast<Path2D*>(static_cast<const Path2D*>(data));
  }
  else {
    path = &_worker._tmpPath[2];
    path->reset();
    B2D_PROPAGATE(path->_addArg(argId, data, nullptr, Path2D::kDirCW));
  }

  return RasterContext2DImpl_strokeUnsafePath(this, &fillCmd, *path);
}

Error RasterContext2DImpl::strokeRectD(const Rect& rect) noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotStroke], FillRule::kNonZero, kSWContext2DDisableStrokeFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  Path2D& path = _worker._tmpPath[2];
  path.reset();
  B2D_PROPAGATE(path.addRect(rect));

  return RasterContext2DImpl_strokeUnsafePath(this, &fillCmd, path);
}

Error RasterContext2DImpl::strokeRectI(const IntRect& rect) noexcept {
  FillCmd fillCmd;
  if (RasterContext2DImpl_prepareFill(this, &fillCmd, &_style[Context2D::kStyleSlotStroke], FillRule::kNonZero, kSWContext2DDisableStrokeFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  Path2D& path = _worker._tmpPath[2];
  path.reset();
  B2D_PROPAGATE(path.addRect(rect));

  return RasterContext2DImpl_strokeUnsafePath(this, &fillCmd, path);
}

// ============================================================================
// [b2d::RasterContext2DImpl - Blit]
// ============================================================================

static B2D_INLINE uint32_t RasterContext2DImpl_prepareBlit(RasterContext2DImpl* ctxI, FillCmd* fillCmd, RasterFetchData* localFetchData, uint32_t alpha, PixelFormatInfo pixelFormatInfo, uint32_t fillRule, uint32_t nopFlags) noexcept {
  const Alternatives::AltCompOpInfo& altCompOpInfo =
    Alternatives::altCompOpInfo[
      ctxI->simplifiedCompOpFromStyleComponents(pixelFormatInfo.pixelFlags() & PixelFlags::kComponentMask)];

  fillCmd->reset(ctxI->baseSignature(), altCompOpInfo.suggestedCompOp(), alpha, fillRule);
  fillCmd->setFetchDataFromLocal(localFetchData);
  fillCmd->_baseSignature.addSrcPixelFormat(pixelFormatInfo.pixelFormat());

  // Preferred case.
  uint32_t combinedFlags = ctxI->_contextFlags | altCompOpInfo.additionalContextFlags();
  if ((combinedFlags & (nopFlags | kSWContext2DFlagCompOpSolid)) == 0)
    return FillCmd::kStatusValid;

  // Nothing to render as something regarding compositing, style, alpha, ... is INVALID or results in NOP.
  if (B2D_UNLIKELY(combinedFlags & nopFlags))
    return FillCmd::kStatusNop;

  // Replace the current fill `style` with a solid color as suggested by `AltCompOpInfo`.
  // TODO: We reset to zero atm to support CLEAR operator, this should be improved.
  fillCmd->_solidData.reset();
  fillCmd->_fetchData = SWContext2DUtil::solidFetchSentinel();
  fillCmd->_baseSignature.resetSrcPixelFormat();
  fillCmd->_baseSignature.addSrcPixelFormat(0);

  return FillCmd::kStatusSolid;
}

Error RasterContext2DImpl::blitImagePD(const Point& dst, const Image& src, const IntRect* srcArea) noexcept {
  const ImageImpl* srcI = src.impl();
  int srcX = 0;
  int srcY = 0;
  int srcW = srcI->_size.width();
  int srcH = srcI->_size.height();

  if (srcArea) {
    unsigned maxW = unsigned(srcW) - unsigned(srcArea->x());
    unsigned maxH = unsigned(srcH) - unsigned(srcArea->y());

    if ( (maxW > unsigned(srcW)) | (unsigned(srcArea->w()) > maxW) |
         (maxH > unsigned(srcH)) | (unsigned(srcArea->h()) > maxH) )
      return DebugUtils::errored(kErrorInvalidArgument);

    srcX = srcArea->x();
    srcY = srcArea->y();
    srcW = srcArea->w();
    srcH = srcArea->h();
  }

  FillCmd fillCmd;
  RasterFetchData fetchData;

  uint32_t status = RasterContext2DImpl_prepareBlit(this, &fillCmd, &fetchData, _extParams._globalAlphaI, srcI->_pfi, FillRule::_kPickAny, kSWContext2DDisableBlitFlags);
  if (status <= FillCmd::kStatusSolid) {
    if (status == FillCmd::kStatusNop)
      return kErrorOk;

    Box finalBox { dst.x(), dst.y(), dst.x() + double(srcW), dst.y() + double(srcH) };
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
  else if (finalMatrixType() <= Matrix2D::kTypeTranslation) {
    double startX = dst.x() * _finalMatrixFixed.m00() + _finalMatrixFixed.m20();
    double startY = dst.y() * _finalMatrixFixed.m11() + _finalMatrixFixed.m21();

    double dx0 = Math::max(startX, _finalClipBoxFixedD.x0());
    double dy0 = Math::max(startY, _finalClipBoxFixedD.y0());
    double dx1 = Math::min(startX + double(srcW) * _finalMatrixFixed.m00(), _finalClipBoxFixedD.x1());
    double dy1 = Math::min(startY + double(srcH) * _finalMatrixFixed.m11(), _finalClipBoxFixedD.y1());

    // Clipped out, invalid coordinates, or empty `srcArea`.
    if (!((dx0 < dx1) & (dy0 < dy1)))
      return kErrorOk;

    int64_t startFx = Math::i64floor(startX);
    int64_t startFy = Math::i64floor(startY);

    int ix0 = Math::itrunc(dx0);
    int iy0 = Math::itrunc(dy0);
    int ix1 = Math::itrunc(dx1);
    int iy1 = Math::itrunc(dy1);

    if (!((startFx | startFy) & fpMaskI())) {
      // Pixel aligned blit. At this point we still don't know whether the area where
      // the pixels will be composited is aligned, but we for sure know that the pixels
      // of `src` image don't require any filtering.
      int x0 = (ix0            ) >> fpShiftI();
      int y0 = (iy0            ) >> fpShiftI();
      int x1 = (ix1 + fpMaskI()) >> fpShiftI();
      int y1 = (iy1 + fpMaskI()) >> fpShiftI();

      srcX += x0 - int(startFx >> fpShiftI());
      srcY += y0 - int(startFy >> fpShiftI());

      fetchData.initTextureBlitAA(srcI, IntRect { srcX, srcY, x1 - x0, y1 - y0 });
      return RasterContext2DImpl_fillClippedBoxAU(this, &fillCmd, IntBox { ix0, iy0, ix1, iy1 });
    }
    else {
      fetchData.initTextureFxFy(srcI, IntRect { srcX, srcY, srcW, srcH }, Pattern::kExtendReflect, _coreParams._hints._patternFilter, startFx, startFy);
      return RasterContext2DImpl_fillClippedBoxAU(this, &fillCmd, IntBox { ix0, iy0, ix1, iy1 });
    }
  }
  else {
    Matrix2D m(finalMatrix());
    Matrix2D mInv(NoInit);

    m.translate(dst.x(), dst.y());
    if (!Matrix2D::invert(mInv, m))
      return kErrorOk;

    IntRect srcRect = { srcX, srcY, srcW, srcH };
    fetchData.initTextureAffine(srcI, srcRect, Pattern::kExtendReflect, _coreParams._hints._patternFilter, m, mInv);

    Box finalBox { dst.x(), dst.y(), dst.x() + double(srcW), dst.y() + double(srcH) };
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
}

Error RasterContext2DImpl::blitImagePI(const IntPoint& dst, const Image& src, const IntRect* srcArea) noexcept {
  if (!isIntTranslation())
    return blitImagePD(Point(dst), src, srcArea);

  const ImageImpl* srcI = src.impl();
  int srcX = 0;
  int srcY = 0;
  int srcW = srcI->_size.width();
  int srcH = srcI->_size.height();

  if (srcArea) {
    unsigned maxW = unsigned(srcW) - unsigned(srcArea->x());
    unsigned maxH = unsigned(srcH) - unsigned(srcArea->y());

    if ( (maxW > unsigned(srcW)) | (unsigned(srcArea->w()) > maxW) |
         (maxH > unsigned(srcH)) | (unsigned(srcArea->h()) > maxH) )
      return DebugUtils::errored(kErrorInvalidArgument);

    srcX = srcArea->x();
    srcY = srcArea->y();
    srcW = srcArea->w();
    srcH = srcArea->h();
  }

  IntBox dstBox;
  IntRect srcRect;

  if (B2D_ARCH_BITS >= 64) {
    // We don't have to worry about 32-bit overflow if we use 64-bit arithmetic.
    int64_t dx = int64_t(dst.x());
    int64_t dy = int64_t(dst.y());

    // Use 64-bit ints in 64-bit mode as they make things much easier and won't overflow.
    int64_t x0 = dx + int64_t(_intTranslation.x());
    int64_t y0 = dy + int64_t(_intTranslation.y());
    int64_t x1 = x0 + int64_t(unsigned(srcW));
    int64_t y1 = y0 + int64_t(unsigned(srcH));

    x0 = Math::max<int64_t>(x0, _finalClipBoxI.x0());
    y0 = Math::max<int64_t>(y0, _finalClipBoxI.y0());
    x1 = Math::min<int64_t>(x1, _finalClipBoxI.x1());
    y1 = Math::min<int64_t>(y1, _finalClipBoxI.y1());

    // Clipped out or zero srcW/srcH.
    if ((x0 >= x1) | (y0 >= y1))
      return kErrorOk;

    srcX += int(x0 - dx);
    srcY += int(y0 - dy);

    dstBox.reset(int(x0), int(y0), int(x1), int(y1));
    srcRect.reset(srcX, srcY, int(x1 - x0), int(y1 - y0));
  }
  else {
    int dx = dst.x();
    int dy = dst.y();

    int x0 = Math::max(dx, _intClientBox.x0());
    int y0 = Math::max(dy, _intClientBox.y0());

    // Must be unsigned otherwise it could trigger undefined behavior (signed
    // overflow) based on user input, which is something we should never allow.
    unsigned xMoved = unsigned(x0) - unsigned(dx);
    unsigned yMoved = unsigned(y0) - unsigned(dy);

    // Clipped out or zero `srcArea`.
    if ( (dx >= _intClientBox.x1()) | (xMoved >= unsigned(srcW)) |
         (dy >= _intClientBox.y1()) | (yMoved >= unsigned(srcH)) )
      return kErrorOk;

    x0 += _intTranslation.x();
    y0 += _intTranslation.y();

    srcX += xMoved;
    srcY += yMoved;
    srcW -= xMoved;
    srcH -= yMoved;

    dstBox.reset(x0, y0, x0 + srcW, y0 + srcH);
    srcRect.reset(srcX, srcY, srcW, srcH);
  }

  FillCmd fillCmd;
  RasterFetchData fetchData;
  fetchData.initTextureBlitAA(srcI, srcRect);

  if (RasterContext2DImpl_prepareBlit(this, &fillCmd, &fetchData, _extParams._globalAlphaI, srcI->_pfi, FillRule::_kPickAny, kSWContext2DDisableBlitFlags) == FillCmd::kStatusNop)
    return kErrorOk;

  return RasterContext2DImpl_fillClippedBoxAA(this, &fillCmd, dstBox);
}

Error RasterContext2DImpl::blitImageRD(const Rect& rect, const Image& src, const IntRect* srcArea) noexcept {
  const ImageImpl* srcI = src.impl();
  int srcX = 0;
  int srcY = 0;
  int srcW = srcI->_size.width();
  int srcH = srcI->_size.height();

  if (srcArea) {
    unsigned maxW = unsigned(srcW) - unsigned(srcArea->x());
    unsigned maxH = unsigned(srcH) - unsigned(srcArea->y());

    if ( (maxW > unsigned(srcW)) | (unsigned(srcArea->w()) > maxW) |
         (maxH > unsigned(srcH)) | (unsigned(srcArea->h()) > maxH) )
      return DebugUtils::errored(kErrorInvalidArgument);

    srcX = srcArea->x();
    srcY = srcArea->y();
    srcW = srcArea->w();
    srcH = srcArea->h();
  }

  FillCmd fillCmd;
  RasterFetchData fetchData;

  uint32_t status = RasterContext2DImpl_prepareBlit(this, &fillCmd, &fetchData, _extParams._globalAlphaI, srcI->_pfi, FillRule::_kPickAny, kSWContext2DDisableBlitFlags);
  Box finalBox = Box(rect);

  if (status <= FillCmd::kStatusSolid) {
    if (status == FillCmd::kStatusNop)
      return kErrorOk;
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
  else {
    Matrix2D m(rect.w() / double(srcW), 0.0,
               0.0, rect.h() / double(srcH),
               rect.x(), rect.y());
    m.multiply(finalMatrix());

    Matrix2D mInv(NoInit);
    if (!Matrix2D::invert(mInv, m))
      return kErrorOk;

    IntRect srcRect { srcX, srcY, srcW, srcH };
    fetchData.initTextureAffine(srcI, srcRect, Pattern::kExtendReflect, _coreParams._hints._patternFilter, m, mInv);
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
}

Error RasterContext2DImpl::blitImageRI(const IntRect& rect, const Image& src, const IntRect* srcArea) noexcept {
  const ImageImpl* srcI = src.impl();
  int srcX = 0;
  int srcY = 0;
  int srcW = srcI->_size.width();
  int srcH = srcI->_size.height();

  if (srcArea) {
    unsigned maxW = unsigned(srcW) - unsigned(srcArea->x());
    unsigned maxH = unsigned(srcH) - unsigned(srcArea->y());

    if ( (maxW > unsigned(srcW)) | (unsigned(srcArea->w()) > maxW) |
         (maxH > unsigned(srcH)) | (unsigned(srcArea->h()) > maxH) )
      return DebugUtils::errored(kErrorInvalidArgument);

    srcX = srcArea->x();
    srcY = srcArea->y();
    srcW = srcArea->w();
    srcH = srcArea->h();
  }

  FillCmd fillCmd;
  RasterFetchData fetchData;

  uint32_t status = RasterContext2DImpl_prepareBlit(this, &fillCmd, &fetchData, _extParams._globalAlphaI, srcI->_pfi, FillRule::_kPickAny, kSWContext2DDisableBlitFlags);
  Box finalBox = Box(rect);

  if (status <= FillCmd::kStatusSolid) {
    if (status == FillCmd::kStatusNop)
      return kErrorOk;
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
  else {
    Matrix2D m(double(rect.w()) / double(srcW), 0.0,
               0.0, double(rect.h()) / double(srcH),
               double(rect.x()), double(rect.y()));
    m.multiply(finalMatrix());

    Matrix2D mInv(NoInit);
    if (!Matrix2D::invert(mInv, m))
      return kErrorOk;

    IntRect srcRect { srcX, srcY, srcW, srcH };
    fetchData.initTextureAffine(srcI, srcRect, Pattern::kExtendReflect, _coreParams._hints._patternFilter, m, mInv);
    return RasterContext2DImpl_fillUnsafeBox(this, &fillCmd, finalMatrixFixed(), finalMatrixType(), finalBox);
  }
}

B2D_END_SUB_NAMESPACE
