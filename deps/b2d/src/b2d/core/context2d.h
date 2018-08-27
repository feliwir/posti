// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_CONTEXT2D_H
#define _B2D_CORE_CONTEXT2D_H

// [Dependencies]
#include "../core/any.h"
#include "../core/argb.h"
#include "../core/array.h"
#include "../core/carray.h"
#include "../core/compop.h"
#include "../core/contextdefs.h"
#include "../core/cookie.h"
#include "../core/fill.h"
#include "../core/geomtypes.h"
#include "../core/gradient.h"
#include "../core/image.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/pattern.h"
#include "../core/region.h"
#include "../core/stroke.h"
#include "../core/unicode.h"
#include "../text/font.h"
#include "../text/fontface.h"
#include "../text/glyphrun.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class Context2D;
class Context2DImpl;

struct Context2DHints;
struct Context2DParams;

// ============================================================================
// [b2d::Context2DImpl]
// ============================================================================

//! \internal
//!
//! Context2D implementation (abstract).
class B2D_VIRTAPI Context2DImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(Context2DImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API Context2DImpl() noexcept;
  B2D_API virtual ~Context2DImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Destroy]
  // --------------------------------------------------------------------------

  virtual void destroy() noexcept = 0;

  // --------------------------------------------------------------------------
  // [Flush]
  // --------------------------------------------------------------------------

  virtual Error flush(uint32_t flags) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Properties]
  // --------------------------------------------------------------------------

  virtual Error getProperty(uint32_t id, void* value) const noexcept = 0;
  virtual Error setProperty(uint32_t id, const void* value) noexcept = 0;
  virtual Error resetProperty(uint32_t id) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Style]
  // --------------------------------------------------------------------------

  virtual Error getStyleType(uint32_t slot, uint32_t& type) const noexcept = 0;
  virtual Error getStyleArgb32(uint32_t slot, uint32_t& argb32) const noexcept = 0;
  virtual Error getStyleArgb64(uint32_t slot, uint64_t& argb64) const noexcept = 0;
  virtual Error getStyleObject(uint32_t slot, Object* obj) const noexcept = 0;

  virtual Error setStyleArgb32(uint32_t slot, uint32_t argb32) noexcept = 0;
  virtual Error setStyleArgb64(uint32_t slot, uint64_t argb64) noexcept = 0;
  virtual Error setStyleObject(uint32_t slot, const Object* obj) noexcept = 0;
  virtual Error resetStyle(uint32_t slot) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Matrix]
  // --------------------------------------------------------------------------

  virtual Error getMatrix(Matrix2D& m) const noexcept = 0;
  virtual Error setMatrix(const Matrix2D& m) noexcept = 0;
  virtual Error matrixOp(uint32_t opId, const void* data) noexcept = 0;
  virtual Error resetMatrix() noexcept = 0;
  virtual Error userToMeta() noexcept = 0;

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  virtual Error save(Cookie* cookie) noexcept = 0;
  virtual Error restore(const Cookie* cookie) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Group]
  // --------------------------------------------------------------------------

  virtual Error beginGroup() noexcept = 0;
  virtual Error paintGroup() noexcept = 0;

  // --------------------------------------------------------------------------
  // [Fill]
  // --------------------------------------------------------------------------

  virtual Error fillArg(uint32_t id, const void* data) noexcept = 0;
  virtual Error fillAll() noexcept = 0;
  virtual Error fillRectD(const Rect& rect) noexcept = 0;
  virtual Error fillRectI(const IntRect& rect) noexcept = 0;

  virtual Error fillText(const Point& dst, const Font& font, const void* text, size_t size, uint32_t encoding) noexcept = 0;
  virtual Error fillGlyphRun(const Point& dst, const Font& font, const GlyphRun& glyphRun) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Stroke]
  // --------------------------------------------------------------------------

  virtual Error strokeArg(uint32_t id, const void* data) noexcept = 0;
  virtual Error strokeRectD(const Rect& rect) noexcept = 0;
  virtual Error strokeRectI(const IntRect& rect) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Blit]
  // --------------------------------------------------------------------------

  virtual Error blitImagePD(const Point& dst, const Image& src, const IntRect* srcArea) noexcept = 0;
  virtual Error blitImagePI(const IntPoint& dst, const Image& src, const IntRect* srcArea) noexcept = 0;
  virtual Error blitImageRD(const Rect& dst, const Image& src, const IntRect* srcArea) noexcept = 0;
  virtual Error blitImageRI(const IntRect& dst, const Image& src, const IntRect* srcArea) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _implType;                    //!< Context implementation type.
  uint32_t _contextFlags;                //!< Context flags.

  IntSize _targetSizeI;                  //!< Target buffer size (int).
  Size _targetSizeD;                     //!< Target buffer size (double).
};

// ============================================================================
// [b2d::Context2D]
// ============================================================================

