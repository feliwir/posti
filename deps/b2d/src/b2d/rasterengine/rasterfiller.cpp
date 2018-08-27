// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../rasterengine/analyticrasterizer_p.h"
#include "../rasterengine/rasterfiller_p.h"
#include "../rasterengine/rasterworker_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

Error B2D_CDECL RasterFiller::fillRectImpl(RasterFiller* ctx, RasterWorker* worker, const RasterFetchData* fetchData) noexcept {
  return ctx->_fillFunc(&worker->_ctxData, &ctx->_fillData, fetchData);
}

struct ActiveEdge {
  AnalyticRasterizerState _state;  // Analytic rasterizer state.
  uint32_t _signBit;               // Sign bit, for making cover/area negative.
  FixedPoint<int>* _cur;           // Start of point data (advanced during rasterization).
  FixedPoint<int>* _end;           // End of point data.
  ActiveEdge* _next;               // Next active edge (single-linked list).
};

// TODO: REMOVE
static size_t calcLines(EdgeStorage<int>* edgeStorage) noexcept {
  EdgeVector<int>** edges = edgeStorage->bandEdges();
  size_t count = edgeStorage->bandCount();

  size_t n = 0;

  for (size_t bandId = 0; bandId < count; bandId++) {
    EdgeVector<int>* edge = edges[bandId];
    while (edge) {
      n += edge->count - 1;
      edge = edge->next;
    }
  }

  return n;
}

static void debugEdges(EdgeStorage<int>* edgeStorage) noexcept {
  EdgeVector<int>** edges = edgeStorage->bandEdges();
  size_t count = edgeStorage->bandCount();
  uint32_t bandHeight = edgeStorage->bandHeight();

  int minX = std::numeric_limits<int>::max();
  int minY = std::numeric_limits<int>::max();
  int maxX = std::numeric_limits<int>::min();
  int maxY = std::numeric_limits<int>::min();

  const IntBox& bb = edgeStorage->boundingBox();
  printf("EDGE STORAGE [%d.%d %d.%d %d.%d %d.%d]:\n",
         bb.x0() >> 8, bb.x0() & 0xFF,
         bb.y0() >> 8, bb.y0() & 0xFF,
         bb.x1() >> 8, bb.x1() & 0xFF,
         bb.y1() >> 8, bb.y1() & 0xFF);

  for (size_t bandId = 0; bandId < count; bandId++) {
    EdgeVector<int>* edge = edges[bandId];
    if (edge) {
      printf("BAND #%d y={%d:%d}\n", int(bandId), int(bandId * bandHeight), int((bandId + 1) * bandHeight - 1));
      while (edge) {
        printf("  EDGES {sign=%u count=%u}", unsigned(edge->signBit), unsigned(edge->count));

        if (edge->count <= 1)
          printf("{WRONG COUNT!}");

        FixedPoint<int>* ptr = edge->pts;
        FixedPoint<int>* ptrStart = ptr;
        FixedPoint<int>* ptrEnd = edge->pts + edge->count;

        while (ptr != ptrEnd) {
          minX = Math::min(minX, ptr[0].x());
          minY = Math::min(minY, ptr[0].y());
          maxX = Math::max(maxX, ptr[0].x());
          maxY = Math::max(maxY, ptr[0].y());

          printf(" [%d.%d, %d.%d]", ptr[0].x() >> 8, ptr[0].x() & 0xFF, ptr[0].y() >> 8, ptr[0].y() & 0xFF);

          if (ptr != ptrStart && ptr[-1].y() > ptr[0].y())
            printf(" !INVALID! ");

          ptr++;
        }

        printf("\n");
        edge = edge->next;
      }
    }
  }

  printf("EDGE STORAGE BBOX [%d.%d, %d.%d] -> [%d.%d, %d.%d]\n\n",
    minX >> 8, minX & 0xFF,
    minY >> 8, minY & 0xFF,
    maxX >> 8, maxX & 0xFF,
    maxY >> 8, maxY & 0xFF);
}

