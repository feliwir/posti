// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"
#include "../core/uniqueidgenerator_p.h"
#include "../text/font.h"
#include "../text/fontface.h"
#include "../text/otface_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::FontFace - Globals]
// ============================================================================

static Wrap<FontFaceImpl> gFontFaceImplNone;
static UniqueIdGenerator gFontFaceIdGenerator;

// ============================================================================
// [b2d::FontFace - None Implementation]
// ============================================================================

static Error B2D_CDECL FontFaceNone_mapGlyphItems(const GlyphEngine* self, GlyphItem* glyphItems, size_t count, GlyphMappingState* state) noexcept {
  B2D_UNUSED(self);
  B2D_UNUSED(glyphItems);
  B2D_UNUSED(count);

  state->_glyphCount = 0;
  state->_undefinedCount = 0;
  state->_undefinedFirst = 0;

  Error err = self->fontFace().isNone() ? kErrorFontNotInitialized : kErrorFontMissingCMap;
  return DebugUtils::errored(err);
}

static Error B2D_CDECL FontFaceNone_getGlyphBoundingBoxes(const GlyphEngine* self, const GlyphId* glyphIds, size_t glyphIdAdvance, IntBox* boxes, size_t count) noexcept {
  B2D_UNUSED(self);
  B2D_UNUSED(glyphIds);
  B2D_UNUSED(glyphIdAdvance);
  B2D_UNUSED(boxes);
  B2D_UNUSED(count);

  return DebugUtils::errored(kErrorFontNotInitialized);
}

static Error B2D_CDECL FontFaceNone_getGlyphAdvances(const GlyphEngine* self, const GlyphItem* glyphItems, Point* glyphOffsets, size_t count) noexcept {
  B2D_UNUSED(self);
  B2D_UNUSED(glyphItems);
  B2D_UNUSED(glyphOffsets);
  B2D_UNUSED(count);

  return DebugUtils::errored(kErrorFontNotInitialized);
}

static Error B2D_CDECL FontFaceNone_offsetGlyphs(const GlyphEngine* self, GlyphItem* glyphItems, Point* glyphOffsets, size_t count) noexcept {
  B2D_UNUSED(self);
  B2D_UNUSED(glyphItems);
  B2D_UNUSED(glyphOffsets);
  B2D_UNUSED(count);

  return DebugUtils::errored(kErrorFontNotInitialized);
}

static Error B2D_CDECL FontFaceNone_decodeGlyph(
  GlyphOutlineDecoder* self,
  uint32_t glyphId,
  const Matrix2D& userMatrix,
  Path2D& out,
  MemBuffer& tmpBuffer,
  GlyphOutlineSinkFunc sink, size_t sinkGlyphIndex, void* closure) noexcept {

  B2D_UNUSED(self);
  B2D_UNUSED(glyphId);
  B2D_UNUSED(userMatrix);
  B2D_UNUSED(out);
  B2D_UNUSED(tmpBuffer);
  B2D_UNUSED(sink);
  B2D_UNUSED(sinkGlyphIndex);
  B2D_UNUSED(closure);

  return DebugUtils::errored(kErrorFontNotInitialized);
}

// ============================================================================
// [b2d::FontFace - Impl - Construction / Destruction]
// ============================================================================

FontFaceImpl::FontFaceImpl(const FontLoader& loader, uint32_t faceIndex) noexcept
  : ObjectImpl(Any::kTypeIdFontFace),
    _loader(loader),
    _faceIndex(faceIndex),
    _faceFlags(0),
    _faceUniqueId(0),
    _implType(uint8_t(FontFace::kImplTypeNone)),
    _reserved(0),
    _glyphCount(0),
    _unitsPerEm(0),
    _weight(Font::kWeightNormal),
    _stretch(Font::kStretchNormal),
    _style(Font::kStyleNormal),
    _diagnosticsInfo {},
    _panose {},
    _unicodeCoverage {},
    _designMetrics {},
    _funcs {} {
  // Functions that don't provide any implementation.
  _funcs.mapGlyphItems         = FontFaceNone_mapGlyphItems;
  _funcs.getGlyphBoundingBoxes = FontFaceNone_getGlyphBoundingBoxes;
  _funcs.getGlyphAdvances      = FontFaceNone_getGlyphAdvances;
  _funcs.applyPairAdjustment   = FontFaceNone_offsetGlyphs;
  _funcs.decodeGlyph           = FontFaceNone_decodeGlyph;
}