//! 2D context.
class Context2D : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdContext2D };

  // --------------------------------------------------------------------------
  // [ImplType]
  // --------------------------------------------------------------------------

  //! Context2D implementation type.
  enum ImplType : uint32_t {
    kImplTypeNone        = 0,            //!< Null implementation (does nothing).
    kImplTypePipe2D      = 1,            //!< Pipe2D implementation (CPU & JIT).

    kImplTypeCount       = 2             //!< Count of implementation types.
  };

  // --------------------------------------------------------------------------
  // [StyleType]
  // --------------------------------------------------------------------------

  //! Style type.
  enum StyleType : uint32_t {
    kStyleTypeNone       = 0,            //!< Style is none.
    kStyleTypeSolid      = 1,            //!< Style is solid.
    kStyleTypeGradient   = 2,            //!< Style is gradient.
    kStyleTypePattern    = 3,            //!< Style is pattern.

    kStyleTypeCount      = 4             //!< Count of style types.
  };

  // --------------------------------------------------------------------------
  // [StyleSlot]
  // --------------------------------------------------------------------------

  //! Style slot, used to distinguish between fill and stroke styles.
  enum StyleSlot : uint32_t {
    kStyleSlotFill       = 0,            //!< Fill slot.
    kStyleSlotStroke     = 1,            //!< Stroke slot.

    kStyleSlotCount      = 2             //!< Count of slots.
  };

  // --------------------------------------------------------------------------
  // [ClipMode]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Clip mode.
  enum ClipMode : uint32_t {
    kClipModeRect = 0,                   //!< The clip is a rectangle (IntBox) or none.
    kClipModeRegion = 1,                 //!< The clip is a region.
    kClipModeMask = 2,                   //!< The clip is a mask.
    kClipModeCount = 3                   //!< Count of clip modes.
  };

  // --------------------------------------------------------------------------
  // [ClipOp]
  // --------------------------------------------------------------------------

  //! Clip operation.
  enum ClipOp : uint32_t {
    kClipOpReplace = 0,                  //!< Replace the current clip area.
    kClipOpIntersect = 1,                //!< Intersect with the current clip area.

    kClipOpCount = 2                     //!< Count of clip operators.
  };

  // --------------------------------------------------------------------------
  // [RenderQuality]
  // --------------------------------------------------------------------------

  //! Rendering quality.
  enum RenderQuality : uint32_t {
    //! Disabled antialiasing.
    kRenderQualityAliased = 0,
    //! High-quality antialiasing.
    kRenderQualitySmooth = 1,

    //! Count of render quality settings (for error checking).
    kRenderQualityCount = 2,
    //! Default rendering quality (links to \ref kRenderQualityNormal).
    kRenderQualityDefault = kRenderQualitySmooth
  };

  // --------------------------------------------------------------------------
  // [InitFlags]
  // --------------------------------------------------------------------------

  //! Flags that can be passed to `Context2D()` or `Context2D::begin()`.
  enum InitFlags : uint32_t {
    kInitFlagIsolated     = 0x80000000u  //!< Run in isolated mode (testing-only).
  };

  // --------------------------------------------------------------------------
  // [FlushFlags]
  // --------------------------------------------------------------------------

  //! Flags that can be passed to `flush()`.
  enum FlushFlags : uint32_t {
    //! Flush all commands and wait for completion.
    //!
    //! This will sync the content on the raster with all commands send to the 2d
    //! context. Please note that even if you flush the context and wait for
    //! completion you shouldn't access its data. It might work at the moment,
    //! but break in the future.
    kFlushFlagSync = 0x80000000u
  };

  // --------------------------------------------------------------------------
  // [PropertyId]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Changing the state of the `Context2D` is internally implemented as passing
  //! `propertyId` and `params`. This way allows to add features to Context2D
  //! without breaking ABI.
  enum PropertyId : uint32_t {
    kPropertyIdContextParams = 0,        //!< Context parameters (hints, opacity, fill and stroke).

    kPropertyIdContextHints,             //!< Context hints.
    kPropertyIdRenderQuality,            //!< Render quality.
    kPropertyIdGradientFilter,           //!< Gradient filter.
    kPropertyIdPatternFilter,            //!< Pattern filter.

    kPropertyIdTolerance,                //!< Curve flattening tolerance.

    kPropertyIdCompOp,                   //!< Compositing operator.
    kPropertyIdAlpha,                    //!< Global alpha value.
    kPropertyIdFillAlpha,                //!< Fill alpha value.
    kPropertyIdStrokeAlpha,              //!< Stroke alpha value.

    kPropertyIdFillRule,                 //!< Fill rule.

    kPropertyIdStrokeParams,             //!< Stroke parameters.
    kPropertyIdStrokeWidth,              //!< Stroke width.
    kPropertyIdStrokeMiterLimit,         //!< Stroke miter limit.
    kPropertyIdStrokeDashOffset,         //!< Stroke dash offset.
    kPropertyIdStrokeDashArray,          //!< Stroke dash array.
    kPropertyIdStrokeJoin,               //!< Stroke join.
    kPropertyIdStrokeStartCap,           //!< Stroke start cap.
    kPropertyIdStrokeEndCap,             //!< Stroke end cap.
    kPropertyIdStrokeLineCaps,           //!< Start cap and end cap.
    kPropertyIdStrokeTransformOrder,     //!< Stroke transform order.

    kPropertyIdCount                     //!< Count of Context2D properties.
  };

  // --------------------------------------------------------------------------
  // [InitParams]
  // --------------------------------------------------------------------------

  class InitParams {
  public:
    inline InitParams(uint32_t flags = 0, uint32_t optLevel = 0) noexcept
      : _flags(flags),
        _optLevel(optLevel),
        _maxPixels(0) {}

    inline bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }
    inline uint32_t flags() const noexcept { return _flags; }
    inline void setFlags(uint32_t flags) noexcept { _flags = flags; }

    inline uint32_t optLevel() const noexcept { return _optLevel; }
    inline void setOptLevel(uint32_t optLevel) noexcept { _optLevel = optLevel; }
    inline void resetOptLevel() noexcept { _optLevel = 0; }

    inline uint32_t maxPixels() const noexcept { return _maxPixels; }
    inline void setMaxPixelStep(uint32_t value) noexcept { _maxPixels = value; }
    inline void resetMaxPixelStep() noexcept { _maxPixels = 0; }

    uint32_t _flags;                     //!< Initialization flags.
    uint32_t _optLevel;                  //!< Maximum optimization level (Pipe2D).
    uint32_t _maxPixels;                 //!< Maximum pixels at a time (Pipe2D).
  };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Context2D() noexcept
    : Object(none().impl()) {}

  B2D_INLINE Context2D(const Context2D& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE Context2D(Context2D&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Context2D(Image& target) noexcept
    : Object(none().impl()) { begin(target); }

  B2D_INLINE Context2D(Image& target, const InitParams& initParams) noexcept
    : Object(none().impl()) { begin(target, initParams); }

  B2D_INLINE Context2D(Image& target, const InitParams* initParams) noexcept
    : Object(none().impl()) { begin(target, initParams); }

  static B2D_INLINE const Context2D& none() noexcept { return Any::noneOf<Context2D>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = Context2DImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Assign]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const Context2D& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }

  // --------------------------------------------------------------------------
  // [Begin / End]
  // --------------------------------------------------------------------------

  //! Begin painting to the `image` - Context will gain exclusive access to the image data.
  B2D_INLINE Error begin(Image& image) noexcept { return _begin(image, nullptr); }
  //! \overload
  B2D_INLINE Error begin(Image& image, const InitParams& initParams) noexcept { return _begin(image, &initParams); }
  //! \overload
  B2D_INLINE Error begin(Image& image, const InitParams* initParams) noexcept { return _begin(image, initParams); }

  B2D_API Error _begin(Image& image, const InitParams* initParams) noexcept;

  //! Wait for completion of all render commands and end the rendering.
  //!
  //! NOTE: It's safe to assume the `Image` unlocked after `end()` returns.
  B2D_API Error end() noexcept;

  // --------------------------------------------------------------------------
  // [Flush]
  // --------------------------------------------------------------------------

  //! Flush painter, see \ref FlushFlags.
  B2D_INLINE Error flush(uint32_t flags) noexcept { return impl()->flush(flags); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the type of the implementation of Context2D, see `ImplType`.
  B2D_INLINE uint32_t implType() const noexcept { return impl()->_implType; }

  //! Get the size of the target buffer.
  B2D_INLINE Error getTargetSize(IntSize& val) const noexcept {
    val = impl()->_targetSizeI;
    return kErrorOk;
  }

  //! \overload
  B2D_INLINE Error getTargetSize(Size& val) const noexcept {
    val = impl()->_targetSizeD;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Context Params]
  // --------------------------------------------------------------------------

  //! Get the context parameters.
  B2D_INLINE Error getContextParams(Context2DParams& val) const noexcept { return impl()->getProperty(kPropertyIdContextParams, &val); }
  //! Set the context parameters.
  B2D_INLINE Error setContextParams(const Context2DParams& val) noexcept { return impl()->setProperty(kPropertyIdContextParams, &val); }
  //! Reset the context parameters.
  B2D_INLINE Error resetContextParams() noexcept { return impl()->resetProperty(kPropertyIdContextParams); }

  // --------------------------------------------------------------------------
  // [Context Hints]
  // --------------------------------------------------------------------------

  //! Get the context hints.
  B2D_INLINE Error getContextHints(Context2DHints& val) const noexcept { return impl()->getProperty(kPropertyIdContextHints, &val); }
  //! Set the context hints.
  B2D_INLINE Error setContextHints(const Context2DHints& val) noexcept { return impl()->setProperty(kPropertyIdContextHints, &val); }
  //! Reset the context hints.
  B2D_INLINE Error resetContextHints() noexcept { return impl()->resetProperty(kPropertyIdContextHints); }

  //! Get the render-quality hint, see \ref RenderQuality.
  B2D_INLINE Error getRenderQuality(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdRenderQuality, &val); }
  //! Set the render-quality hint, see \ref RenderQuality.
  B2D_INLINE Error setRenderQuality(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdRenderQuality, &val); }
  //! Reset the render-quality hint.
  B2D_INLINE Error resetRenderQuality() noexcept { return impl()->resetProperty(kPropertyIdRenderQuality); }

  //! Get gradient filter, see `Gradient::Filter`.
  B2D_INLINE Error getGradientFilter(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdGradientFilter, &val); }
  //! Set gradient filter, see `Gradient::Filter`.
  B2D_INLINE Error setGradientFilter(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdGradientFilter, &val); }
  //! Reset gradient filter to `Gradient::kFilterDefault`.
  B2D_INLINE Error resetGradientFilter() noexcept { return impl()->resetProperty(kPropertyIdGradientFilter); }

  //! Get pattern filter, see `Pattern::Filter`.
  B2D_INLINE Error getPatternFilter(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdPatternFilter, &val); }
  //! Set pattern filter, see `Pattern::Filter`.
  B2D_INLINE Error setPatternFilter(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdPatternFilter, &val); }
  //! Reset pattern filter to `Pattern::kFilterDefault`.
  B2D_INLINE Error resetPatternFilter() noexcept { return impl()->resetProperty(kPropertyIdPatternFilter); }

  //! Get flattening tolerance.
  B2D_INLINE Error getTolerance(double& val) const noexcept { return impl()->getProperty(kPropertyIdTolerance, &val); }
  //! Set flattening tolerance.
  B2D_INLINE Error setTolerance(double val) noexcept { return impl()->setProperty(kPropertyIdTolerance, &val); }
  //! Reset flattening tolerance.
  B2D_INLINE Error resetTolerance() noexcept { return impl()->resetProperty(kPropertyIdTolerance); }

  //! Get global compositing operator.
  B2D_INLINE Error getCompOp(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdCompOp, &val); }
  //! Set global compositing operator.
  B2D_INLINE Error setCompOp(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdCompOp, &val); }
  //! Reset global compositing operator.
  B2D_INLINE Error resetCompOp() noexcept { return impl()->resetProperty(kPropertyIdCompOp); }

  // --------------------------------------------------------------------------
  // [Global Alpha]
  // --------------------------------------------------------------------------

  //! Get global alpha value.
  B2D_INLINE Error getAlpha(double& value) const noexcept { return impl()->getProperty(kPropertyIdAlpha, &value); }
  //! Set global alpha value.
  B2D_INLINE Error setAlpha(double value) noexcept { return impl()->setProperty(kPropertyIdAlpha, &value); }
  //! Reset global alpha value.
  B2D_INLINE Error resetAlpha() noexcept { return impl()->resetProperty(kPropertyIdAlpha); }

  // --------------------------------------------------------------------------
  // [Style Slots]
  // --------------------------------------------------------------------------

  B2D_INLINE Error getStyle(uint32_t slot, Argb32& argb32) const noexcept { return impl()->getStyleArgb32(slot, argb32._value); }
  B2D_INLINE Error getStyle(uint32_t slot, Argb64& argb64) const noexcept { return impl()->getStyleArgb64(slot, argb64._value); }
  B2D_INLINE Error getStyle(uint32_t slot, Gradient& gradient) const noexcept { return impl()->getStyleObject(slot, &gradient); }
  B2D_INLINE Error getStyle(uint32_t slot, Image& image) const noexcept { return impl()->getStyleObject(slot, &image); }
  B2D_INLINE Error getStyle(uint32_t slot, Pattern& pattern) const noexcept { return impl()->getStyleObject(slot, &pattern); }

  B2D_INLINE Error setStyle(uint32_t slot, const Argb32& argb32) noexcept { return impl()->setStyleArgb32(slot, argb32._value); }
  B2D_INLINE Error setStyle(uint32_t slot, const Argb64& argb64) noexcept { return impl()->setStyleArgb64(slot, argb64._value); }
  B2D_INLINE Error setStyle(uint32_t slot, const Gradient& gradient) noexcept { return impl()->setStyleObject(slot, &gradient); }
  B2D_INLINE Error setStyle(uint32_t slot, const Image& image) noexcept { return impl()->setStyleObject(slot, &image); }
  B2D_INLINE Error setStyle(uint32_t slot, const Pattern& pattern) noexcept { return impl()->setStyleObject(slot, &pattern); }

  B2D_INLINE Error resetStyle(uint32_t slot) noexcept { return impl()->resetStyle(slot); }

  // --------------------------------------------------------------------------
  // [Fill Style]
  // --------------------------------------------------------------------------

  B2D_INLINE Error getFillStyle(Argb32& argb32) const noexcept { return getStyle(kStyleSlotFill, argb32); }
  B2D_INLINE Error getFillStyle(Argb64& argb64) const noexcept { return getStyle(kStyleSlotFill, argb64); }
  B2D_INLINE Error getFillStyle(Gradient& gradient) const noexcept { return getStyle(kStyleSlotFill, gradient); }
  B2D_INLINE Error getFillStyle(Image& image) const noexcept { return getStyle(kStyleSlotFill, image); }
  B2D_INLINE Error getFillStyle(Pattern& pattern) const noexcept { return getStyle(kStyleSlotFill, pattern); }

  B2D_INLINE Error setFillStyle(const Argb32& argb32) noexcept { return setStyle(kStyleSlotFill, argb32); }
  B2D_INLINE Error setFillStyle(const Argb64& argb64) noexcept { return setStyle(kStyleSlotFill, argb64); }
  B2D_INLINE Error setFillStyle(const Gradient& gradient) noexcept { return setStyle(kStyleSlotFill, gradient); }
  B2D_INLINE Error setFillStyle(const Image& image) noexcept { return setStyle(kStyleSlotFill, image); }
  B2D_INLINE Error setFillStyle(const Pattern& pattern) noexcept { return setStyle(kStyleSlotFill, pattern); }

  B2D_INLINE Error resetFillStyle() noexcept { return resetStyle(kStyleSlotFill); }

  // --------------------------------------------------------------------------
  // [Fill Alpha]
  // --------------------------------------------------------------------------

  //! Get the fill alpha value.
  B2D_INLINE Error getFillAlpha(double& value) const noexcept { return impl()->getProperty(kPropertyIdFillAlpha, &value); }
  //! Set the fill alpha value.
  B2D_INLINE Error setFillAlpha(double value) noexcept { return impl()->setProperty(kPropertyIdFillAlpha, &value); }
  //! Reset the fill alpha value.
  B2D_INLINE Error resetFillAlpha() noexcept { return impl()->resetProperty(kPropertyIdFillAlpha); }

  // --------------------------------------------------------------------------
  // [Fill Params]
  // --------------------------------------------------------------------------

  //! Get the fill rule, see `FillRule::Type`.
  B2D_INLINE Error getFillRule(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdFillRule, &val); }
  //! Set the fill rule, see `FillRule::Type`.
  B2D_INLINE Error setFillRule(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdFillRule, &val); }
  //! Reset the fill rule.
  B2D_INLINE Error resetFillRule() noexcept { return impl()->resetProperty(kPropertyIdFillRule); }

  // --------------------------------------------------------------------------
  // [Stroke Style]
  // --------------------------------------------------------------------------

  B2D_INLINE Error getStrokeStyle(Argb32& argb32) const noexcept { return getStyle(kStyleSlotStroke, argb32); }
  B2D_INLINE Error getStrokeStyle(Argb64& argb64) const noexcept { return getStyle(kStyleSlotStroke, argb64); }
  B2D_INLINE Error getStrokeStyle(Gradient& gradient) const noexcept { return getStyle(kStyleSlotStroke, gradient); }
  B2D_INLINE Error getStrokeStyle(Image& image) const noexcept { return getStyle(kStyleSlotStroke, image); }
  B2D_INLINE Error getStrokeStyle(Pattern& pattern) const noexcept { return getStyle(kStyleSlotStroke, pattern); }

  B2D_INLINE Error setStrokeStyle(const Argb32& argb32) noexcept { return setStyle(kStyleSlotStroke, argb32); }
  B2D_INLINE Error setStrokeStyle(const Argb64& argb64) noexcept { return setStyle(kStyleSlotStroke, argb64); }
  B2D_INLINE Error setStrokeStyle(const Gradient& gradient) noexcept { return setStyle(kStyleSlotStroke, gradient); }
  B2D_INLINE Error setStrokeStyle(const Image& image) noexcept { return setStyle(kStyleSlotStroke, image); }
  B2D_INLINE Error setStrokeStyle(const Pattern& pattern) noexcept { return setStyle(kStyleSlotStroke, pattern); }

  B2D_INLINE Error resetStrokeStyle() noexcept { return resetStyle(kStyleSlotStroke); }

  // --------------------------------------------------------------------------
  // [Stroke Alpha]
  // --------------------------------------------------------------------------

  //! Get the stroke alpha value.
  B2D_INLINE Error getStrokeAlpha(double& value) const noexcept { return impl()->getProperty(kPropertyIdStrokeAlpha, &value); }
  //! Set the stroke alpha value.
  B2D_INLINE Error setStrokeAlpha(double value) noexcept { return impl()->setProperty(kPropertyIdStrokeAlpha, &value); }
  //! Reset the stroke alpha value.
  B2D_INLINE Error resetStrokeAlpha() noexcept { return impl()->resetProperty(kPropertyIdStrokeAlpha); }

  // --------------------------------------------------------------------------
  // [Stroke Params]
  // --------------------------------------------------------------------------

  //! Get the stroke parameters.
  B2D_INLINE Error getStrokeParams(StrokeParams& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeParams, &val); }
  //! Set the stroke parameters.
  B2D_INLINE Error setStrokeParams(const StrokeParams& val) noexcept { return impl()->setProperty(kPropertyIdStrokeParams, &val); }
  //! Reset the stroke parameters.
  B2D_INLINE Error resetStrokeParams() noexcept { return impl()->resetProperty(kPropertyIdStrokeParams); }

  //! Get stroke width.
  B2D_INLINE Error getStrokeWidth(double& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeWidth, &val); }
  //! Set stroke width.
  B2D_INLINE Error setStrokeWidth(double val) noexcept { return impl()->setProperty(kPropertyIdStrokeWidth, &val); }
  //! Reset stroke width.
  B2D_INLINE Error resetStrokeWidth() noexcept { return impl()->resetProperty(kPropertyIdStrokeWidth); }

  //! Get the miter limit.
  B2D_INLINE Error getMiterLimit(double& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeMiterLimit, &val); }
  //! Set the miter limit.
  B2D_INLINE Error setMiterLimit(double val) noexcept { return impl()->setProperty(kPropertyIdStrokeMiterLimit, &val); }
  //! Reset the miter limit.
  B2D_INLINE Error resetMiterLimit() noexcept { return impl()->resetProperty(kPropertyIdStrokeMiterLimit); }

  //! Get the dash-offset.
  B2D_INLINE Error getDashOffset(double& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeDashOffset, &val); }
  //! Set the dash-offset.
  B2D_INLINE Error setDashOffset(double val) noexcept { return impl()->setProperty(kPropertyIdStrokeDashOffset, &val); }
  //! Reset the dash-offset.
  B2D_INLINE Error resetDashOffset() noexcept { return impl()->resetProperty(kPropertyIdStrokeDashOffset); }

  //! Get the dash-array.
  B2D_INLINE Error getDashArray(Array<double>& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeDashArray, &val); }
  //! Set the dash-array.
  B2D_INLINE Error setDashArray(const Array<double>& val) noexcept { return impl()->setProperty(kPropertyIdStrokeDashArray, &val); }
  //! Reset the dash-array.
  B2D_INLINE Error resetDashArray() noexcept { return impl()->resetProperty(kPropertyIdStrokeDashArray); }

  //! Get stroke join, see \ref StrokeJoin::Type.
  B2D_INLINE Error getStrokeJoin(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeJoin, &val); }
  //! Set stroke join, see \ref StrokeJoin::Type.
  B2D_INLINE Error setStrokeJoin(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdStrokeJoin, &val); }
  //! Reset stroke join.
  B2D_INLINE Error resetStrokeJoin() noexcept { return impl()->resetProperty(kPropertyIdStrokeJoin); }

  //! Get the start cap, see \ref StrokeCap::Type.
  B2D_INLINE Error getStartCap(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeStartCap, &val); }
  //! Set the start cap, see \ref StrokeCap::Type.
  B2D_INLINE Error setStartCap(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdStrokeStartCap, &val); }
  //! Reset the start cap.
  B2D_INLINE Error resetStartCap() noexcept { return impl()->resetProperty(kPropertyIdStrokeStartCap); }

  //! Get the end cap, see \ref StrokeCap::Type.
  B2D_INLINE Error getEndCap(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeEndCap, &val); }
  //! Set the end cap, see \ref StrokeCap::Type.
  B2D_INLINE Error setEndCap(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdStrokeEndCap, &val); }
  //! Reset the start cap.
  B2D_INLINE Error resetEndCap() noexcept { return impl()->resetProperty(kPropertyIdStrokeEndCap); }

  //! Get both start and end caps.
  //!
  //! NOTE: This function only succeeds if both start and end caps are the same.
  B2D_INLINE Error getLineCaps(uint32_t val) noexcept { return impl()->getProperty(kPropertyIdStrokeLineCaps, &val); }
  //! Set both start and end caps.
  B2D_INLINE Error setLineCaps(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdStrokeLineCaps, &val); }
  //! Reset both start and end caps.
  B2D_INLINE Error resetLineCpas() noexcept { return impl()->resetProperty(kPropertyIdStrokeLineCaps); }

  //! Get the end cap, see \ref StrokeTransformOrder::Type.
  B2D_INLINE Error getStrokeTransformOrder(uint32_t& val) const noexcept { return impl()->getProperty(kPropertyIdStrokeTransformOrder, &val); }
  //! Set the end cap, see \ref StrokeTransformOrder::Type.
  B2D_INLINE Error setStrokeTransformOrder(uint32_t val) noexcept { return impl()->setProperty(kPropertyIdStrokeTransformOrder, &val); }
  //! Reset the start cap.
  B2D_INLINE Error resetStrokeTransformOrder() noexcept { return impl()->resetProperty(kPropertyIdStrokeTransformOrder); }

  // --------------------------------------------------------------------------
  // [Matrix]
  // --------------------------------------------------------------------------

  //! Get the current transformation matrix.
  B2D_INLINE Error getMatrix(Matrix2D& m) const noexcept { return impl()->getMatrix(m); }
  //! Set the current transformation matrix.
  B2D_INLINE Error setMatrix(const Matrix2D& m) noexcept { return impl()->setMatrix(m); }
  //! Matrix operation (internal).
  B2D_INLINE Error _matrixOp(uint32_t op, const void* data) noexcept { return impl()->matrixOp(op, data); }

  B2D_INLINE Error translate(double x, double y) noexcept { return translate(Point(x, y)); }
  B2D_INLINE Error translate(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpTranslateP, &p); }

  B2D_INLINE Error translateAppend(double x, double y) noexcept { return translateAppend(Point(x, y)); }
  B2D_INLINE Error translateAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpTranslateA, &p); }

  B2D_INLINE Error scale(double xy) noexcept { return scale(Point(xy, xy)); }
  B2D_INLINE Error scale(double x, double y) noexcept { return scale(Point(x, y)); }
  B2D_INLINE Error scale(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpScaleP, &p); }

  B2D_INLINE Error scaleAppend(double xy) noexcept { return scaleAppend(Point(xy, xy)); }
  B2D_INLINE Error scaleAppend(double x, double y) noexcept { return scaleAppend(Point(x, y)); }
  B2D_INLINE Error scaleAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpScaleA, &p); }

  B2D_INLINE Error skew(double x, double y) noexcept { return skew(Point(x, y)); }
  B2D_INLINE Error skew(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpSkewP, &p); }

  B2D_INLINE Error skewAppend(double x, double y) noexcept { return skewAppend(Point(x, y)); }
  B2D_INLINE Error skewAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpSkewA, &p); }

  B2D_INLINE Error rotate(double angle) noexcept { return _matrixOp(Matrix2D::kOpRotateP, &angle); }
  B2D_INLINE Error rotateAppend(double angle) noexcept { return _matrixOp(Matrix2D::kOpRotateA, &angle); }

  B2D_INLINE Error rotate(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotate(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotateAppend(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error rotateAppend(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error transform(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyP, &m); }
  B2D_INLINE Error transformAppend(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyA, &m); }

  //! Reset the current transformation matrix to identity.
  B2D_INLINE Error resetMatrix() noexcept { return impl()->resetMatrix(); }

  //! Store the result of combining the current MetaMatrix and UserMatrix to
  //! MetaMatrix and reset UserMatrix to identity:
  //!
  //! ```
  //! MetaMatrix = MetaMatrix x UserMatrix
  //! UserMatrix = Identity
  //! ```
  //!
  //! This operation is irreversible. The only way to restore both matrices
  //! to the state before `userToMeta()` is to use `save()` and `restore()`
  //! functions.
  B2D_INLINE Error userToMeta() noexcept { return impl()->userToMeta(); }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Save the current Context2D state.
  B2D_INLINE Error save() noexcept { return impl()->save(nullptr); }
  //! Save the current Context2D state.
  B2D_INLINE Error save(Cookie& cookie) noexcept { return impl()->save(&cookie); }

  //! Restore the current Context2D state.
  B2D_INLINE Error restore() noexcept { return impl()->restore(nullptr); }
  //! Restore the current Context2D state.
  B2D_INLINE Error restore(const Cookie& cookie) noexcept { return impl()->restore(&cookie); }

  // --------------------------------------------------------------------------
  // [Group]
  // --------------------------------------------------------------------------

  B2D_INLINE Error beginGroup() noexcept { return impl()->beginGroup(); }
  B2D_INLINE Error paintGroup() noexcept { return impl()->paintGroup(); }

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  // TODO: clearAll() and clearRect()

  // --------------------------------------------------------------------------
  // [Fill]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Fill (internal).
  B2D_INLINE Error _fillArg(uint32_t id, const void* data) noexcept { return impl()->fillArg(id, data); }

  //! Fill all.
  B2D_INLINE Error fillAll() noexcept { return impl()->fillAll(); }

  //! Fill a box.
  B2D_INLINE Error fillBox(const Box& box) noexcept { return impl()->fillArg(kGeomArgBox, &box); }
  // \overload
  B2D_INLINE Error fillBox(const IntBox& box) noexcept { return impl()->fillArg(kGeomArgIntBox, &box); }
  // \overload
  B2D_INLINE Error fillBox(double x0, double y0, double x1, double y1) noexcept { return fillBox(Box(x0, y0, x1, y1)); }
  // \overload
  B2D_INLINE Error fillBox(int x0, int y0, int x1, int y1) noexcept { return fillBox(IntBox(x0, y0, x1, y1)); }

  //! Fill a rectangle.
  B2D_INLINE Error fillRect(const Rect& rect) noexcept { return impl()->fillRectD(rect); }
  //! \overload
  B2D_INLINE Error fillRect(const IntRect& rect) noexcept { return impl()->fillRectI(rect); }
  //! \overload
  B2D_INLINE Error fillRect(double x, double y, double w, double h) noexcept { return impl()->fillRectD(Rect(x, y, w, h)); }
  //! \overload
  B2D_INLINE Error fillRect(int x, int y, int w, int h) noexcept { return impl()->fillRectI(IntRect(x, y, w, h)); }

  //! Fill a circle.
  B2D_INLINE Error fillCircle(const Circle& circle) noexcept { return impl()->fillArg(kGeomArgCircle, &circle); }
  //! \overload
  B2D_INLINE Error fillCircle(double cx, double cy, double r) noexcept { return fillCircle(Circle(cx, cy, r)); }

  //! Fill an ellipse.
  B2D_INLINE Error fillEllipse(const Ellipse& ellipse) noexcept { return impl()->fillArg(kGeomArgEllipse, &ellipse); }
  //! \overload
  B2D_INLINE Error fillEllipse(double cx, double cy, double rx, double ry) noexcept { return fillEllipse(Ellipse(cx, cy, rx, ry)); }

  //! Fill a round.
  B2D_INLINE Error fillRound(const Round& round) noexcept { return impl()->fillArg(kGeomArgRound, &round); }
  //! \overload
  B2D_INLINE Error fillRound(const Rect& rect, double r) noexcept { return fillRound(Round(rect, r)); }
  //! \overload
  B2D_INLINE Error fillRound(const Rect& rect, double rx, double ry) noexcept { return fillRound(Round(rect, rx, ry)); }
  //! \overload
  B2D_INLINE Error fillRound(double x, double y, double w, double h, double r) noexcept { return fillRound(Round(x, y, w, h, r)); }
  //! \overload
  B2D_INLINE Error fillRound(double x, double y, double w, double h, double rx, double ry) noexcept { return fillRound(Round(x, y, w, h, rx, ry)); }

  //! Fill a chord.
  B2D_INLINE Error fillChord(const Arc& chord) noexcept { return impl()->fillArg(kGeomArgChord, &chord); }
  //! \overload
  B2D_INLINE Error fillChord(const Rect& rect, double start, double sweep) noexcept { return fillChord(Arc(rect, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillChord(const Box& box, double start, double sweep) noexcept { return fillChord(Arc(box, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillChord(double cx, double cy, double r, double start, double sweep) noexcept { return fillChord(Arc(cx, cy, r, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillChord(double cx, double cy, double rx, double ry, double start, double sweep) noexcept { return fillChord(Arc(cx, cy, rx, ry, start, sweep)); }

  //! Fill a pie.
  B2D_INLINE Error fillPie(const Arc& pie) noexcept { return impl()->fillArg(kGeomArgPie, &pie); }
  //! \overload
  B2D_INLINE Error fillPie(const Rect& rect, double start, double sweep) noexcept { return fillPie(Arc(rect, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillPie(const Box& box, double start, double sweep) noexcept { return fillPie(Arc(box, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillPie(double cx, double cy, double r, double start, double sweep) noexcept { return fillPie(Arc(cx, cy, r, start, sweep)); }
  //! \overload
  B2D_INLINE Error fillPie(double cx, double cy, double rx, double ry, double start, double sweep) noexcept { return fillPie(Arc(cx, cy, rx, ry, start, sweep)); }

  //! Fill a triangle.
  B2D_INLINE Error fillTriangle(const Triangle& triangle) noexcept { return impl()->fillArg(kGeomArgTriangle, &triangle); }
  //! \overload
  B2D_INLINE Error fillTriangle(double x0, double y0, double x1, double y1, double x2, double y2) noexcept { return fillTriangle(Triangle(x0, y0, x1, y1, x2, y2)); }

  //! Fill a polygon.
  B2D_INLINE Error fillPolygon(const CArray<Point>& poly) noexcept { return impl()->fillArg(kGeomArgPolygon, &poly); }
  //! \overload
  B2D_INLINE Error fillPolygon(const Point* poly, size_t count) noexcept { return fillPolygon(CArray<Point>(poly, count)); }

  //! Fill a polygon.
  B2D_INLINE Error fillPolygon(const CArray<IntPoint>& poly) noexcept { return impl()->fillArg(kGeomArgIntPolygon, &poly); }
  //! \overload
  B2D_INLINE Error fillPolygon(const IntPoint* poly, size_t count) noexcept { return fillPolygon(CArray<IntPoint>(poly, count)); }

  //! Fill an array of boxes.
  B2D_INLINE Error fillBoxArray(const CArray<Box>& array) noexcept { return impl()->fillArg(kGeomArgArrayBox, &array); }
  //! \overload
  B2D_INLINE Error fillBoxArray(const Box* data, size_t count) noexcept { return fillBoxArray(CArray<Box>(data, count)); }

  //! Fill an array of boxes.
  B2D_INLINE Error fillBoxArray(const CArray<IntBox>& array) noexcept { return impl()->fillArg(kGeomArgArrayIntBox, &array); }
  //! \overload
  B2D_INLINE Error fillBoxArray(const IntBox* data, size_t count) noexcept { return fillBoxArray(CArray<IntBox>(data, count)); }

  //! Fill an array of rectangles.
  B2D_INLINE Error fillRectArray(const CArray<Rect>& array) noexcept { return impl()->fillArg(kGeomArgArrayRect, &array); }
  //! \overload
  B2D_INLINE Error fillRectArray(const Rect* data, size_t count) noexcept { return fillRectArray(CArray<Rect>(data, count)); }

  //! Fill an array of rectangles.
  B2D_INLINE Error fillRectArray(const CArray<IntRect>& array) noexcept { return impl()->fillArg(kGeomArgArrayIntRect, &array); }
  //! \overload
  B2D_INLINE Error fillRectArray(const IntRect* data, size_t count) noexcept { return fillRectArray(CArray<IntRect>(data, count)); }

  //! Fill a region.
  B2D_INLINE Error fillRegion(const Region& region) noexcept { return impl()->fillArg(kGeomArgRegion, &region); }

  //! Fill a path.
  B2D_INLINE Error fillPath(const Path2D& path) noexcept { return impl()->fillArg(kGeomArgPath2D, &path); }

  //! Fill the passed UTF-8 text by using the given `font`.
  B2D_INLINE Error fillUtf8Text(const Point& dst, const Font& font, const char* text, size_t size = Globals::kNullTerminated) noexcept {
    return impl()->fillText(dst, font, text, size, Unicode::kEncodingUtf8);
  }

  //! Fill the passed UTF-16 text by using the given `font`.
  B2D_INLINE Error fillUtf16Text(const Point& dst, const Font& font, const uint16_t* text, size_t size = Globals::kNullTerminated) noexcept {
    return impl()->fillText(dst, font, text, size, Unicode::kEncodingUtf16);
  }

  //! Fill the passed UTF-32 text by using the given `font`.
  B2D_INLINE Error fillUtf32Text(const Point& dst, const Font& font, const uint32_t* text, size_t size = Globals::kNullTerminated) noexcept {
    return impl()->fillText(dst, font, text, size, Unicode::kEncodingUtf32);
  }

  //! Fill the passed `glyphRun` by using the given `font`.
  B2D_INLINE Error fillGlyphRun(const Point& dst, const Font& font, const GlyphRun& glyphRun) noexcept { return impl()->fillGlyphRun(dst, font, glyphRun); }

  // --------------------------------------------------------------------------
  // [Stroke]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Stroke a path argument.
  B2D_INLINE Error _strokeArg(uint32_t id, const void* data) noexcept { return impl()->strokeArg(id, data); }

  //! Stroke a box.
  B2D_INLINE Error strokeBox(const Box& box) noexcept { return impl()->strokeArg(kGeomArgBox, &box); }
  // \overload
  B2D_INLINE Error strokeBox(const IntBox& box) noexcept { return impl()->strokeArg(kGeomArgIntBox, &box); }
  // \overload
  B2D_INLINE Error strokeBox(double x0, double y0, double x1, double y1) noexcept { return strokeBox(Box(x0, y0, x1, y1)); }
  // \overload
  B2D_INLINE Error strokeBox(int x0, int y0, int x1, int y1) noexcept { return strokeBox(IntBox(x0, y0, x1, y1)); }

  //! Stroke a rectangle.
  B2D_INLINE Error strokeRect(const Rect& rect) noexcept { return impl()->strokeRectD(rect); }
  //! \overload
  B2D_INLINE Error strokeRect(const IntRect& rect) noexcept { return impl()->strokeRectI(rect); }
  //! \overload
  B2D_INLINE Error strokeRect(double x, double y, double w, double h) noexcept { return impl()->strokeRectD(Rect(x, y, w, h)); }
  //! \overload
  B2D_INLINE Error strokeRect(int x, int y, int w, int h) noexcept { return impl()->strokeRectI(IntRect(x, y, w, h)); }

  //! Stroke a line.
  B2D_INLINE Error strokeLine(const Line& line) noexcept { return impl()->strokeArg(kGeomArgLine, &line); }
  //! \overload
  B2D_INLINE Error strokeLine(const Point& p0, const Point& p1) noexcept { return strokeLine(Line(p0, p1)); }
  //! \overload
  B2D_INLINE Error strokeLine(double x0, double y0, double x1, double y1) noexcept { return strokeLine(Line(x0, y0, x1, y1)); }

  //! Stroke a circle.
  B2D_INLINE Error strokeCircle(const Circle& circle) noexcept { return impl()->strokeArg(kGeomArgCircle, &circle); }
  //! \overload
  B2D_INLINE Error strokeCircle(double cx, double cy, double r) noexcept { return strokeCircle(Circle(cx, cy, r)); }

  //! Stroke an ellipse.
  B2D_INLINE Error strokeEllipse(const Ellipse& ellipse) noexcept { return impl()->strokeArg(kGeomArgEllipse, &ellipse); }
  //! \overload
  B2D_INLINE Error strokeEllipse(double cx, double cy, double rx, double ry) noexcept { return strokeEllipse(Ellipse(cx, cy, rx, ry)); }

  //! Stroke a round.
  B2D_INLINE Error strokeRound(const Round& round) noexcept { return impl()->strokeArg(kGeomArgRound, &round); }
  //! \overload
  B2D_INLINE Error strokeRound(const Rect& rect, double r) noexcept { return strokeRound(Round(rect, r)); }
  //! \overload
  B2D_INLINE Error strokeRound(const Rect& rect, double rx, double ry) noexcept { return strokeRound(Round(rect, rx, ry)); }
  //! \overload
  B2D_INLINE Error strokeRound(double x, double y, double w, double h, double r) noexcept { return strokeRound(Round(x, y, w, h, r)); }
  //! \overload
  B2D_INLINE Error strokeRound(double x, double y, double w, double h, double rx, double ry) noexcept { return strokeRound(Round(x, y, w, h, rx, ry)); }

  //! Stroke an arc.
  B2D_INLINE Error strokeArc(const Arc& arc) noexcept { return impl()->strokeArg(kGeomArgArc, &arc); }
  //! \overload
  B2D_INLINE Error strokeArc(const Rect& rect, double start, double sweep) noexcept { return strokeArc(Arc(rect, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeArc(const Box& box, double start, double sweep) noexcept { return strokeArc(Arc(box, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeArc(double cx, double cy, double r, double start, double sweep) noexcept { return strokeArc(Arc(cx, cy, r, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeArc(double cx, double cy, double rx, double ry, double start, double sweep) noexcept { return strokeArc(Arc(cx, cy, rx, ry, start, sweep)); }

  //! Stroke a chord.
  B2D_INLINE Error strokeChord(const Arc& chord) noexcept { return impl()->strokeArg(kGeomArgChord, &chord); }
  //! \overload
  B2D_INLINE Error strokeChord(const Rect& rect, double start, double sweep) noexcept { return strokeChord(Arc(rect, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeChord(const Box& box, double start, double sweep) noexcept { return strokeChord(Arc(box, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeChord(double cx, double cy, double r, double start, double sweep) noexcept { return strokeChord(Arc(cx, cy, r, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokeChord(double cx, double cy, double rx, double ry, double start, double sweep) noexcept { return strokeChord(Arc(cx, cy, rx, ry, start, sweep)); }

  //! Stroke a pie.
  B2D_INLINE Error strokePie(const Arc& pie) noexcept { return impl()->strokeArg(kGeomArgPie, &pie); }
  //! \overload
  B2D_INLINE Error strokePie(const Rect& rect, double start, double sweep) noexcept { return strokePie(Arc(rect, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokePie(const Box& box, double start, double sweep) noexcept { return strokePie(Arc(box, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokePie(double cx, double cy, double r, double start, double sweep) noexcept { return strokePie(Arc(cx, cy, r, start, sweep)); }
  //! \overload
  B2D_INLINE Error strokePie(double cx, double cy, double rx, double ry, double start, double sweep) noexcept { return strokePie(Arc(cx, cy, rx, ry, start, sweep)); }

  //! Stroke a triangle.
  B2D_INLINE Error strokeTriangle(const Triangle& triangle) noexcept { return impl()->strokeArg(kGeomArgTriangle, &triangle); }
  //! \overload
  B2D_INLINE Error strokeTriangle(double x0, double y0, double x1, double y1, double x2, double y2) noexcept { return strokeTriangle(Triangle(x0, y0, x1, y1, x2, y2)); }

  //! Stroke a polyline.
  B2D_INLINE Error strokePolyline(const CArray<Point>& poly) noexcept { return impl()->strokeArg(kGeomArgPolyline, &poly); }
  //! \overload
  B2D_INLINE Error strokePolyline(const Point* poly, size_t count) noexcept { return strokePolyline(CArray<Point>(poly, count)); }

  //! Stroke a polyline.
  B2D_INLINE Error strokePolyline(const CArray<IntPoint>& poly) noexcept { return impl()->strokeArg(kGeomArgPolyline, &poly); }
  //! \overload
  B2D_INLINE Error strokePolyline(const IntPoint* poly, size_t count) noexcept { return strokePolyline(CArray<IntPoint>(poly, count)); }

  //! Stroke a polygon.
  B2D_INLINE Error strokePolygon(const CArray<Point>& poly) noexcept { return impl()->strokeArg(kGeomArgPolygon, &poly); }
  //! \overload
  B2D_INLINE Error strokePolygon(const Point* poly, size_t count) noexcept { return strokePolygon(CArray<Point>(poly, count)); }

  //! Stroke a polygon.
  B2D_INLINE Error strokePolygon(const CArray<IntPoint>& poly) noexcept { return impl()->strokeArg(kGeomArgIntPolygon, &poly); }
  //! \overload
  B2D_INLINE Error strokePolygon(const IntPoint* poly, size_t count) noexcept { return strokePolygon(CArray<IntPoint>(poly, count)); }

  //! Stroke an array of boxes.
  B2D_INLINE Error strokeBoxArray(const CArray<Box>& array) noexcept { return impl()->strokeArg(kGeomArgArrayBox, &array); }
  //! \overload
  B2D_INLINE Error strokeBoxArray(const Box* data, size_t count) noexcept { return strokeBoxArray(CArray<Box>(data, count)); }

  //! Stroke an array of boxes.
  B2D_INLINE Error strokeBoxArray(const CArray<IntBox>& array) noexcept { return impl()->strokeArg(kGeomArgArrayIntBox, &array); }
  //! \overload
  B2D_INLINE Error strokeBoxArray(const IntBox* data, size_t count) noexcept { return strokeBoxArray(CArray<IntBox>(data, count)); }

  //! Stroke an array of rectangles.
  B2D_INLINE Error strokeRectArray(const CArray<Rect>& array) noexcept { return impl()->strokeArg(kGeomArgArrayRect, &array); }
  //! \overload
  B2D_INLINE Error strokeRectArray(const Rect* data, size_t count) noexcept { return strokeRectArray(CArray<Rect>(data, count)); }

  //! Stroke an array of rectangles.
  B2D_INLINE Error strokeRectArray(const CArray<IntRect>& array) noexcept { return impl()->strokeArg(kGeomArgArrayIntRect, &array); }
  //! \overload
  B2D_INLINE Error strokeRectArray(const IntRect* data, size_t count) noexcept { return strokeRectArray(CArray<IntRect>(data, count)); }

  //! Stroke a path.
  B2D_INLINE Error strokePath(const Path2D& path) noexcept { return impl()->strokeArg(kGeomArgPath2D, &path); }

  // --------------------------------------------------------------------------
  // [Blit]
  // --------------------------------------------------------------------------

  B2D_INLINE Error blitImage(const Point& dst, const Image& src) noexcept { return impl()->blitImagePD(dst, src, nullptr); }
  B2D_INLINE Error blitImage(const Point& dst, const Image& src, const IntRect& srcArea) noexcept { return impl()->blitImagePD(dst, src, &srcArea); }

  B2D_INLINE Error blitImage(const IntPoint& dst, const Image& src) noexcept { return impl()->blitImagePI(dst, src, nullptr); }
  B2D_INLINE Error blitImage(const IntPoint& dst, const Image& src, const IntRect& srcArea) noexcept { return impl()->blitImagePI(dst, src, &srcArea); }

  B2D_INLINE Error blitImage(const Rect& dst, const Image& src) noexcept { return impl()->blitImageRD(dst, src, nullptr); }
  B2D_INLINE Error blitImage(const Rect& dst, const Image& src, const IntRect& srcArea) noexcept { return impl()->blitImageRD(dst, src, &srcArea); }

  B2D_INLINE Error blitImage(const IntRect& dst, const Image& src) noexcept { return impl()->blitImageRI(dst, src, nullptr); }
  B2D_INLINE Error blitImage(const IntRect& dst, const Image& src, const IntRect& srcArea) noexcept { return impl()->blitImageRI(dst, src, &srcArea); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Context2D& operator=(const Context2D& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Context2D& operator=(Context2D&& other) noexcept {
    Context2DImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

// ============================================================================
// [b2d::Context2DHints]
// ============================================================================

struct Context2DHints {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _packed = 0;

    _compOp = CompOp::kSrcOver;
    _renderQuality = Context2D::kRenderQualityDefault;
    _gradientFilter = Gradient::kFilterDefault;
    _patternFilter = Pattern::kFilterDefault;
  }

  // --------------------------------------------------------------------------
  // [Valid]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isValid() const noexcept {
    return _compOp < CompOp::kCount &&
           _renderQuality < Context2D::kRenderQualityCount &&
           _gradientFilter < Gradient::kFilterCount &&
           _patternFilter < Pattern::kFilterCount;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint32_t _compOp         :  8;
      uint32_t _renderQuality  :  4;
      uint32_t _gradientFilter :  4;
      uint32_t _patternFilter  :  4;
      uint32_t _reserved       : 12;
    };

    uint32_t _packed;
  };
};

// ============================================================================
// [b2d::Context2DParams]
// ============================================================================

//! Context2D parameters.
//!
//! This structure can hold most of the `Context2D` parameters, except styles,
//! clipping, and effects. It can be used to fast set and get all of these
//! parameters to and from `Context2D`, respectively.
//!
//! It contains:
//!   - Context hints.
//!   - Compositing operator.
//!   - Global alpha.
//!   - Fill alpha and parameters.
//!   - Stroke alpha and parameters.
struct Context2DParams {
  // --------------------------------------------------------------------------
  // [CoreParams]
  // --------------------------------------------------------------------------

  struct CoreParams {
    B2D_INLINE void reset() noexcept {
      _hints.reset();
      _fillRule = FillRule::kDefault;

      _globalAlpha = 1.0;
      _styleAlpha[0] = 1.0;
      _styleAlpha[1] = 1.0;
    }

    B2D_INLINE bool isValid() const noexcept {
      return _hints.isValid() &&
        Math::isUnitInterval(_globalAlpha) &&
        Math::isUnitInterval(_styleAlpha[0]) &&
        Math::isUnitInterval(_styleAlpha[1]);
    }

    //! Context hints.
    Context2DHints _hints;
    //! Fill rule.
    uint32_t _fillRule;

    //! Global alpha value.
    double _globalAlpha;
    //! Fill & stroke alpha values.
    double _styleAlpha[Context2D::kStyleSlotCount];
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Context2DParams() noexcept {
    _coreParams.reset();
  }

  B2D_INLINE Context2DParams(const Context2DParams& other) noexcept
    : _coreParams(other._coreParams),
    _strokeParams(other._strokeParams) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _coreParams.reset();
    _strokeParams.reset();
  }

  // --------------------------------------------------------------------------
  // [Valid]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isValid() const noexcept {
    return _coreParams.isValid();
  }

  // --------------------------------------------------------------------------
  // [Context Hints]
  // --------------------------------------------------------------------------

  B2D_INLINE const Context2DHints& hints() const noexcept { return _coreParams._hints; }
  B2D_INLINE void setHints(const Context2DHints& hints) noexcept { _coreParams._hints._packed = hints._packed; }

  B2D_INLINE uint32_t compOp() const noexcept { return _coreParams._hints._compOp; }
  B2D_INLINE void setCompOp(uint32_t op) noexcept { _coreParams._hints._compOp = op; }

  B2D_INLINE uint32_t renderQuality() const noexcept { return _coreParams._hints._renderQuality; }
  B2D_INLINE void setRenderQuality(uint32_t hint) noexcept { _coreParams._hints._renderQuality = hint; }

  B2D_INLINE uint32_t gradientFilter() const noexcept { return _coreParams._hints._gradientFilter; }
  B2D_INLINE void setGradientFilter(uint32_t filter) noexcept { _coreParams._hints._gradientFilter = filter; }

  B2D_INLINE uint32_t patternFilter() const noexcept { return _coreParams._hints._patternFilter; }
  B2D_INLINE void setPatternFilter(uint32_t filter) noexcept { _coreParams._hints._patternFilter = filter; }

  // --------------------------------------------------------------------------
  // [Global Alpha]
  // --------------------------------------------------------------------------

  B2D_INLINE double globalAlpha() const noexcept { return _coreParams._globalAlpha; }
  B2D_INLINE void setGlobalAlpha(double value) noexcept { _coreParams._globalAlpha = value; }

  // --------------------------------------------------------------------------
  // [Fill Alpha]
  // --------------------------------------------------------------------------

  B2D_INLINE double fillAlpha() const noexcept { return _coreParams._styleAlpha[Context2D::kStyleSlotFill]; }
  B2D_INLINE void setFillAlpha(double value) noexcept { _coreParams._styleAlpha[Context2D::kStyleSlotFill] = value; }

  // --------------------------------------------------------------------------
  // [Fill Params]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t fillRule() const noexcept { return _coreParams._fillRule; }
  B2D_INLINE void setFillRule(uint32_t fillRule) noexcept { _coreParams._fillRule = fillRule; }

  // --------------------------------------------------------------------------
  // [Stroke Alpha]
  // --------------------------------------------------------------------------

  B2D_INLINE double strokeAlpha() const noexcept { return _coreParams._styleAlpha[Context2D::kStyleSlotStroke]; }
  B2D_INLINE void setStrokeAlpha(double value) noexcept { _coreParams._styleAlpha[Context2D::kStyleSlotStroke] = value; }

  // --------------------------------------------------------------------------
  // [Stroke Params]
  // --------------------------------------------------------------------------

  B2D_INLINE const StrokeParams& strokeParams() const noexcept { return _strokeParams; }
  B2D_INLINE void setStrokeParams(const StrokeParams& params) noexcept { _strokeParams = params; }

  B2D_INLINE double strokeWidth() const noexcept { return _strokeParams.strokeWidth(); }
  B2D_INLINE double miterLimit() const noexcept { return _strokeParams.miterLimit(); }
  B2D_INLINE double dashOffset() const noexcept { return _strokeParams.dashOffset(); }
  B2D_INLINE const Array<double>& dashArray() const noexcept { return _strokeParams.dashArray(); }

  B2D_INLINE uint32_t startCap() const noexcept { return _strokeParams.startCap(); }
  B2D_INLINE uint32_t endCap() const noexcept { return _strokeParams.endCap(); }
  B2D_INLINE uint32_t strokeJoin() const noexcept { return _strokeParams.strokeJoin(); }

  B2D_INLINE void setStrokeWidth(double strokeWidth) noexcept { _strokeParams.setStrokeWidth(strokeWidth); }
  B2D_INLINE void setMiterLimit(double miterLimit) noexcept { _strokeParams.setMiterLimit(miterLimit); }
  B2D_INLINE void setDashOffset(double dashOffset) noexcept { _strokeParams.setDashOffset(dashOffset); }
  B2D_INLINE void setDashArray(const Array<double>& dashArray) noexcept { _strokeParams.setDashArray(dashArray); }
  B2D_INLINE Error setDashArray(const double* dashArray, size_t size) noexcept { return _strokeParams.setDashArray(dashArray, size); }

  B2D_INLINE void setStartCap(uint32_t startCap) noexcept { _strokeParams.setStartCap(startCap); }
  B2D_INLINE void setEndCap(uint32_t endCap) noexcept { _strokeParams.setEndCap(endCap); }
  B2D_INLINE void setLineCaps(uint32_t lineCaps) noexcept { _strokeParams.setLineCaps(lineCaps); }
  B2D_INLINE void setStrokeJoin(uint32_t join) noexcept { _strokeParams.setStrokeJoin(join); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  Context2DParams& operator=(const Context2DParams& other) noexcept {
    _coreParams = other._coreParams;
    _strokeParams = other._strokeParams;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Core parameters.
  CoreParams _coreParams;
  //! Stroke parameters.
  StrokeParams _strokeParams;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_CONTEXT2D_H
