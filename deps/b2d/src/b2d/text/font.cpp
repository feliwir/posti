// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"
#include "../text/font.h"
#include "../text/glyphoutlinedecoder.h"
#include "../text/glyphrun.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Font - Globals]
// ============================================================================

static Wrap<FontImpl> gFontImplNone;

// ============================================================================
// [b2d::Font - Impl]
// ============================================================================

FontImpl::FontImpl(const FontFace& face, float size) noexcept
  : ObjectImpl(Any::kTypeIdFont),
    _face(face),
    _size(size) {
  face.calcFontProperties(_matrix, _metrics, size);
}
FontImpl::~FontImpl() noexcept {}

// ============================================================================
// [b2d::Font - Construction / Destruction]
// ============================================================================

Error Font::createFromFace(const FontFace& face, float size) noexcept {
  if (face.isNone())
    return DebugUtils::errored(kErrorFontNotInitialized);

  FontImpl* newI = FontImpl::new_(face, size);
  if (B2D_UNLIKELY(!newI))
    return DebugUtils::errored(kErrorNoMemory);

  return AnyInternal::replaceImpl(this, newI);
}

// ============================================================================
// [b2d::Font - Interface]
// ============================================================================

Error Font::decodeGlyphRunOutlines(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out) noexcept {
  GlyphOutlineDecoder decoder;
  B2D_PROPAGATE(decoder.createFromFont(*this));
  return decoder.decodeGlyphRun(glyphRun, userMatrix, out);
}

// ============================================================================
// [b2d::Font - Runtime Handlers]
// ============================================================================

void FontOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontImpl* fontI = gFontImplNone.init(FontFace::none(), 0.0f);
  fontI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFont, fontI);
}

B2D_END_NAMESPACE
