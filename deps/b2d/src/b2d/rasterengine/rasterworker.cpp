// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../rasterengine/rastercontext2d_p.h"
#include "../rasterengine/rasterworker_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

// ============================================================================
// [b2d::RasterWorker - Construction / Destruction]
// ============================================================================

RasterWorker::RasterWorker(RasterContext2DImpl* context) noexcept
  : _context(context),
    _ctxData(),
    _workerZone(65536 - Zone::kBlockOverhead, 8),
    _zeroMem(),
    _edgeStorage() {

  _clipMode = Context2D::kClipModeRect;
  _reserved[0] = 0;
  _reserved[1] = 0;
  _reserved[2] = 0;

  _edgeStorage.setBandHeight(32);

  _fullAlpha = 0x100;
  _buffer.reset();
}

RasterWorker::~RasterWorker() noexcept {
  if (_edgeStorage.bandEdges())
    ZeroAllocator::release(_edgeStorage.bandEdges(), _edgeStorage.bandCapacity() * sizeof(void*));
}

// ============================================================================
// [b2d::RasterWorker - Interface]
// ============================================================================

Error RasterWorker::ensureEdgeStorage() noexcept {
  uint32_t targetHeight = _ctxData.dst.height;
  uint32_t bandHeight = _edgeStorage.bandHeight();
  uint32_t bandCount = (targetHeight + bandHeight - 1) >> Support::ctz(bandHeight);

  if (bandCount <= _edgeStorage.bandCapacity())
    return kErrorOk;

  size_t allocatedSize = 0;
  _edgeStorage._bandEdges = static_cast<EdgeVector<int>**>(
    ZeroAllocator::realloc(
      _edgeStorage.bandEdges(),
      _edgeStorage.bandCapacity() * sizeof(void*),
      bandCount * sizeof(void*),
      allocatedSize));

  uint32_t bandCapacity = uint32_t(allocatedSize / sizeof(void*));
  _edgeStorage._bandCount = Math::min(bandCount, bandCapacity);
  _edgeStorage._bandCapacity = bandCapacity;

  return _edgeStorage._bandEdges ? kErrorOk : kErrorNoMemory;
}

B2D_END_SUB_NAMESPACE
