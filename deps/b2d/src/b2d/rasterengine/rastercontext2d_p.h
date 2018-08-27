// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_RASTERCONTEXT2D_P_H
#define _B2D_RASTERENGINE_RASTERCONTEXT2D_P_H

// [Dependencies]
#include "../core/argb.h"
#include "../core/context2d.h"
#include "../core/fill.h"
#include "../core/gradient.h"
#include "../core/matrix2d.h"
#include "../core/membuffer.h"
#include "../core/path2d.h"
#include "../core/pathflatten.h"
#include "../core/region.h"
#include "../core/stroke.h"
#include "../core/wrap.h"
#include "../core/zone.h"
#include "../core/zonepool.h"
#include "../rasterengine/analyticrasterizer_p.h"
#include "../rasterengine/edgebuilder_p.h"
#include "../rasterengine/edgesource_p.h"
#include "../rasterengine/edgestorage_p.h"
#include "../rasterengine/rasterfetchdata_p.h"
#include "../rasterengine/rasterfiller_p.h"
#include "../rasterengine/rasterworker_p.h"
#include "../text/textbuffer.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class RasterContext2DImpl;
struct SWContext2DState;

// Must be the highest possible value the state can hold.
static constexpr uint64_t kUnassignedStateId = std::numeric_limits<uint64_t>::max();

static constexpr double kToleranceMinimum = 0.01;
static constexpr double kToleranceDefault = 0.20;
static constexpr double kToleranceMaximum = 0.50;

// ============================================================================
// [b2d::kSWContext2DFlags]
// ============================================================================

//! Internal SWContext2D flags:
//!
//!   - `NoPaint` - used to describe that there will be nothing paint regardless
//!     of the paint command.
//!
//!     If one more no-paint flag is set each painting command will be terminated
//!     before the parameters are evaluated. It's one of the fastest checks
//!     available in the `RasterContext2DImpl`.
//!
//!   - `Info` - informative flags contain some precalculated values that are
//!     handy when determining some code paths.
//!
//!   - `State` - describe which states must be saved to `SWContext2DState` in
//!     order to modify them. Used by `save()`, `restore()` and by all other
//!     functions that manipulate the painter state. Initially all state flags
//!     are unset.
enum kSWContext2DFlags {
  // --------------------------------------------------------------------------
  // [Bad Flags]
  // --------------------------------------------------------------------------

  kSWContext2DNoPaintMetaMatrix   = 0x00000001u, //!< Meta matrix is invalid.
  kSWContext2DNoPaintUserMatrix   = 0x00000002u, //!< User matrix is invalid.
  kSWContext2DNoPaintUserClip     = 0x00000004u, //!< User region is empty.
  kSWContext2DNoPaintUserMask     = 0x00000008u, //!< User mask is empty.
  kSWContext2DNoPaintGlobalCompOp = 0x00000020u, //!< Compositing operator is NOP.
  kSWContext2DNoPaintGlobalAlpha  = 0x00000040u, //!< Global alpha is zero.
  kSWContext2DNoPaintFillAlpha    = 0x00000080u, //!< Fill alpha is zero.
  kSWContext2DNoPaintStrokeAlpha  = 0x00000100u, //!< Stroke alpha is zero.
  kSWContext2DNoPaintBeginStyle   = 0x00000200u, //!< Begin fill and stroke style..
  kSWContext2DNoPaintFillStyle    = 0x00000200u, //!< Fill style is invalid or none.
  kSWContext2DNoPaintStrokeStyle  = 0x00000400u, //!< Stroke style is invalid or none.
  kSWContext2DNoPaintStrokeParams = 0x00000800u, //!< One or more stroke parameter is invalid.
  kSWContext2DNoPaintFatal        = 0x00001000u, //!< Painting is disabled because of fatal error.
  kSWContext2DNoPaintAllFlags     = 0x00001FFFu, //!< All no-paint flags.

  // --------------------------------------------------------------------------
  // [Info]
  // --------------------------------------------------------------------------

  kSWContext2DInfoBeginFetchData  = 0x00002000u, //!< Begin fill & stroke fetch data.
  kSWContext2DInfoFillFetchData   = 0x00002000u, //!< Fill style is gradient or pattern.
  kSWContext2DInfoStrokeFetchData = 0x00004000u, //!< Stroke style is gradient or pattern.

  kSWContext2DIsIntTranslation    = 0x00008000u, //!< Final matrix is just a scale by `fpScaleD()` and integral translation.
  kSWContext2DFlagCompOpSolid     = 0x00010000u, //!< Source of compositing operator doesn't matter or is always solid.

  // --------------------------------------------------------------------------
  // [State Flags]
  // --------------------------------------------------------------------------