Error B2D_CDECL RasterFiller::fillAnalyticImpl(RasterFiller* ctx, RasterWorker* worker, const RasterFetchData* fetchData) noexcept {
  typedef Support::BitWord BitWord;

  // Can only be called if there is something to fill.
  EdgeStorage<int>* edgeStorage = ctx->_edgeStorage;

  // TODO: REMOVE.
  // debugEdges(edgeStorage);
  // printf("NUM LINES: %u\n", unsigned(calcLines(edgeStorage)));

  // NOTE: This won't happen often, but it's possible. If, for any reason, the
  // data in bands is all h-lines or no data at all we would trigger this condition.
  if (B2D_UNLIKELY(edgeStorage->boundingBox().y0() >= edgeStorage->boundingBox().y1()))
    return kErrorOk;

  uint32_t bandHeight = edgeStorage->bandHeight();
  uint32_t bandHeightMask = bandHeight - 1;

  const uint32_t yStart = (uint32_t(edgeStorage->boundingBox().y0())          ) >> kA8Shift;
  const uint32_t yEnd   = (uint32_t(edgeStorage->boundingBox().y1()) + kA8Mask) >> kA8Shift;

  size_t requiredWidth = Support::alignUp(worker->_buffer.width() + 1 + kPixelsPerOneBit, kPixelsPerOneBit);
  size_t requiredHeight = bandHeight;
  size_t cellAlignment = 16;

  size_t bitStride = Support::bitWordCountFromBitCount(requiredWidth / kPixelsPerOneBit) * sizeof(BitWord);
  size_t cellStride = requiredWidth * sizeof(Pipe2D::FillData::Cell);

  size_t bitsStart = 0;
  size_t bitsSize = requiredHeight * bitStride;

  size_t cellsStart = Support::alignUp(bitsStart + bitsSize, cellAlignment);
  size_t cellsSize = requiredHeight * cellStride;

  B2D_PROPAGATE(worker->_zeroMem.ensure(cellsStart + cellsSize));

  AnalyticCellStorage cellStorage;
  cellStorage.init(reinterpret_cast<BitWord*>(worker->_zeroMem.ptr() + bitsStart), bitStride,
                   Support::alignUp(reinterpret_cast<uint32_t*>(worker->_zeroMem.ptr() + cellsStart), cellAlignment), cellStride);

  ActiveEdge* active = nullptr;
  ActiveEdge* pooled = nullptr;

  EdgeVector<int>** bandEdges = edgeStorage->_bandEdges;
  uint32_t fixedBandHeightShift = edgeStorage->fixedBandHeightShift();

  uint32_t bandId = unsigned(edgeStorage->boundingBox().y0()) >> fixedBandHeightShift;
  uint32_t bandLast = unsigned(edgeStorage->boundingBox().y1() - 1) >> fixedBandHeightShift;

  ctx->_fillData.analytic.box._x0 = 0;
  ctx->_fillData.analytic.box._x1 = int(worker->_buffer.width());
  ctx->_fillData.analytic.box._y0 = 0; // Overwritten before calling `fillFunc`.
  ctx->_fillData.analytic.box._y1 = 0; // Overwritten before calling `fillFunc`.

  ctx->_fillData.analytic.bitTopPtr = cellStorage.bitPtrTop();
  ctx->_fillData.analytic.bitStride = cellStorage.bitStride();
  ctx->_fillData.analytic.cellTopPtr = cellStorage.cellPtrTop();
  ctx->_fillData.analytic.cellStride = cellStorage.cellStride();

  AnalyticRasterizer ras;
  ras.init(cellStorage.bitPtrTop(), cellStorage.bitStride(), cellStorage.cellPtrTop(), cellStorage.cellStride(), bandId * bandHeight, bandHeight);
  ras._bandOffset = yStart;

  Zone* workerZone = &worker->_workerZone;
  do {
    EdgeVector<int>* edges = bandEdges[bandId];
    bandEdges[bandId] = nullptr;

    ActiveEdge** pPrev = &active;
    ActiveEdge* current = *pPrev;

    ras._bandEnd = Math::min((bandId + 1) * bandHeight, yEnd) - 1;

    while (current) {
      ras.restore(current->_state);
      ras.setSignMaskFromBit(current->_signBit);

      for (;;) {
Rasterize:
        if (ras.template rasterize<AnalyticRasterizer::kOptionBandOffset | AnalyticRasterizer::kOptionBandingMode>()) {
          // The edge is fully rasterized.
          FixedPoint<int>* pts = current->_cur;
          while (pts != current->_end) {
            pts++;
            if (!ras.prepare(pts[-2].x(), pts[-2].y(), pts[-1].x(), pts[-1].y()))
              continue;

            current->_cur = pts;
            if (uint32_t(ras._ey0) <= ras._bandEnd)
              goto Rasterize;
            else
              goto SaveState;
          }

          ActiveEdge* old = current;
          current = current->_next;

          old->_next = pooled;
          pooled = old;
          break;
        }

SaveState:
        // The edge is not fully rasterized and crosses the next band.
        ras.save(current->_state);

        *pPrev = current;
        pPrev = &current->_next;
        current = *pPrev;
        break;
      }
    }

    if (edges) {
      if (!pooled) {
        pooled = static_cast<ActiveEdge*>(workerZone->alloc(sizeof(ActiveEdge)));
        if (B2D_UNLIKELY(!pooled))
          return DebugUtils::errored(kErrorNoMemory);
        pooled->_next = nullptr;
      }

      do {
        FixedPoint<int>* pts = edges->pts + 1;
        FixedPoint<int>* end = edges->pts + edges->count;

        uint32_t signBit = edges->signBit;
        ras.setSignMaskFromBit(signBit);

        edges = edges->next;
        do {
          pts++;
          if (!ras.prepare(pts[-2].x(), pts[-2].y(), pts[-1].x(), pts[-1].y()))
            continue;

          if (uint32_t(ras._ey1) <= ras._bandEnd) {
            ras.template rasterize<AnalyticRasterizer::kOptionBandOffset>();
          }
          else {
            current = pooled;
            pooled = current->_next;

            current->_signBit = signBit;
            current->_cur = pts;
            current->_end = end;
            current->_next = nullptr;

            if (uint32_t(ras._ey0) <= ras._bandEnd)
              goto Rasterize;
            else
              goto SaveState;
          }
        } while (pts != end);
      } while (edges);
    }

    // Makes `active` or the last `ActiveEdge->_next` null. It's important,
    // because we don't unlink during edge pooling as we can do it here.
    *pPrev = nullptr;

    ctx->_fillData.analytic.box._y0 = int(ras._bandOffset);
    ctx->_fillData.analytic.box._y1 = int(ras._bandEnd) + 1;
    ctx->_fillFunc(&worker->_ctxData, &ctx->_fillData, fetchData);

    ras._bandOffset = (ras._bandOffset + bandHeight) & ~bandHeightMask;
  } while (++bandId <= bandLast);

  workerZone->reset();
  return kErrorOk;
}

B2D_END_SUB_NAMESPACE
