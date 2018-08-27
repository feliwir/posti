// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"
#include "../text/fontmatcher.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Font - Globals]
// ============================================================================

static Wrap<FontMatcherImpl> gFontMatcherImplNone;

// ============================================================================
// [b2d::FontMatcher - Impl]
// ============================================================================

FontMatcherImpl::FontMatcherImpl() noexcept
  : ObjectImpl(Any::kTypeIdFontMatcher) {}

FontMatcherImpl::~FontMatcherImpl() noexcept {}

// ============================================================================
// [b2d::FontMatcher - Runtime Handlers]
// ============================================================================

void FontMatcherOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontMatcherImpl* fontMatcherI = gFontMatcherImplNone.init();
  fontMatcherI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFontMatcher, fontMatcherI);
}

B2D_END_NAMESPACE