  kSWContext2DStateClip           = 0x01000000u, //!< Clip state.
  kSWContext2DStateConfig         = 0x02000000u, //!< Configuration (tolerance).
  kSWContext2DStateBeginStyle     = 0x04000000u, //!< Begin of fill and stroke style state.
  kSWContext2DStateFillStyle      = 0x04000000u, //!< Fill style state.
  kSWContext2DStateStrokeStyle    = 0x08000000u, //!< Stroke style state.
  kSWContext2DStateStrokeParams   = 0x10000000u, //!< Stroke params state.
  kSWContext2DStateMetaMatrix     = 0x20000000u, //!< Meta matrix state.
  kSWContext2DStateUserMatrix     = 0x40000000u, //!< User matrix state.
  kSWContext2DStateAllFlags       = 0xFF000000u, //!< All states' flags.

  // --------------------------------------------------------------------------
  // [Combinations]
  // --------------------------------------------------------------------------

  kSWContext2DCompOpFlags         = kSWContext2DNoPaintGlobalCompOp |
                                    kSWContext2DFlagCompOpSolid     ,

  kSWContext2DDisableFillAllFlags = kSWContext2DNoPaintUserClip     |
                                    kSWContext2DNoPaintUserMask     |
                                    kSWContext2DNoPaintGlobalCompOp |
                                    kSWContext2DNoPaintGlobalAlpha  |
                                    kSWContext2DNoPaintFillAlpha    |
                                    kSWContext2DNoPaintFillStyle    |
                                    kSWContext2DNoPaintFatal        ,

  kSWContext2DDisableFillOpFlags  = kSWContext2DNoPaintMetaMatrix   |
                                    kSWContext2DNoPaintUserClip     |
                                    kSWContext2DNoPaintUserMask     |
                                    kSWContext2DNoPaintUserMatrix   |
                                    kSWContext2DNoPaintGlobalCompOp |
                                    kSWContext2DNoPaintGlobalAlpha  |
                                    kSWContext2DNoPaintFillAlpha    |
                                    kSWContext2DNoPaintFillStyle    |
                                    kSWContext2DNoPaintFatal        ,

  kSWContext2DDisableStrokeFlags  = kSWContext2DNoPaintMetaMatrix   |
                                    kSWContext2DNoPaintUserClip     |
                                    kSWContext2DNoPaintUserMask     |
                                    kSWContext2DNoPaintUserMatrix   |
                                    kSWContext2DNoPaintGlobalCompOp |
                                    kSWContext2DNoPaintGlobalAlpha  |
                                    kSWContext2DNoPaintStrokeAlpha  |
                                    kSWContext2DNoPaintStrokeStyle  |
                                    kSWContext2DNoPaintStrokeParams |
                                    kSWContext2DNoPaintFatal        ,

  kSWContext2DDisableBlitFlags    = kSWContext2DNoPaintMetaMatrix   |
                                    kSWContext2DNoPaintUserClip     |
                                    kSWContext2DNoPaintUserMask     |
                                    kSWContext2DNoPaintUserMatrix   |
                                    kSWContext2DNoPaintGlobalCompOp |
                                    kSWContext2DNoPaintGlobalAlpha  |
                                    kSWContext2DNoPaintFatal

};

// ============================================================================
// [b2d::SWContext2DCoreParams]
// ============================================================================

// No need to create another struct, it's all in `Context2DParams::CoreParams`.
typedef Context2DParams::CoreParams SWContext2DCoreParams;

struct MatrixInfo {
  union {
    struct {
      uint8_t _metaMatrixType;
      uint8_t _finalMatrixType;
    };

    uint32_t _hints;
  };
};

// ============================================================================
// [b2d::SWContext2DExtParams]
// ============================================================================

//! Base data shared across `RasterContext2DImpl` and `SWContext2DState`.
struct SWContext2DExtParams {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(uint32_t alphaI) noexcept {
    _globalAlphaI = alphaI;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Global alpha as integer (0..256 or 0..65536).
  uint32_t _globalAlphaI;
};

// ============================================================================
// [b2d::SWContext2DDstInfo]
// ============================================================================

//! SWContext2D destination info. immutable after the image has been attached.
struct SWContext2DDstInfo {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _is16Bit;                      //!< Whether the destination uses 16 bits per component.
  uint8_t _dstFlags;                     //!< Destination flags.
  uint8_t _reserved[2];                  //!< Reserved.
  uint32_t _fullAlphaI;                  //!< Full alpha (256 or 65536).
  double _fullAlphaD;                    //!< Full alpha (256 or 65536) stored as `double`.
};

// ============================================================================
// [b2d::StyleData]
// ============================================================================

//! Style data - a copy of the user provided data.
struct StyleData {
  union {
    uint32_t _packed;
    struct {
      uint32_t _styleType          : 4;   //!< Style type as returned by `styleType()`.
      uint32_t _styleComponents    : 4;   //!< Style components, only [A|RGB].
      uint32_t _styleFormat        : 8;   //!< Style pixel format.
      uint32_t _styleMaxPfAltId    : 8;   //!< Maximum AltPixelFormat::Id (0 means never change).
      uint32_t _filter             : 4;   //!< Gradient/Pattern filter.
      uint32_t _adjustedMatrixType : 4;   //!< Adjusted matrix type.
    };
  };

