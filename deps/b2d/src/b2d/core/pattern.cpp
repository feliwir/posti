// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/pattern.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Pattern - Globals]
// ============================================================================

static Wrap<PatternImpl> gPatternImplNone;

static const IntRect gPatternAreaNone = { 0, 0, 0, 0 };
static const Matrix2D gPatternMatrixNone = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };

// ============================================================================
// [b2d::Pattern - Impl]
// ============================================================================

PatternImpl::PatternImpl() noexcept
  : ObjectImpl(Any::kTypeIdPattern),
    _image(),
    _area(0, 0, 0, 0),
    _matrix(Matrix2D::identity()) {}

PatternImpl::PatternImpl(const Image& image, const IntRect& area, uint32_t extend, const Matrix2D& m, uint32_t mType) noexcept
  : ObjectImpl(Any::kTypeIdPattern),
    _image(image),
    _area(area),
    _matrix(m) {
  setExtend(extend);
  setMatrixType(mType);
}

PatternImpl::~PatternImpl() noexcept {}

// ============================================================================
// [b2d::Pattern - Construction / Destruction]
// ============================================================================

Pattern::Pattern(const Image* image, const IntRect* area, uint32_t extend, const Matrix2D* m) noexcept
  : Object(static_cast<PatternImpl*>(nullptr)) {

  uint32_t mType = Matrix2D::kTypeIdentity;
  if (!area)
    area = &gPatternAreaNone;

  if (!m)
    m = &gPatternMatrixNone;
  else
    mType = m->type();

  _impl = AnyInternal::fallback(PatternImpl::new_(*image, *area, extend, *m, mType), Pattern::none().impl());
}

// ============================================================================
// [b2d::Pattern - Sharing]
// ============================================================================

Error Pattern::_detach() noexcept {
  if (!isShared())
    return kErrorOk;

  PatternImpl* thisI = impl();
  PatternImpl* newI = PatternImpl::new_(thisI->image(), thisI->area(), thisI->extend(), thisI->matrix(), thisI->matrixType());

  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Pattern - Reset]
// ============================================================================

Error Pattern::reset() noexcept {
  return AnyInternal::replaceImpl(this, none().impl());
}

// ============================================================================
// [b2d::Pattern - Setup]
// ============================================================================

Error Pattern::_setupPattern(const Image* image, const IntRect* area, uint32_t extend, const Matrix2D* _matrix) noexcept {
  PatternImpl* thisI = impl();

  Matrix2D matrix;
  uint32_t mType = Matrix2D::kTypeIdentity;

  if (area == nullptr)
    area = &gPatternAreaNone;

  if (_matrix) {
    matrix = *_matrix;
    mType = matrix.type();
  }

  if (thisI->isShared()) {
    PatternImpl* newI = PatternImpl::new_(*image, *area, extend, matrix, mType);
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);
    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->setImage(*image);
    thisI->setArea(*area);
    thisI->setExtend(extend);
    thisI->setMatrix(matrix);
    thisI->setMatrixType(mType);
    return kErrorOk;
  }
}

Error Pattern::setDeep(const Pattern& other) noexcept {
  PatternImpl* thisI = impl();
  PatternImpl* otherI = other.impl();

  if (thisI == otherI)
    return detach();

  if (thisI->isShared()) {
    PatternImpl* newI = PatternImpl::new_(otherI->image(), otherI->area(), otherI->extend(), otherI->matrix(), otherI->matrixType());
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);
    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->setImage(otherI->image());
    thisI->setArea(otherI->area());
    thisI->setExtend(otherI->extend());
    thisI->setMatrix(otherI->matrix());
    thisI->setMatrixType(otherI->matrixType());
    return kErrorOk;
  }
}

// ============================================================================
// [b2d::Pattern - Accessors]
// ============================================================================

Error Pattern::setImage(const Image& image) noexcept {
  PatternImpl* thisI = impl();
  if (thisI->image().impl() == image.impl())
    return kErrorOk;

  if (thisI->isShared()) {
    PatternImpl* newI = PatternImpl::new_(image, gPatternAreaNone, thisI->extend(), thisI->matrix(), thisI->matrixType());
    if (B2D_UNLIKELY(!newI))
      return DebugUtils::errored(kErrorNoMemory);
    return AnyInternal::replaceImpl(this, newI);
  }
  else {
    thisI->setImage(image);
    thisI->_area.reset();
    return kErrorOk;
  }
}

Error Pattern::resetImage() noexcept {
  return setImage(Image::none());
}

Error Pattern::setArea(const IntRect& area) noexcept {
  if (!area.isValid())
    return resetArea();

  if (impl()->_area == area)
    return kErrorOk;

  B2D_PROPAGATE(detach());
  impl()->_area = area;
  return kErrorOk;
}

Error Pattern::resetArea() noexcept {
  if (impl()->_area.w() == 0)
    return kErrorOk;

  B2D_PROPAGATE(detach());
  impl()->_area.reset();
  return kErrorOk;
}

Error Pattern::setExtend(uint32_t extend) noexcept {
  if (impl()->extend() == extend)
    return kErrorOk;

  if (B2D_UNLIKELY(extend >= kExtendCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  B2D_PROPAGATE(detach());
  impl()->setExtend(extend);
  return kErrorOk;
}

Error Pattern::resetExtend() noexcept {
  return setExtend(kExtendDefault);
}

// ============================================================================
// [b2d::Pattern - Operations]
// ============================================================================

Error Pattern::setMatrix(const Matrix2D& m) noexcept {
  B2D_PROPAGATE(detach());
  impl()->setMatrix(m);
  impl()->setMatrixType(m.type());
  return kErrorOk;
}

Error Pattern::resetMatrix() noexcept {
  if (matrixType() == Matrix2D::kTypeIdentity)
    return kErrorOk;

  B2D_PROPAGATE(detach());
  impl()->_matrix.reset();
  impl()->setMatrixType(Matrix2D::kTypeIdentity);
  return kErrorOk;
}

Error Pattern::_matrixOp(uint32_t op, const void* params) noexcept {
  B2D_PROPAGATE(detach());
  impl()->_matrix._matrixOp(op, params);
  impl()->setMatrixType(impl()->matrix().type());
  return kErrorOk;
}

// ============================================================================
// [b2d::Pattern - Equality]
// ============================================================================

bool B2D_CDECL Pattern::_eq(const Pattern* a, const Pattern* b) noexcept {
  const PatternImpl* aI = a->impl();
  const PatternImpl* bI = b->impl();

  if (aI == bI)
    return true;

  return aI->_commonData._extra32 == bI->_commonData._extra32 &&
         aI->matrix() == bI->matrix() &&
         aI->area()   == bI->area()   &&
         aI->image().eq(bI->image());
}

// ============================================================================
// [b2d::Pattern - Runtime Handlers]
// ============================================================================

void PatternOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  PatternImpl* impl = gPatternImplNone.init();
  impl->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdPattern, impl);

  // Checks whether the initialization order is correct.
  B2D_ASSERT(impl->image().impl() != nullptr);
}

B2D_END_NAMESPACE
