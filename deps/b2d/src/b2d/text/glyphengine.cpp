// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../text/font.h"
#include "../text/fontface.h"
#include "../text/glyphengine.h"
#include "../text/otface_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::GlyphEngine - Construction / Destruction]
// ============================================================================

GlyphEngine::GlyphEngine() noexcept { _fontFace = _font.face(); }
GlyphEngine::~GlyphEngine() noexcept {}

// ============================================================================
// [b2d::GlyphEngine - Reset / Create]
// ============================================================================

void GlyphEngine::reset() noexcept {
  _font.reset();
  _fontFace = _font.face();
  _fontData.reset();
}

Error GlyphEngine::createFromFont(const Font& font) noexcept {
  const FontFace& face = font.face();
  if (face.isNone())
    return DebugUtils::errored(kErrorFontNotInitialized);

  // If `fontDataI` is none it means it's the build-in none instance. It's an
  // ethernal instance that doesn't have to be released (releasing is NOP) so
  // we don't bother releasing it and just return quickly.
  FontDataImpl* fontDataI = face.impl()->_loader._fontDataImpl(face.faceIndex());
  if (fontDataI->isNone()) {
    return DebugUtils::errored(kErrorFontNotInitialized);
  }
  else {
    OT::OTFaceImpl* faceI = face.impl<OT::OTFaceImpl>();
    B2D_PROPAGATE(fontDataI->loadSlots(
      Support::bitMask<uint32_t>(
        FontData::kSlotCFF ,
        FontData::kSlotCFF2,
        FontData::kSlotCMap,
        FontData::kSlotGDef,
        FontData::kSlotGPos,
        FontData::kSlotGSub,
        FontData::kSlotGlyf,
        FontData::kSlotHMtx,
        FontData::kSlotKern,
        FontData::kSlotLoca,
        FontData::kSlotVMtx)));

    // This would mean that the data has changed, we don't allow that once the data is loaded.
    if (B2D_UNLIKELY(fontDataI->loadedTableSizeBySlot(FontData::kSlotCMap) < faceI->_cMapData.dataRange().length()))
      goto InvalidData;

    AnyInternal::replaceImpl(&_font, font.impl()->addRef());
    AnyInternal::replaceImpl(&_fontData, fontDataI);

    _fontFace = face;
    return kErrorOk;
  }

InvalidData:
  fontDataI->release();
  return DebugUtils::errored(kErrorFontInvalidData);
}

B2D_END_NAMESPACE