  uint32_t _alpha;                       //!< Alpha value (0..256 or 0..65536).

  Pipe2D::FetchSolidData _solidData;     //!< Solid data.
  RasterFetchData* _fetchData;           //!< Fetch data.

  union {
    Wrap<Argb64> _color;                 //!< Color, not premultiplied.
    Wrap<Object> _object;                //!< Object.
    Wrap<Gradient> _gradient;            //!< Gradient.
    Wrap<Pattern> _pattern;              //!< Texture.
  };
  Matrix2D _adjusted;                    //!< Adjusted matrix.
};

// ============================================================================
// [b2d::FillCmd]
// ============================================================================

struct FillCmd {
  enum Status : uint32_t {
    kStatusNop   = 0,
    kStatusSolid = 1,
    kStatusValid = 2
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const Pipe2D::PipeSignature& initialSignature, uint32_t compOp, uint32_t alpha, uint32_t fillRule) noexcept {
    _baseSignature = initialSignature;
    _baseSignature.addCompOp(compOp);
    _alpha = alpha;
    _packed = 0;
    _fillRule = uint8_t(fillRule);
    _styleData = nullptr;
  }

  B2D_INLINE void setFillRule(uint32_t fillRule) noexcept {
    _fillRule = uint8_t(fillRule);
  }

  B2D_INLINE void setFetchDataFromStyle(StyleData* styleData) noexcept {
    _solidData = styleData->_solidData;
    _fetchData = styleData->_fetchData;
    _styleData = styleData;
  }

  B2D_INLINE void setFetchDataFromLocal(RasterFetchData* fetchData) noexcept {
    _fetchData = fetchData;
  }

  B2D_INLINE Pipe2D::PipeSignature& baseSignature() noexcept { return _baseSignature; }
  B2D_INLINE const Pipe2D::PipeSignature& baseSignature() const noexcept { return _baseSignature; }

  B2D_INLINE uint32_t fillRule() const noexcept { return _fillRule; }
  B2D_INLINE uint32_t alpha() const noexcept { return _alpha; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Pipe2D::PipeSignature _baseSignature;  //!< Signature parts related to `compOp` and source `style`.
  uint32_t _alpha;                       //!< Final alpha (integral).

  union {
    uint32_t _packed;
    struct {
      uint8_t _fillRule;                 //!< Fill rule.
      uint8_t _styleFormat;              //!< Style pixel format.
      uint8_t _reserved[2];
    };
  };

  Pipe2D::FetchSolidData _solidData;     //!< Solid data.
  RasterFetchData* _fetchData;           //!< Fetch data.
  StyleData* _styleData;                 //!< Style data to use when `_fetchData` is not available.
};

// ============================================================================
// [b2d::SWContext2DState]
// ============================================================================

//! SWContext2D state.
struct alignas(16) SWContext2DState {
  SWContext2DState* _prevState;          //!< Link to the previous state.

  uint64_t _stateId;                     //!< State ID (only valid if Cookie was used).
  uint32_t _prevContextFlags;            //!< Copy of previous `RasterContext2DImpl::_contextFlags`.
  MatrixInfo _matrixInfo;                //!< Matrix information.

  uint8_t _clipMode;                     //!< Clip mode.
  double _toleranceD;                    //!< Tolerance.

  SWContext2DCoreParams _coreParams;     //!< Core parameters.
  SWContext2DExtParams _extParams;       //!< Extended parameters.

  IntBox _finalClipBoxI;                 //!< Final clipBox (int).
  Box _finalClipBoxD;                    //!< Final clipBox (double).

  StyleData _style[
    Context2D::kStyleSlotCount];         //!< Fill and stroke styles.
  Wrap<StrokeParams> _strokeParams;      //!< Stroke parameters.

  Matrix2D _altMatrix;                   //!< Meta matrix or final matrix (depending on flags).
  Matrix2D _userMatrix;                  //!< User matrix.

