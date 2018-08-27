// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"
#include "../text/fontcollection.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::FontCollection - Globals]
// ============================================================================

static Wrap<FontCollectionImpl> gFontCollectionImplNone;

// ============================================================================
// [b2d::FontCollection - Impl]
// ============================================================================

FontCollectionImpl::FontCollectionImpl() noexcept
  : ObjectImpl(Any::kTypeIdFontCollection) {}

FontCollectionImpl::~FontCollectionImpl() noexcept {}

// ============================================================================
// [b2d::FontCollection - Runtime Handlers]
// ============================================================================

void FontCollectionOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontCollectionImpl* fontCollectionI = gFontCollectionImplNone.init();
  fontCollectionI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFontCollection, fontCollectionI);
}

B2D_END_NAMESPACE