FontFaceImpl::~FontFaceImpl() noexcept {}

// ============================================================================
// [b2d::FontFace - Impl - Interface]
// ============================================================================

Error FontFaceImpl::initFace(FontData& fontData) noexcept {
  B2D_UNUSED(fontData);
  return kErrorOk;
}

// ============================================================================
// [b2d::FontFace - Construction / Destruction]
// ============================================================================

Error FontFace::createFromFile(const char* fileName) noexcept {
  FontLoader loader;
  B2D_PROPAGATE(loader.createFromFile(fileName));
  return createFromLoader(loader, 0);
}

Error FontFace::createFromLoader(const FontLoader& loader, uint32_t faceIndex) noexcept {
  if (loader.isNone())
    return DebugUtils::errored(kErrorFontNotInitialized);

  FontData fontData { loader.fontData(faceIndex) };
  if (fontData.empty())
    return DebugUtils::errored(loader.isNone() ? kErrorInvalidArgument : kErrorNoMemory);

  OT::OTFaceImpl* newI = AnyInternal::newImplT<OT::OTFaceImpl>(loader, faceIndex);
  if (!newI)
    return DebugUtils::errored(kErrorNoMemory);

  Error err = newI->initFace(fontData);
  if (err) {
    newI->destroy();
    return err;
  }

  newI->_faceUniqueId = gFontFaceIdGenerator.next();
  return AnyInternal::replaceImpl(this, newI);
}

FontData FontFace::fontData() const noexcept {
  return impl()->loader().fontData(faceIndex());
}

Error FontFace::listTags(Array<uint32_t>& out) noexcept {
  FontData fd = fontData();
  return fd.listTags(out);
}

// ============================================================================
// [b2d::FontFace - Interface]
// ============================================================================

Error FontFace::calcFontProperties(FontMatrix& matrixOut, FontMetrics& metricsOut, float size) const noexcept {
  typedef TextOrientation TO;

  const FontFaceImpl* thisI = impl();
  const FontDesignMetrics& dm = thisI->designMetrics();

  double yScale = thisI->unitsPerEm() ? double(size) / double(int(thisI->unitsPerEm())) : 0.0;
  double xScale = yScale;

  matrixOut.reset(xScale, 0.0, 0.0, -yScale);

  metricsOut._ascent[TO::kHorizontal]  = float(double(dm.ascent()                ) * yScale);
  metricsOut._descent[TO::kHorizontal] = float(double(dm.descent()               ) * yScale);
  metricsOut._lineGap                  = float(double(dm.lineGap()               ) * yScale);
  metricsOut._xHeight                  = float(double(dm.xHeight()               ) * yScale);
  metricsOut._capHeight                = float(double(dm.capHeight()             ) * yScale);
  metricsOut._ascent[TO::kVertical]    = float(double(dm.vertAscent()            ) * yScale);
  metricsOut._descent[TO::kVertical]   = float(double(dm.vertDescent()           ) * yScale);
  metricsOut._underlinePosition        = float(double(dm.underlinePosition()     ) * yScale);
  metricsOut._underlineThickness       = float(double(dm.underlineThickness()    ) * yScale);
  metricsOut._strikethroughPosition    = float(double(dm.strikethroughPosition() ) * yScale);
  metricsOut._strikethroughThickness   = float(double(dm.strikethroughThickness()) * yScale);

  return kErrorOk;
}

// ============================================================================
// [b2d::FontFace - Runtime Handlers]
// ============================================================================

void FontFaceOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontFaceImpl* fontFaceI = gFontFaceImplNone.init(FontLoader::none(), 0);
  fontFaceI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFontFace, fontFaceI);
}

B2D_END_NAMESPACE