  IntPoint _intTranslation;              //!< Integral translation, if possible.
  IntBox _intClientBox;                  //!< Integral clipBox translated by `_intTranslation`.
};

// ============================================================================
// [b2d::SWContext2DUtil]
// ============================================================================

//! `RasterContext2DImpl` utilities.
namespace SWContext2DUtil {
  static B2D_INLINE RasterFetchData* solidFetchSentinel() noexcept { return (RasterFetchData*)uintptr_t(0x1); }
  static B2D_INLINE bool isFetchDataSolid(RasterFetchData* fetcher) noexcept { return (uintptr_t)fetcher == (uintptr_t)solidFetchSentinel(); }
  static B2D_INLINE bool isFetchDataCreated(RasterFetchData* fetcher) noexcept { return (uintptr_t)fetcher > (uintptr_t)solidFetchSentinel(); }

  static B2D_INLINE void initStyleToDefault(StyleData& style, uint32_t alpha) noexcept {
    style._packed = 0;
    style._styleFormat = PixelFormat::kXRGB32;
    style._styleComponents = PixelFlags::kComponentRGB;
    style._styleMaxPfAltId = 0xFF;
    style._alpha = alpha;
    style._solidData.prgb32 = 0xFF000000u;
    style._fetchData = solidFetchSentinel();

    style._color->setValue(0xFFFF000000000000u);
    style._adjusted.reset();
  }

  static B2D_INLINE void destroyGradientFetchData(RasterFetchData* fetchData) noexcept { fetchData->gradientCacheRef->release(); }
  static B2D_INLINE void destroyPatternFetchData(RasterFetchData* fetchData) noexcept { fetchData->imageImplRef->release(); }

  //! Destroy the `RasterFetchData` content.
  static B2D_INLINE void destroyFetchData(RasterFetchData* fetchData) noexcept {
    uint32_t styleType = fetchData->styleType;

    if (styleType == Context2D::kStyleTypeGradient)
      destroyGradientFetchData(fetchData);
    else if (styleType == Context2D::kStyleTypePattern)
      destroyPatternFetchData(fetchData);
  }

  // TODO: Move to .cpp.
  static B2D_INLINE uint32_t analyzeGradient32(const GradientStop* stops, size_t count) noexcept {
    B2D_ASSERT(count > 0);
    uint32_t flags = PixelFlags::kComponentRGB | PixelFlags::_kSolid;

    if (B2D_ARCH_BITS >= 64) {
      uint64_t initial = stops[0].argb64().value() & 0xFF00FF00FF00FF00u;

      if (initial < 0xFF00000000000000u)
        flags |= PixelFlags::kComponentAlpha;

      for (size_t i = 1; i < count; i++) {
        uint64_t value = stops[i].argb64().value() & 0xFF00FF00FF00FF00u;

        if (value != initial)
          flags &= ~PixelFlags::_kSolid;

        if (value < 0xFF00000000000000u)
          flags |= PixelFlags::kComponentAlpha;
      }
    }
    else {
      uint32_t initialLo = stops[0].argb64().lo() & 0xFF00FF00u;
      uint32_t initialHi = stops[0].argb64().hi() & 0xFF00FF00u;

      if (initialHi < 0xFF000000u)
        flags |= PixelFlags::kComponentAlpha;

      for (size_t i = 1; i < count; i++) {
        uint32_t valueLo = stops[i].argb64().lo() & 0xFF00FF00u;
        uint32_t valueHi = stops[i].argb64().hi() & 0xFF00FF00u;

        if (valueLo != initialLo || valueHi != initialHi)
          flags &= ~PixelFlags::_kSolid;

        if (valueHi < 0xFF000000u)
          flags |= PixelFlags::kComponentAlpha;
      }
    }

    return flags;
  }
};

// ============================================================================
// [b2d::RasterContext2DImpl]
// ============================================================================

//! Pipe2D based rendering context implementation.
class RasterContext2DImpl : public Context2DImpl {
public:
  typedef Context2D::InitParams InitParams;
  typedef Pipe2D::PipeSignature PipeSignature;

  struct GlyphRunClosure {
    RasterContext2DImpl* ctxI;
    EdgeBuilderFromSource<EdgeSourcePath2D<>>* edgeBuilder;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  RasterContext2DImpl() noexcept;
  virtual ~RasterContext2DImpl() noexcept;

  static RasterContext2DImpl* _create(Image& image, const InitParams* initParams) noexcept;
  void destroy() noexcept override;

  // --------------------------------------------------------------------------
  // [Attach / Detach]
  // --------------------------------------------------------------------------

  static Error _attach(RasterContext2DImpl* d, Image& image, const InitParams* initParams) noexcept;
  static Error _detach(RasterContext2DImpl* d) noexcept;

  void _reset() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint64_t contextOriginId() const noexcept { return _contextOriginId; }
  B2D_INLINE uint64_t generateStateId() noexcept { return ++_stateIdCounter; }

