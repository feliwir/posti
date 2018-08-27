// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/lookuptable_p.h"
#include "../core/matrix2d.h"
#include "../core/membuffer.h"
#include "../core/path2d.h"
#include "../core/support.h"
#include "../core/wrap.h"
#include "../core/zone.h"
#include "../text/fontface.h"
#include "../text/otface_p.h"
#include "../text/otglyf_p.h"
#include "../text/othead_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

// ============================================================================
// [b2d::OT::GlyfFlag]
// ============================================================================

// This table contains information about the number of bytes vertex data consumes
// per each flag. It's used to calculate the size of X and Y arrays of all contours
// a simple glyph defines. It's used to speedup vertex processing.
template<uint32_t Flags>
struct GlyfFlagPredicateT {
  enum : uint8_t {
    kX = (Flags & GlyfTable::Simple::kXIsByte) ? 1 : (Flags & GlyfTable::Simple::kXIsSameOrXByteIsPositive) ? 0 : 2,
    kY = (Flags & GlyfTable::Simple::kYIsByte) ? 1 : (Flags & GlyfTable::Simple::kYIsSameOrYByteIsPositive) ? 0 : 2
  };
};

static const uint8_t gGlyfFlagToXSizeTable[] = {
  #define VALUE(X) GlyfFlagPredicateT<X>::kX
  B2D_LOOKUP_TABLE_64(VALUE, 0)
  #undef VALUE
};

static const uint8_t gGlyfFlagToYSizeTable[] = {
  #define VALUE(X) GlyfFlagPredicateT<X>::kY
  B2D_LOOKUP_TABLE_64(VALUE, 0)
  #undef VALUE
};

static_assert(sizeof(gGlyfFlagToXSizeTable) == GlyfTable::Simple::kImportantFlagsMask + 1,
              "Invalid table, it must have enough elements to be able to represent all flags.");

static_assert(sizeof(gGlyfFlagToYSizeTable) == GlyfTable::Simple::kImportantFlagsMask + 1,
              "Invalid table, it must have enough elements to be able to represent all flags.");

// ============================================================================
// [b2d::OT::GlyfCompoundEntry]
// ============================================================================

struct GlyfCompoundEntry {
  enum : uint32_t {
    kMaxLevel = 16
  };

  const uint8_t* gPtr;
  size_t remainingSize;
  uint32_t compoundFlags;
  Wrap<Matrix2D> matrix;
};

// ============================================================================
// [b2d::OT::GlyfImpl - Init]
// ============================================================================

Error GlyfImpl::initGlyf(OTFaceImpl* faceI, FontData& fontData) noexcept {
  FontDataTable<HeadTable> head;
  if (B2D_UNLIKELY(!fontData.queryTableByTag(&head, FontTag::head) || !head.fitsTable()))
    return DebugUtils::errored(kErrorFontInvalidData);

  if (head->glyphDataFormat() != 0)
    return DebugUtils::errored(kErrorFontInvalidData);

  switch (head->indexToLocFormat()) {
    case HeadTable::kIndexToLocUInt16:
      faceI->_locaOffsetSize = 2;
      break;

    case HeadTable::kIndexToLocUInt32:
      faceI->_locaOffsetSize = 4;
      break;

    default:
      return DebugUtils::errored(kErrorFontInvalidData);
  }

  faceI->_funcs.getGlyphBoundingBoxes = getGlyphBoundingBoxes;
  faceI->_funcs.decodeGlyph = decodeGlyph;
  return kErrorOk;
}

// ============================================================================
// [b2d::OT::GlyfImpl - GetGlyphBoundingBoxes]
// ============================================================================

