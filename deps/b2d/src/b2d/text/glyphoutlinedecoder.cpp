// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/support.h"
#include "../text/glyphoutlinedecoder.h"
#include "../text/otcff_p.h"
#include "../text/otface_p.h"
#include "../text/otglyf_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::GlyphOutlineDecoder - Create / Reset]
// ============================================================================

GlyphOutlineDecoder::GlyphOutlineDecoder() noexcept {}
GlyphOutlineDecoder::~GlyphOutlineDecoder() noexcept {}

Error GlyphOutlineDecoder::createFromFont(const Font& font) noexcept {
  const FontFace& face = font.face();
  if (face.isNone())
    return DebugUtils::errored(kErrorFontInvalidFace);

  // If `fontDataI` is none it means it's the build-in none instance. It's an
  // ethernal instance that doesn't have to be released (releasing is NOP) so
  // we don't bother releasing it and just return quickly.
  FontDataImpl* fontDataI = face.impl()->_loader._fontDataImpl(face.faceIndex());
  if (fontDataI->isNone()) {
    return DebugUtils::errored(kErrorFontNotInitialized);
  }

  if (face.implType() >= FontFace::kImplTypeOT_CFFv1) {
    B2D_PROPAGATE(fontDataI->loadSlots(Support::bitMask<uint32_t>(FontData::kSlotCFF)));
  }
  else {
    B2D_PROPAGATE(fontDataI->loadSlots(Support::bitMask<uint32_t>(FontData::kSlotGlyf, FontData::kSlotLoca)));
    if (fontDataI->loadedTableSizeBySlot(FontData::kSlotLoca) < (face.glyphCount() + 1) * size_t(face.impl<OT::OTFaceImpl>()->_locaOffsetSize)) {
      fontDataI->release();
      return DebugUtils::errored(kErrorFontInvalidData);
    }
  }

  FontFaceImpl* faceOldI = _fontFace.impl();
  FontDataImpl* dataOldI = _fontData.impl();

  _fontFace._impl = face.impl()->addRef();
  _fontData._impl = fontDataI;
  _fontMatrix = font.matrix();

  faceOldI->release();
  dataOldI->release();

  return kErrorOk;
}

void GlyphOutlineDecoder::reset() noexcept {
  _fontFace.reset();
  _fontData.reset();
}

// ============================================================================
// [b2d::GlyphOutlineDecoder - Decode]
// ============================================================================

Error GlyphOutlineDecoder::_decodeGlyph(uint32_t glyphId, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept {
  MemBufferTmp<1024> tmpBuffer;
  Matrix2D finalMatrix { _fontMatrix.multiplied(userMatrix) };
  return funcs().decodeGlyph(this, glyphId, finalMatrix, out, tmpBuffer, sink, 0, closure);
}

Error GlyphOutlineDecoder::_decodeGlyphRun(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out, GlyphOutlineSinkFunc sink, void* closure) noexcept {
  Error err = kErrorOk;
  if (glyphRun.empty())
    return err;

  MemBufferTmp<1024> tmpBuffer;
  Matrix2D finalMatrix { _fontMatrix.multiplied(userMatrix) };
  uint32_t flags = glyphRun.flags();

  FontFaceFuncs::DecodeGlyphFunc decodeGlyphImpl = funcs().decodeGlyph;
  GlyphRunIterator it(glyphRun);

  if (it.hasOffsets() && (flags & (GlyphRun::kFlagOffsetsAreAbsolute | GlyphRun::kFlagOffsetsAreAdvances))) {
    Matrix2D offsetMatrix = { 1.0, 0.0, 0.0, 1.0, finalMatrix.m20(), finalMatrix.m21() };

    if (flags & GlyphRun::kFlagUserSpaceOffsets) {
      offsetMatrix.setM00(userMatrix.m00());
      offsetMatrix.setM01(userMatrix.m01());
      offsetMatrix.setM10(userMatrix.m10());
      offsetMatrix.setM11(userMatrix.m11());
    }
    else if (flags & GlyphRun::kFlagDesignSpaceOffsets) {
      offsetMatrix.setM00(finalMatrix.m00());
      offsetMatrix.setM01(finalMatrix.m01());
      offsetMatrix.setM10(finalMatrix.m10());
      offsetMatrix.setM11(finalMatrix.m11());
    }

    if (flags & GlyphRun::kFlagOffsetsAreAbsolute) {
      while (!it.atEnd()) {
        Point offset = it.glyphOffset();
        finalMatrix.setM20(offset.x() * offsetMatrix.m00() + offset.y() * offsetMatrix.m10() + offsetMatrix.m20());
        finalMatrix.setM21(offset.x() * offsetMatrix.m01() + offset.y() * offsetMatrix.m11() + offsetMatrix.m21());

        err = decodeGlyphImpl(this, it.glyphId(), finalMatrix, out, tmpBuffer, sink, it.index(), closure);
        if (B2D_UNLIKELY(err))
          break;
        it.advance();
      }
    }
    else {
      while (!it.atEnd()) {
        err = decodeGlyphImpl(this, it.glyphId(), finalMatrix, out, tmpBuffer, sink, it.index(), closure);
        if (B2D_UNLIKELY(err))
          break;

        Point offset = it.glyphOffset();
        finalMatrix.m[Matrix2D::k20] += offset.x() * offsetMatrix.m00() + offset.y() * offsetMatrix.m10();
        finalMatrix.m[Matrix2D::k21] += offset.x() * offsetMatrix.m01() + offset.y() * offsetMatrix.m11();
        it.advance();
      }
    }
  }
  else {
    while (!it.atEnd()) {
      err = decodeGlyphImpl(this, it.glyphId(), finalMatrix, out, tmpBuffer, sink, it.index(), closure);
      if (B2D_UNLIKELY(err))
        break;
      it.advance();
    }
  }

  return err;
}

B2D_END_NAMESPACE