  B2D_INLINE int fpShiftI() const noexcept { return _fpShiftI; }
  B2D_INLINE int fpScaleI() const noexcept { return _fpScaleI; }
  B2D_INLINE int fpMaskI() const noexcept { return _fpMaskI; }

  B2D_INLINE double fpScaleD() const noexcept { return _fpScaleD; }
  B2D_INLINE double fpMinSafeCoordD() const noexcept { return _fpMinSafeCoordD; }
  B2D_INLINE double fpMaxSafeCoordD() const noexcept { return _fpMaxSafeCoordD; }

  B2D_INLINE double toleranceD() const noexcept { return _toleranceD; }
  B2D_INLINE double toleranceFixedD() const noexcept { return _toleranceFixedD; }
  B2D_INLINE double toleranceFixedSqD() const noexcept { return _toleranceFixedSqD; }

  B2D_INLINE bool isIntTranslation() const noexcept { return (_contextFlags & kSWContext2DIsIntTranslation) != 0; }

  B2D_INLINE const PipeSignature& baseSignature() const noexcept { return _baseSignature; }

  B2D_INLINE uint32_t compOp() const noexcept { return _coreParams._hints._compOp; }
  B2D_INLINE uint32_t fillRule() const noexcept { return _coreParams._fillRule; }

  B2D_INLINE uint32_t simplifiedCompOpFromStyleComponents(uint32_t components) const noexcept {
    B2D_ASSERT((components & ~PixelFlags::kComponentMask) == 0);
    return _compOpTable._simplify[components];
  }

  B2D_INLINE const IntBox& metaClipBoxI() const noexcept { return _metaClipBoxI; }
  B2D_INLINE const IntBox& finalClipBoxI() const noexcept { return _finalClipBoxI; }
  B2D_INLINE const Box& finalClipBoxD() const noexcept { return _finalClipBoxD; }
  B2D_INLINE const Box& finalClipBoxFixedD() const noexcept { return _finalClipBoxFixedD; }

  B2D_INLINE const Matrix2D& metaMatrix() const noexcept { return _metaMatrix; }
  B2D_INLINE const Matrix2D& userMatrix() const noexcept { return _userMatrix; }
  B2D_INLINE const Matrix2D& finalMatrix() const noexcept { return _finalMatrix; }

  B2D_INLINE const Matrix2D& metaMatrixFixed() const noexcept { return _metaMatrixFixed; }
  B2D_INLINE const Matrix2D& finalMatrixFixed() const noexcept { return _finalMatrixFixed; }

  B2D_INLINE uint32_t metaMatrixType() const noexcept { return _matrixInfo._metaMatrixType; }
  B2D_INLINE uint32_t finalMatrixType() const noexcept { return _matrixInfo._finalMatrixType; }

  // --------------------------------------------------------------------------
  // [Flush]
  // --------------------------------------------------------------------------

  Error flush(uint32_t flags) noexcept override;

  // --------------------------------------------------------------------------
  // [Properties]
  // --------------------------------------------------------------------------

  B2D_INLINE void _updateCompOpTable(uint32_t compOp) noexcept {
    B2D_ASSERT(compOp < CompOp::_kInternalCount);
    _compOpTable._packed = *reinterpret_cast<const uint32_t*>(&Pipe2D::CompOpInfo::table[compOp]._simplify[uint32_t(_dstInfo._dstFlags) * 4]);
  }

  B2D_INLINE void _saveStroke() noexcept {
    SWContext2DState* state = _curState;
    B2D_ASSERT(state != NULL);

    state->_strokeParams.init(_stroker._params);
  }

  Error getProperty(uint32_t id, void* value) const noexcept override;
  Error setProperty(uint32_t id, const void* value) noexcept override;
  Error resetProperty(uint32_t id) noexcept override;

  // --------------------------------------------------------------------------
  // [Style]
  // --------------------------------------------------------------------------

  //! Create a new `RasterFetchData` from a `style`.
  //!
  //! Returns a `RasterFetchData` instance with reference count set to 1.
  RasterFetchData* _createFetchData(StyleData& style) noexcept;

  B2D_INLINE void _releaseFetchData(RasterFetchData* fetchData) noexcept {
    if (--fetchData->refCount == 0) {
      SWContext2DUtil::destroyFetchData(fetchData);
      _fetchPool.free(fetchData);
    }
  }

  B2D_INLINE void _discardStyle(StyleData& style) noexcept {
    RasterFetchData* fetchData = style._fetchData;
    if (SWContext2DUtil::isFetchDataCreated(fetchData))
      _releaseFetchData(fetchData);
    AnyInternal::releaseImpl(style._object->impl());
  }

