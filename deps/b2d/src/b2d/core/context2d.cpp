// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/context2d.h"
#include "../rasterengine/rastercontext2d_p.h"
#include "../core/runtime.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Context2DImpl]
// ============================================================================

Context2DImpl::Context2DImpl() noexcept
  : ObjectImpl(Any::kTypeIdContext2D),
    _implType(Context2D::kImplTypeNone),
    _contextFlags(0),
    _targetSizeI(0, 0),
    _targetSizeD(0.0, 0.0) {}
Context2DImpl::~Context2DImpl() noexcept {}

// ============================================================================
// [b2d::NullContext2DImpl]
// ============================================================================

//! \internal
//!
//! Null-painter implementation
struct NullContext2DImpl : public Context2DImpl {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  NullContext2DImpl() noexcept { _commonData._setToBuiltInNone(); }
  void destroy() noexcept override {}

  // --------------------------------------------------------------------------
  // [Impl]
  // --------------------------------------------------------------------------

  #define INV return DebugUtils::errored(kErrorInvalidState)

  Error flush(uint32_t flags) noexcept override { INV; }

  Error getProperty(uint32_t id, void* value) const noexcept override { INV; }
  Error setProperty(uint32_t id, const void* value) noexcept override { INV; }
  Error resetProperty(uint32_t id) noexcept override { INV; }

  Error getStyleType(uint32_t slot, uint32_t& type) const noexcept override { type = Context2D::kStyleTypeNone; INV; }
  Error getStyleArgb32(uint32_t slot, uint32_t& argb32) const noexcept override { argb32 = 0; INV; }
  Error getStyleArgb64(uint32_t slot, uint64_t& argb64) const noexcept override { argb64 = 0; INV; }
  Error getStyleObject(uint32_t slot, Object* obj) const noexcept override { INV; }

  Error setStyleArgb32(uint32_t slot, uint32_t argb32) noexcept override { INV; }
  Error setStyleArgb64(uint32_t slot, uint64_t argb64) noexcept override { INV; }
  Error setStyleObject(uint32_t slot, const Object* obj) noexcept override { INV; }
  Error resetStyle(uint32_t slot) noexcept override { INV; }

  Error getMatrix(Matrix2D& m) const noexcept override { m.reset(); INV; }
  Error setMatrix(const Matrix2D& m) noexcept override { INV; }
  Error matrixOp(uint32_t opId, const void* data) noexcept override { INV; }
  Error resetMatrix() noexcept override { INV; }
  Error userToMeta() noexcept override { INV; }

  Error save(Cookie* cookie) noexcept override { INV; }
  Error restore(const Cookie* cookie) noexcept override { INV; }

  Error beginGroup() noexcept override { INV; }
  Error paintGroup() noexcept override { INV; }

  Error fillAll() noexcept override { INV; }
  Error fillArg(uint32_t id, const void* data) noexcept override { INV; }
  Error fillRectD(const Rect& rect) noexcept override { INV; }
  Error fillRectI(const IntRect& rect) noexcept override { INV; }
  Error fillText(const Point& dst, const Font& font, const void* text, size_t size, uint32_t encoding) noexcept override { INV; }
  Error fillGlyphRun(const Point& dst, const Font& font, const GlyphRun& glyphRun) noexcept override { INV; }

  Error strokeArg(uint32_t id, const void* data) noexcept override { INV; }
  Error strokeRectD(const Rect& rect) noexcept override { INV; }
  Error strokeRectI(const IntRect& rect) noexcept override { INV; }

  Error blitImagePD(const Point& dst, const Image& src, const IntRect* srcArea) noexcept override { INV; }
  Error blitImagePI(const IntPoint& dst, const Image& src, const IntRect* srcArea) noexcept override { INV; }
  Error blitImageRD(const Rect& dst, const Image& src, const IntRect* srcArea) noexcept override { INV; }
  Error blitImageRI(const IntRect& dst, const Image& src, const IntRect* srcArea) noexcept override { INV; }

  #undef INV
};
static Wrap<NullContext2DImpl> _bNullContext2DImpl;

// ============================================================================
// [b2d::Context2D - Begin / End]
// ============================================================================

Error Context2D::_begin(Image& image, const InitParams* initParams) noexcept {
  InitParams noParams;
  Context2DImpl* newI = RasterEngine::RasterContext2DImpl::_create(image, initParams ? initParams : &noParams);

  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

Error Context2D::end() noexcept {
  return AnyInternal::replaceImpl(this, Any::noneOf<Context2D>().impl());
}

// ============================================================================
// [b2d::Context2D - Runtime Handlers]
// ============================================================================

void Context2DOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  Context2DImpl* impl = _bNullContext2DImpl.init();
  impl->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdContext2D, impl);
}

B2D_END_NAMESPACE