Error B2D_CDECL GlyfImpl::getGlyphBoundingBoxes(const GlyphEngine* self, const GlyphId* glyphIds, size_t glyphIdAdvance, IntBox* boxes, size_t count) noexcept {
  typedef GlyfTable::Simple Simple;
  typedef GlyfTable::Compound Compound;

  const OTFaceImpl* faceI = self->fontFace().impl<OTFaceImpl>();
  uint32_t locaOffsetSize = faceI->_locaOffsetSize;

  FontDataRegion locaTable = self->fontData().loadedTableBySlot(FontData::kSlotLoca);
  FontDataRegion glyfTable = self->fontData().loadedTableBySlot(FontData::kSlotGlyf);

  for (size_t i = 0; i < count; i++) {
    uint32_t glyphId = glyphIds[0];
    glyphIds = Support::advancePtr(glyphIds, glyphIdAdvance);

    size_t offset;
    size_t endOff;

    // NOTE: Maximum glyphId is 65535, so we are always safe here regarding
    // multiplying the `glyphId` by 2 or 4 to calculate the correct index.
    if (locaOffsetSize == 2) {
      size_t index = size_t(glyphId) * 2u;
      if (B2D_UNLIKELY(index + 4 > locaTable.size()))
        goto InvalidData;
      offset = uint32_t(reinterpret_cast<const OTUInt16*>(locaTable.data<uint8_t>() + index + 0)->value()) * 2u;
      endOff = uint32_t(reinterpret_cast<const OTUInt16*>(locaTable.data<uint8_t>() + index + 2)->value()) * 2u;
    }
    else {
      size_t index = size_t(glyphId) * 4u;
      if (B2D_UNLIKELY(index + 8 > locaTable.size()))
        goto InvalidData;
      offset = reinterpret_cast<const OTUInt32*>(locaTable.data<uint8_t>() + index + 0)->value();
      endOff = reinterpret_cast<const OTUInt32*>(locaTable.data<uint8_t>() + index + 4)->value();
    }

    if (offset < endOff && endOff <= glyfTable.size()) {
      const uint8_t* gPtr = glyfTable.data<uint8_t>() + offset;
      size_t remainingSize = endOff - offset;

      if (B2D_UNLIKELY(remainingSize < sizeof(GlyfTable::GlyphData)))
        goto InvalidData;

      boxes[i].reset(reinterpret_cast<const GlyfTable::GlyphData*>(gPtr)->xMin(),
                     reinterpret_cast<const GlyfTable::GlyphData*>(gPtr)->yMin(),
                     reinterpret_cast<const GlyfTable::GlyphData*>(gPtr)->xMax(),
                     reinterpret_cast<const GlyfTable::GlyphData*>(gPtr)->yMax());
      continue;
    }

    // Invalid data or the glyph is not defined. In either case we just zero the box.
InvalidData:
    boxes[i].reset();
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::GlyfImpl - DecodeGlyph]
// ============================================================================

Error B2D_CDECL GlyfImpl::decodeGlyph(
  GlyphOutlineDecoder* self,
  uint32_t glyphId,
  const Matrix2D& matrix,
  Path2D& out,
  MemBuffer& tmpBuffer,
  GlyphOutlineSinkFunc sink, size_t sinkGlyphIndex, void* closure) noexcept {

  using OTUtils::readU16;
  using Support::safeSub;

  typedef GlyfTable::Simple Simple;
  typedef GlyfTable::Compound Compound;

  const OTFaceImpl* faceI = self->fontFace().impl<OTFaceImpl>();
  uint32_t locaOffsetSize = faceI->_locaOffsetSize;

  if (B2D_UNLIKELY(glyphId >= faceI->glyphCount()))
    return DebugUtils::errored(kErrorFontInvalidGlyphId);

  FontDataRegion locaTable = self->fontData().loadedTableBySlot(FontData::kSlotLoca);
  FontDataRegion glyfTable = self->fontData().loadedTableBySlot(FontData::kSlotGlyf);

  const uint8_t* gPtr = nullptr;
  size_t remainingSize = 0;
  size_t compoundLevel = 0;

  // Only matrix and compoundFlags are important in the root entry.
  GlyfCompoundEntry compoundData[GlyfCompoundEntry::kMaxLevel];
  compoundData[0].gPtr = nullptr;
  compoundData[0].remainingSize = 0;
  compoundData[0].compoundFlags = Compound::kArgsAreXYValues;
  compoundData[0].matrix.init(matrix);

  GlyphOutlineSinkInfo sinkInfo;
  sinkInfo._glyphIndex = sinkGlyphIndex;
  sinkInfo._contourCount = 0;

  Path2DAppender appender;
  for (;;) {
    size_t offset;
    size_t endOff;

    // NOTE: Maximum glyphId is 65535, so we are always safe here regarding
    // multiplying the `glyphId` by 2 or 4 to calculate the correct index.
    if (locaOffsetSize == 2) {
      size_t index = size_t(glyphId) * 2u;
      if (B2D_UNLIKELY(index + 4 > locaTable.size()))
        goto InvalidData;
      offset = uint32_t(reinterpret_cast<const OTUInt16*>(locaTable.data<uint8_t>() + index + 0)->value()) * 2u;
      endOff = uint32_t(reinterpret_cast<const OTUInt16*>(locaTable.data<uint8_t>() + index + 2)->value()) * 2u;
    }
    else {
      size_t index = size_t(glyphId) * 4u;
      if (B2D_UNLIKELY(index + 8 > locaTable.size()))
        goto InvalidData;
      offset = reinterpret_cast<const OTUInt32*>(locaTable.data<uint8_t>() + index + 0)->value();
      endOff = reinterpret_cast<const OTUInt32*>(locaTable.data<uint8_t>() + index + 4)->value();
    }

    // ------------------------------------------------------------------------
    // [Simple / Empty Glyph]
    // ------------------------------------------------------------------------

    if (B2D_UNLIKELY(offset >= endOff || endOff > glyfTable.size())) {
      // Only ALLOWED when `offset == endOff`.
      if (B2D_UNLIKELY(offset != endOff || endOff > glyfTable.size()))
        goto InvalidData;
    }
    else {
      gPtr = glyfTable.data<uint8_t>() + offset;
      remainingSize = endOff - offset;

      if (B2D_UNLIKELY(remainingSize < sizeof(GlyfTable::GlyphData)))
        goto InvalidData;

      int contourCountSigned = reinterpret_cast<const GlyfTable::GlyphData*>(gPtr)->numberOfContours();
      if (contourCountSigned > 0) {
        uint32_t i;
        uint32_t contourCount = unsigned(contourCountSigned);
        Support::OverflowFlag of;

        // --------------------------------------------------------------------
        // The structure that we are going to read is as follows:
        //
        //   [Header]
        //     uint16_t endPtsOfContours[numberOfContours];
        //
        //   [Hinting Bytecode]
        //     uint16_t instructionLength;
        //     uint8_t instructions[instructionLength];
        //
        //   [Contours]
        //     uint8_t flags[?];
        //     uint8_t/uint16_t xCoordinates[?];
        //     uint8_t/uint16_t yCoordinates[?];
        //
        // The problem with contours data is that it's three arrays next to each
        // other and there is no way to create an iterator as these arrays must be
        // read sequentially. The reader must first read flags, then X coordinates,
        // and then Y coordinates.
        //
        // Minimum data size would be:
        //   10                     [GlyphData header]
        //   (numberOfContours * 2) [endPtsOfContours]
        //   2                      [instructionLength]
        // --------------------------------------------------------------------

        gPtr += sizeof(GlyfTable::GlyphData);
        remainingSize = safeSub(remainingSize, sizeof(GlyfTable::GlyphData) + 2u * contourCount + 2u, &of);
        if (B2D_UNLIKELY(of))
          goto InvalidData;

        const OTUInt16* contourArray = reinterpret_cast<const OTUInt16*>(gPtr);
        gPtr += contourCount * 2u;

        // We don't use hinting instructions, so skip them.
        size_t instructionCount = readU16(gPtr);
        remainingSize = safeSub(remainingSize, instructionCount, &of);
        if (B2D_UNLIKELY(of))
          goto InvalidData;
        gPtr += 2u + instructionCount;

        // --------------------------------------------------------------------
        // We are finally at the beginning of contours data:
        //   flags[]
        //   xCoordinates[]
        //   yCoordinates[]
        // --------------------------------------------------------------------

        // Number of vertices in TrueType sense (could be less than number of
        // points required by Path2D representation, especially if TT outline
        // contains consecutive off-curve points).
        uint32_t vertexCount = size_t(contourArray[contourCount - 1].value()) + 1u;
        uint8_t* flags = static_cast<uint8_t*>(tmpBuffer.alloc(vertexCount));

        if (B2D_UNLIKELY(!flags))
          return DebugUtils::errored(kErrorNoMemory);

        // --------------------------------------------------------------------
        // [Read Flags]
        // --------------------------------------------------------------------

        // Number of bytes required by both X and Y coordinates.
        uint32_t xDataSize = 0;
        uint32_t yDataSize = 0;

        // Number of consecutive off curve vertices making a spline. We need
        // this number to be able to calculate the number of Path2D vertices
        // we will need to convert this glyph into Path2D data.
        uint32_t offCurveSplineCount = 0;

        // We start as off-curve, this would cause adding one more vertex to
        // `offCurveSplineCount` if the start really is off-curve.
        uint32_t prevFlag = 0;

        // Carefully picked bits. It means that this is a second off-curve vertex
        // and it has to be connected by on-curve vertex before the off-curve
        // vertex can be emitted.
        constexpr uint32_t kOffCurveSplineShift = 3;
        constexpr uint32_t kOffCurveSplineBit   = 1 << kOffCurveSplineShift;

        // We parse flags one-by-one and calculate the size required by vertices
        // by using our FLAG tables so we don't have to do bounds checking during
        // vertex decoding.
        const uint8_t* gEnd = gPtr + remainingSize;
        i = 0;

        do {
          if (B2D_UNLIKELY(gPtr == gEnd))
            goto InvalidData;

          uint32_t flag = (*gPtr++ & Simple::kImportantFlagsMask);
          uint32_t offCurveSpline = ((prevFlag | flag) & 1) ^ 1;

          xDataSize += gGlyfFlagToXSizeTable[flag];
          yDataSize += gGlyfFlagToYSizeTable[flag];
          offCurveSplineCount += offCurveSpline;

          if (!(flag & Simple::kRepeatFlag)) {
            flag |= offCurveSpline << kOffCurveSplineShift;
            flags[i++] = uint8_t(flag);
          }
          else {
            // When `kRepeatFlag` is set it means that the next byte contains how
            // many times it should repeat (specification doesn't mention zero
            // length, so we won't fail and just silently consume the byte).
            flag ^= Simple::kRepeatFlag;
            flag |= offCurveSpline << kOffCurveSplineShift;

            if (B2D_UNLIKELY(gPtr == gEnd))
              goto InvalidData;

            uint32_t n = *gPtr++;
            flags[i++] = uint8_t(flag);
            offCurveSpline = (flag & 1) ^ 1;

            if (B2D_UNLIKELY(n > vertexCount - i))
              goto InvalidData;

            xDataSize += n * gGlyfFlagToXSizeTable[flag];
            yDataSize += n * gGlyfFlagToYSizeTable[flag];
            offCurveSplineCount += n * offCurveSpline;

            flag |= offCurveSpline << kOffCurveSplineShift;

            while (n) {
              flags[i++] = uint8_t(flag);
              n--;
            }
          }

          prevFlag = flag;
        } while (i != vertexCount);

        remainingSize = (size_t)(gEnd - gPtr);
        if (B2D_UNLIKELY(xDataSize + yDataSize > remainingSize))
          goto InvalidData;

        // --------------------------------------------------------------------
        // [Read Vertices]
        // --------------------------------------------------------------------

        // Vertex data in `glyf` table doesn't map 1:1 to how Path2D stores
        // contours. Multiple off-point curves in TT data are decomposed into
        // a quad spline, which is one vertex larger (Path2D doesn't offer
        // multiple off-point quads). This means that the number of vertices
        // required by Path2D can be greater than the number of vertices stored
        // in TT 'glyf' data. However, we should know exactly how many vertices
        // we have to add to `vertexCount` as we calculated `offCurveSplineCount`
        // during flags decoding.
        //
        // The number of resulting vertices is thus:
        //   - `vertexCount` - base number of vertices stored in TT data.
        //   - `offCurveSplineCount` - the number of additional vertices we
        //     will need to add for each off-curve spline used in TT data.
        //   - `contourCount` - Number of contours, we multiply this by 3
        //     as we want to include one 'MoveTo', 'Close', and one additional
        //     off-curve spline point per each contour in case it starts
        //     ends with off-curve point.
        size_t pathVertexCount = vertexCount + offCurveSplineCount + contourCount * 3;
        B2D_PROPAGATE(appender.startAppend(out, pathVertexCount, Path2DInfo::kAllFlags));

        // Since we know exactly how many bytes both vertex arrays consume we
        // can decode both X and Y coordinates at the same time. This gives us
        // also the opportunity to start appending to Path2D immediately.
        const uint8_t* yPtr = gPtr + xDataSize;

        // Affine transform applied to each vertex.
        double m00 = compoundData[compoundLevel].matrix->m00();
        double m01 = compoundData[compoundLevel].matrix->m01();
        double m10 = compoundData[compoundLevel].matrix->m10();
        double m11 = compoundData[compoundLevel].matrix->m11();

        // Vertices are stored relative to each other, this is the current point.
        double px = compoundData[compoundLevel].matrix->m20();
        double py = compoundData[compoundLevel].matrix->m21();

        // Current vertex index in TT sense, advanced until `vertexCount`,
        // which must be end index of the last contour.
        i = 0;

        for (uint32_t contourIndex = 0; contourIndex < contourCount; contourIndex++) {
          uint32_t iEnd = uint32_t(contourArray[contourIndex].value()) + 1;
          if (B2D_UNLIKELY(iEnd <= i || iEnd > vertexCount))
            goto InvalidData;

          // We do the first vertex here as we want to emit 'MoveTo' and we
          // want to remember it for a possible off-curve start. Currently
          // this means there is some code duplicated, unfortunately...
          uint32_t flag = flags[i];

          {
            double xOff = 0.0;
            double yOff = 0.0;

            if (flag & Simple::kXIsByte) {
              B2D_ASSERT(gPtr <= gEnd - 1);
              xOff = double(gPtr[0]);
              if (!(flag & Simple::kXIsSameOrXByteIsPositive))
                xOff = -xOff;
              gPtr += 1;
            }
            else if (!(flag & Simple::kXIsSameOrXByteIsPositive)) {
              B2D_ASSERT(gPtr <= gEnd - 2);
              xOff = double(OTUtils::readI16(gPtr));
              gPtr += 2;
            }

            if (flag & Simple::kYIsByte) {
              B2D_ASSERT(yPtr <= gEnd - 1);
              yOff = double(yPtr[0]);
              if (!(flag & Simple::kYIsSameOrYByteIsPositive))
                yOff = -yOff;
              yPtr += 1;
            }
            else if (!(flag & Simple::kYIsSameOrYByteIsPositive)) {
              B2D_ASSERT(yPtr <= gEnd - 2);
              yOff = double(OTUtils::readI16(yPtr));
              yPtr += 2;
            }

            px += xOff * m00 + yOff * m10;
            py += xOff * m01 + yOff * m11;
          }

          if (++i >= iEnd)
            continue;

          // Initial 'MoveTo' coordinates.
          double mx = px;
          double my = py;

          // We need to be able to handle a stupic case when the countour starts off curve.
          uint8_t cmd = Path2D::kCmdLineTo;
          Point* offCurveStart = flag & Simple::kOnCurvePoint ? nullptr : appender.vertexPtr();

          if (offCurveStart) {
            cmd = Path2D::kCmdMoveTo;
          }
          else {
            appender.appendMoveTo(mx, my);
          }

          for (;;) {
            flag = flags[i];
            double dx = 0.0;
            double dy = 0.0;

            if (flag & Simple::kXIsByte) {
              B2D_ASSERT(gPtr <= gEnd - 1);
              double xOff = double(gPtr[0]);
              if (!(flag & Simple::kXIsSameOrXByteIsPositive))
                xOff = -xOff;
              gPtr += 1;
              dx = xOff * m00;
              dy = xOff * m01;
            }
            else if (!(flag & Simple::kXIsSameOrXByteIsPositive)) {
              B2D_ASSERT(gPtr <= gEnd - 2);
              double xOff = double(OTUtils::readI16(gPtr));
              gPtr += 2;
              dx = xOff * m00;
              dy = xOff * m01;
            }

            if (flag & Simple::kYIsByte) {
              B2D_ASSERT(yPtr <= gEnd - 1);
              double yOff = double(yPtr[0]);
              if (!(flag & Simple::kYIsSameOrYByteIsPositive))
                yOff = -yOff;
              yPtr += 1;
              dx += yOff * m10;
              dy += yOff * m11;
            }
            else if (!(flag & Simple::kYIsSameOrYByteIsPositive)) {
              B2D_ASSERT(yPtr <= gEnd - 2);
              double yOff = double(OTUtils::readI16(yPtr));
              yPtr += 2;
              dx += yOff * m10;
              dy += yOff * m11;
            }

            px += dx;
            py += dy;

            if (flag & Simple::kOnCurvePoint) {
              appender.appendVertex(cmd, px, py);
              cmd = Path2D::kCmdLineTo;
            }
            else if (flag & kOffCurveSplineBit) {
              double qx = px - dx * 0.5;
              double qy = py - dy * 0.5;

              appender.appendVertex(cmd               , qx, qy);
              appender.appendVertex(Path2D::kCmdQuadTo, px, py);
              cmd = Path2D::kCmdControl;
            }
            else {
              appender.appendVertex(Path2D::kCmdQuadTo, px, py);
              cmd = Path2D::kCmdControl;
            }

            if (++i >= iEnd)
              break;
          }

          if (cmd == Path2D::kCmdControl) {
            if (offCurveStart) {
              appender.appendVertex(Path2D::kCmdControl, (px + mx) * 0.5, (py + my) * 0.5);
              appender.appendVertex(Path2D::kCmdQuadTo , mx, my);
              appender.appendVertex(Path2D::kCmdControl, (mx + offCurveStart->x()) * 0.5, (my + offCurveStart->y()) * 0.5);
            }
            else {
              appender.appendVertex(Path2D::kCmdControl, mx, my);
            }
          }
          else {
            if (offCurveStart) {
              appender.appendVertex(Path2D::kCmdQuadTo , mx, my);
              appender.appendVertex(Path2D::kCmdControl, offCurveStart->x(), offCurveStart->y());
            }
          }

          appender.appendClose();
        }

        appender.end(out);
        if (sink) {
          sinkInfo._contourCount = contourCount;
          B2D_PROPAGATE(sink(out, sinkInfo, closure));
        }
      }
      else if (contourCountSigned == -1) {
        gPtr += sizeof(GlyfTable::GlyphData);
        remainingSize -= sizeof(GlyfTable::GlyphData);

        if (B2D_UNLIKELY(++compoundLevel >= GlyfCompoundEntry::kMaxLevel))
          goto InvalidData;

        goto ContinueCompound;
      }
      else {
        // Cannot be less than -1, only -1 specifies compound glyph, lesser value
        // is invalid according to the specification.
        if (B2D_UNLIKELY(contourCountSigned < -1))
          goto InvalidData;

        // Otherwise the glyph has no contours.
      }
    }

    // ----------------------------------------------------------------------
    // [Compound Glyph]
    // ----------------------------------------------------------------------

    if (compoundLevel) {
      while (!(compoundData[compoundLevel].compoundFlags & Compound::kMoreComponents))
        if (--compoundLevel == 0)
          break;

      if (compoundLevel) {
        gPtr = compoundData[compoundLevel].gPtr;
        remainingSize = compoundData[compoundLevel].remainingSize;

        // --------------------------------------------------------------------
        // The structure that we are going to read is as follows:
        //
        //   [Header]
        //     uint16_t flags;
        //     uint16_t glyphId;
        //
        //   [Translation]
        //     a) int8_t arg1/arg2;
        //     b) int16_t arg1/arg2;
        //
        //   [Scale/Affine]
        //     a) <None>
        //     b) int16_t scale;
        //     c) int16_t scaleX, scaleY;
        //     d) int16_t m00, m01, m10, m11;
        // --------------------------------------------------------------------

ContinueCompound:
        {
          uint32_t flags;
          int arg1, arg2;
          Support::OverflowFlag of;

          remainingSize = Support::safeSub<size_t>(remainingSize, 6, &of);
          if (B2D_UNLIKELY(of))
            goto InvalidData;

          flags = OTUtils::readU16(gPtr);
          glyphId = OTUtils::readU16(gPtr + 2);
          if (B2D_UNLIKELY(glyphId >= faceI->glyphCount()))
            goto InvalidData;

          arg1 = OTUtils::readI8(gPtr + 4);
          arg2 = OTUtils::readI8(gPtr + 5);
          gPtr += 6;

          if (flags & Compound::kArgsAreWords) {
            remainingSize = Support::safeSub<size_t>(remainingSize, 2, &of);
            if (B2D_UNLIKELY(of))
              goto InvalidData;

            arg1 = Support::shl(arg1, 8) | (arg2 & 0xFF);
            arg2 = OTUtils::readI16(gPtr);
            gPtr += 2;
          }

          if (!(flags & Compound::kArgsAreXYValues)) {
            // This makes them unsigned.
            arg1 &= 0xFFFFu;
            arg2 &= 0xFFFFu;

            // TODO: Not implemented. I don't know how atm.
          }

          constexpr double kScaleF2x14 = 1.0 / 16384.0;

          Matrix2D& matrix = compoundData[compoundLevel].matrix();
          matrix.reset(1.0, 0.0, 0.0, 1.0, double(arg1), double(arg2));

          if (flags & Compound::kAnyCompoundScale) {
            if (flags & Compound::kWeHaveScale) {
              // Simple scaling:
              //   [Sc, 0]
              //   [0, Sc]
              remainingSize = Support::safeSub<size_t>(remainingSize, 2, &of);
              if (B2D_UNLIKELY(of))
                goto InvalidData;

              double scale = double(OTUtils::readI16(gPtr)) * kScaleF2x14;
              matrix.setM00(scale);
              matrix.setM11(scale);
              gPtr += 2;
            }
            else if (flags & Compound::kWeHaveScaleXY) {
              // Simple scaling:
              //   [Sx, 0]
              //   [0, Sy]
              remainingSize = Support::safeSub<size_t>(remainingSize, 4, &of);
              if (B2D_UNLIKELY(of))
                goto InvalidData;

              matrix.setM00(double(OTUtils::readI16(gPtr + 0)) * kScaleF2x14);
              matrix.setM11(double(OTUtils::readI16(gPtr + 2)) * kScaleF2x14);
              gPtr += 4;
            }
            else {
              // Affine case:
              //   [A, B]
              //   [C, D]
              remainingSize = Support::safeSub<size_t>(remainingSize, 8, &of);
              if (B2D_UNLIKELY(of))
                goto InvalidData;

              matrix.setM00(double(OTUtils::readI16(gPtr + 0)) * kScaleF2x14);
              matrix.setM01(double(OTUtils::readI16(gPtr + 2)) * kScaleF2x14);
              matrix.setM10(double(OTUtils::readI16(gPtr + 4)) * kScaleF2x14);
              matrix.setM11(double(OTUtils::readI16(gPtr + 6)) * kScaleF2x14);
              gPtr += 8;
            }

            // Translation scale should only happen when `kArgsAreXYValues` is set. The
            // default behavior according to the specification is `kUnscaledComponentOffset`,
            // which can be overridden by setting `kScaledComponentOffset`. However, if both
            // or neither are set then the behavior is the same as `kUnscaledComponentOffset`.
            if ((flags & (Compound::kArgsAreXYValues | Compound::kAnyCompoundOffset    )) ==
                         (Compound::kArgsAreXYValues | Compound::kScaledComponentOffset)) {
              // This is what FreeType does and what's not 100% according to the specificaion.
              // However, according to FreeType this would produce much better offsets so we
              // will match FreeType instead of following the specification.
              matrix.m[Matrix2D::k20] *= Math::length(matrix.m00(), matrix.m01());
              matrix.m[Matrix2D::k21] *= Math::length(matrix.m10(), matrix.m11());
            }
          }

          compoundData[compoundLevel].gPtr = gPtr;
          compoundData[compoundLevel].remainingSize = remainingSize;
          compoundData[compoundLevel].compoundFlags = flags;
          matrix.multiply(compoundData[compoundLevel - 1].matrix());
          continue;
        }
      }
    }

    break;
  }

  return kErrorOk;

InvalidData:
  return DebugUtils::errored(kErrorFontInvalidData);
}

B2D_END_SUB_NAMESPACE