  B2D_INLINE void _willChangeStyle(StyleData& style, uint32_t slot) noexcept {
    uint32_t contextFlags = _contextFlags;
    RasterFetchData* fetchData = style._fetchData;

    // If source is a fetch data it's better to just jump away as there is some
    // work needed to destroy it. It leaves possibility for CPU to follow this
    // path.
    if ((contextFlags & (kSWContext2DInfoBeginFetchData << slot)) != 0) {
      if ((contextFlags & (kSWContext2DStateBeginStyle << slot)) == 0) {
        if (SWContext2DUtil::isFetchDataCreated(fetchData))
          _releaseFetchData(fetchData);
        style._object->impl()->release();
        return;
      }
    }
    else {
      B2D_ASSERT((contextFlags & (kSWContext2DStateBeginStyle << slot)) != 0);
    }

    B2D_ASSERT(_curState != NULL);
    StyleData& stateStyle = _curState->_style[slot];

    // The content is moved to the `stateStyle`, so it doesn't matter if it
    // contains solid, gradient, or pattern as the state uses the same layout.
    stateStyle._packed = style._packed;
    // `stateStyle._alpha` has been already set by `RasterContext2DImpl::save()`.
    stateStyle._solidData.prgb64 = style._solidData.prgb64;
    stateStyle._fetchData = fetchData;

    stateStyle._color() = style._color();
    stateStyle._adjusted.reset();
  }

  Error getStyleType(uint32_t slot, uint32_t& type) const noexcept override;
  Error getStyleArgb32(uint32_t slot, uint32_t& argb32) const noexcept override;
  Error getStyleArgb64(uint32_t slot, uint64_t& argb64) const noexcept override;
  Error getStyleObject(uint32_t slot, Object* obj) const noexcept override;

  Error setStyleArgb32(uint32_t slot, uint32_t argb32) noexcept override;
  Error setStyleArgb64(uint32_t slot, uint64_t argb64) noexcept override;
  Error setStyleObject(uint32_t slot, const Object* obj) noexcept override;
  Error resetStyle(uint32_t slot) noexcept override;

  // --------------------------------------------------------------------------
  // [Matrix]
  // --------------------------------------------------------------------------

  Error getMatrix(Matrix2D& m) const noexcept override;
  Error setMatrix(const Matrix2D& m) noexcept override;
  Error matrixOp(uint32_t opId, const void* data) noexcept override;
  Error resetMatrix() noexcept override;
  Error userToMeta() noexcept override;

  // --------------------------------------------------------------------------
  // [Clipping]
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t numStatesToRestore(SWContext2DState* curState, uint64_t stateId) noexcept {
    uint32_t n = 1;
    do {
      uint64_t curStateId = curState->_stateId;
      if (curStateId <= stateId)
        return curStateId == stateId ? n : uint32_t(0);
      n++;
      curState = curState->_prevState;
    } while (curState);
    return 0;
  }

  // TODO: NOT USED
  void _saveClip() noexcept;

  void _discardStates(SWContext2DState* top) noexcept;

  Error save(Cookie* cookie) noexcept override;
  Error restore(const Cookie* cookie) noexcept override;

  // --------------------------------------------------------------------------
  // [State Change]
  // --------------------------------------------------------------------------

  // "CoreState" consists of states that are always saved and restored to make
  // the restoration simpler. All states saved/restored by CoreState should be
  // relatively cheap.

  //! Called by `save()` to store all CoreStates.
  B2D_INLINE void _onCoreStateSave(SWContext2DState* state) noexcept {
    state->_prevContextFlags = _contextFlags;

    state->_matrixInfo = _matrixInfo;
    state->_intTranslation = _intTranslation;
    state->_intClientBox = _intClientBox;

    state->_coreParams = _coreParams;
    state->_extParams = _extParams;

    state->_style[0]._alpha = _style[0]._alpha;
    state->_style[1]._alpha = _style[1]._alpha;

    state->_clipMode = _worker._clipMode;
  }

  //! Called by `restore()` to restore all CoreStates.
  B2D_INLINE void _onCoreStateRestore(SWContext2DState* state) noexcept {
    _contextFlags = state->_prevContextFlags;

    _matrixInfo = state->_matrixInfo;
    _intTranslation = state->_intTranslation;
    _intClientBox = state->_intClientBox;

    _coreParams = state->_coreParams;
    _extParams = state->_extParams;

    uint32_t compOp = state->_coreParams._hints._compOp;
    _updateCompOpTable(compOp);

    _style[0]._alpha = state->_style[0]._alpha;
    _style[1]._alpha = state->_style[1]._alpha;
  }

  B2D_INLINE void _onBeforeConfigChange() noexcept {
    if ((_contextFlags & kSWContext2DStateConfig) != 0) {
      SWContext2DState* state = _curState;
      B2D_ASSERT(state != NULL);

      state->_toleranceD = toleranceD();
    }
  }

  B2D_INLINE void _onUpdateTolerance() noexcept {
    _toleranceFixedD = _toleranceD * _fpScaleD;
    _toleranceFixedSqD = Math::pow2(_toleranceFixedD);
  }

  B2D_INLINE void _onUpdateFinalMatrix() noexcept {
    Matrix2D::multiply(_finalMatrix, metaMatrix(), userMatrix());
  }

  B2D_INLINE void _onUpdateMetaMatrixFixed() noexcept {
    _metaMatrixFixed = _metaMatrix;
    _metaMatrixFixed.scaleAppend(fpScaleD());
  }

  B2D_INLINE void _onUpdateFinalMatrixFixed() noexcept {
    _finalMatrixFixed = _finalMatrix;
    _finalMatrixFixed.scaleAppend(fpScaleD());
  }

  //! Called before `userMatrix` is modified.
  //!
  //! This function is responsible for saving the current userMatrix in
  //! case that the `kSWContext2DStateUserMatrix` flag is set, which means
  //! that userMatrix must be saved before any modification.
  B2D_INLINE void _onBeforeUserMatrixChange() noexcept {
    if ((_contextFlags & kSWContext2DStateUserMatrix) != 0) {
      // MetaMatrix change would also save UserMatrix, no way this could be unset.
      B2D_ASSERT((_contextFlags & kSWContext2DStateMetaMatrix) != 0);

      SWContext2DState* state = _curState;
      B2D_ASSERT(state != NULL);

      state->_altMatrix = finalMatrix();
      state->_userMatrix = userMatrix();
    }
  }

  //! Called after `userMatrix` has been modified.
  //!
  //! Responsible for updating `finalMatrix` and other matrix information.
  B2D_INLINE void _onAfterUserMatrixChange() noexcept {
    _contextFlags &= ~(kSWContext2DNoPaintUserMatrix |
                       kSWContext2DIsIntTranslation  |
                       kSWContext2DStateUserMatrix   );

    _onUpdateFinalMatrix();
    _onUpdateFinalMatrixFixed();

    Matrix2D& fm = _finalMatrixFixed;
    uint32_t finalType = _finalMatrix.type();
    _matrixInfo._finalMatrixType = uint8_t(finalType);

    if (finalType <= Matrix2D::kTypeTranslation) {
      // No scaling - input coordinates have pixel granularity. Check if the
      // translation has pixel granularity as well and setup the `_intTranslation`
      // data for that case.
      if (fm.m20() >= fpMinSafeCoordD() && fm.m20() <= fpMaxSafeCoordD() &&
          fm.m21() >= fpMinSafeCoordD() && fm.m21() <= fpMaxSafeCoordD()) {
        // We need 64-bit ints here as we are already scaled. Also we need a `floor`
        // function as we have to handle negative translations which cannot be
        // truncated (the default conversion).
        int64_t tx64 = Math::i64floor(fm.m20());
        int64_t ty64 = Math::i64floor(fm.m21());

        // Pixel to pixel translation is only possible when both fixed points
        // `tx64` and `ty64` have zeros in their fraction parts.
        if (((tx64 | ty64) & fpMaskI()) == 0) {
          int tx = int(tx64 >> fpShiftI());
          int ty = int(ty64 >> fpShiftI());

          _intTranslation.reset(tx, ty);
          _intClientBox.reset(_finalClipBoxI.x0() - tx,
                              _finalClipBoxI.y0() - ty,
                              _finalClipBoxI.x1() - tx,
                              _finalClipBoxI.y1() - ty);
          _contextFlags |= kSWContext2DIsIntTranslation;
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Group]
  // --------------------------------------------------------------------------

  Error beginGroup() noexcept override;
  Error paintGroup() noexcept override;

  // --------------------------------------------------------------------------
  // [Fill]
  // --------------------------------------------------------------------------

  Error fillArg(uint32_t id, const void* data) noexcept override;
  Error fillAll() noexcept override;
  Error fillRectD(const Rect& rect) noexcept override;
  Error fillRectI(const IntRect& rect) noexcept override;
  Error fillText(const Point& dst, const Font& font, const void* text, size_t size, uint32_t encoding) noexcept override;
  Error fillGlyphRun(const Point& dst, const Font& font, const GlyphRun& glyphRun) noexcept override;

  // --------------------------------------------------------------------------
  // [Stroke]
  // --------------------------------------------------------------------------

  Error strokeArg(uint32_t id, const void* data) noexcept override;
  Error strokeRectD(const Rect& rect) noexcept override;
  Error strokeRectI(const IntRect& rect) noexcept override;

  // --------------------------------------------------------------------------
  // [Blit]
  // --------------------------------------------------------------------------

  Error blitImagePD(const Point& dst, const Image& src, const IntRect* srcArea) noexcept override;
  Error blitImagePI(const IntPoint& dst, const Image& src, const IntRect* srcArea) noexcept override;
  Error blitImageRD(const Rect& dst, const Image& src, const IntRect* srcArea) noexcept override;
  Error blitImageRI(const IntRect& dst, const Image& src, const IntRect* srcArea) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  SWContext2DDstInfo _dstInfo;           //!< Destination info.
  Wrap<Image> _dstImage;                 //!< Destination image.

  Pipe2D::PipeRuntime* _pipeRuntime;     //!< Pipe2D runtime.

  Zone _cmdZone;                         //!< Zone allocator used to allocate commands for deferred and asynchronous rendering.
  Zone _baseZone;                        //!< Zone allocator used to allocate many data structures required by `RasterContext2DImpl`.

  // TODO: B2D_SIZEOF_ALIGNED should not be used here, it should be naturally aligned to 16 bytes.

  //! Object pool used to allocate `RasterFetchData`.
  ZonePool<RasterFetchData, B2D_SIZEOF_ALIGNED(RasterFetchData, 16)> _fetchPool;
  //! Object pool used to allocate `SWContext2DState`.
  ZonePool<SWContext2DState, B2D_SIZEOF_ALIGNED(SWContext2DState, 16)> _statePool;

  uint64_t _contextOriginId;             //!< Context origin ID that it would set to a possible `Cookie` as `data0`.
  uint64_t _stateIdCounter;              //!< Used to genearate unique IDs in domain of this `Context2D`.

  PipeSignature _baseSignature;          //!< Context2D signature part (contains target PixelFormat).

  int _fpShiftI;                         //!< Fixed point shift (able to multiply / divide by fpScale).
  int _fpScaleI;                         //!< Fixed point scale as int (either 256 or 65536).
  int _fpMaskI;                          //!< Fixed point mask calculated as `fpScaleI - 1`.

  double _fpScaleD;                      //!< Fixed point scale as double (either 256.0 or 65536.0).
  double _fpMinSafeCoordD;               //!< Minimum safe coordinate for integral transformation (scaled by 256.0 or 65536.0).
  double _fpMaxSafeCoordD;               //!< Maximum safe coordinate for integral transformation (scaled by 256.0 or 65536.0).

  double _toleranceD;                    //!< Curve flattening tolerance.
  double _toleranceFixedD;               //!< Curve flattening tolerance scaled by `_fpScaleD`.
  double _toleranceFixedSqD;             //!< `_toleranceFixedD^2`.

  SWContext2DCoreParams _coreParams;     //!< Core parameters, can be set/get through the public API.
  SWContext2DExtParams _extParams;       //!< Extended parameters, hidden from the public API.

  //! Simplifies the current compositing operator based on the style flags.
  union {
    uint8_t _simplify[4];
    uint32_t _packed;
  } _compOpTable;

  StyleData _style[
    Context2D::kStyleSlotCount];         //!< Fill and stroke styles.

  SWContext2DState* _curState;           //!< Current state.

  IntBox _metaClipBoxI;                  //!< Meta clip-box (int).
  IntBox _finalClipBoxI;                 //!< Final clip box (int).
  Box _finalClipBoxD;                    //!< Final clip-box (double).
  Box _finalClipBoxFixedD;               //!< Final clip-box scaled by `fpScale` (double).

  Matrix2D _metaMatrix;                  //!< Meta matrix.
  Matrix2D _userMatrix;                  //!< User matrix (where user transforms happen).
  Matrix2D _finalMatrix;                 //!< Result of `(metaMatrix * userMatrix)`.
  Matrix2D _metaMatrixFixed;             //!< Meta matrix scaled by `fpScale`.
  Matrix2D _finalMatrixFixed;            //!< Result of `(metaMatrix * userMatrix) * fpScale`.
  MatrixInfo _matrixInfo;                //!< Matrix information and hints.

  IntPoint _intTranslation;              //!< Integral offset to add to input coordinates in case integral transform is ok.
  IntBox _intClientBox;                  //!< Integral clipBox that has been pre-translated by `_intTranslation`.

  Stroker _stroker;                      //!< Stroke context that also holds stroke params.
  RasterWorker _worker;                  //!< Single-threaded worker that also contains some states.
  TextBuffer _textBuffer;                //!< Temporary text buffer used to shape a text to fill.
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_RASTERCONTEXT2D_P_H
