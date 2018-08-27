// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_RASTERENGINE_EDGEBUILDER_P_H
#define _B2D_RASTERENGINE_EDGEBUILDER_P_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/math_roots.h"
#include "../core/matrix2d.h"
#include "../core/path2d.h"
#include "../core/support.h"
#include "../core/zone.h"
#include "../rasterengine/edgeflatten_p.h"
#include "../rasterengine/edgestorage_p.h"

B2D_BEGIN_SUB_NAMESPACE(RasterEngine)

//! \internal
//!
//! \addtogroup b2d_rasterengine
//! \{

// ============================================================================
// [b2d::RasterEngine::Clip]
// ============================================================================

namespace Clip {
  enum Shift : uint32_t {
    kShiftX0      = 0,
    kShiftY0      = 1,
    kShiftX1      = 2,
    kShiftY1      = 3
  };

  enum Flags : uint32_t {
    kNoFlags      = 0u,
    kFlagX0       = 1u << kShiftX0,
    kFlagY0       = 1u << kShiftY0,
    kFlagX1       = 1u << kShiftX1,
    kFlagY1       = 1u << kShiftY1,

    kFlagX0X1     = kFlagX0 | kFlagX1,
    kFlagY0Y1     = kFlagY0 | kFlagY1,

    kFlagX0Y0     = kFlagX0 | kFlagY0,
    kFlagX1Y0     = kFlagX1 | kFlagY0,

    kFlagX0Y1     = kFlagX0 | kFlagY1,
    kFlagX1Y1     = kFlagX1 | kFlagY1
  };

  static B2D_INLINE uint32_t calcX0Flags(const Point& pt, const Box& box) noexcept { return (uint32_t(box.x0() > pt.x()) << kShiftX0); }
  static B2D_INLINE uint32_t calcX1Flags(const Point& pt, const Box& box) noexcept { return (uint32_t(box.x1() < pt.x()) << kShiftX1); }
  static B2D_INLINE uint32_t calcY0Flags(const Point& pt, const Box& box) noexcept { return (uint32_t(box.y0() > pt.y()) << kShiftY0); }
  static B2D_INLINE uint32_t calcY1Flags(const Point& pt, const Box& box) noexcept { return (uint32_t(box.y1() < pt.y()) << kShiftY1); }

  static B2D_INLINE uint32_t calcXFlags(const Point& pt, const Box& box) noexcept { return calcX0Flags(pt, box) | calcX1Flags(pt, box); }
  static B2D_INLINE uint32_t calcYFlags(const Point& pt, const Box& box) noexcept { return calcY0Flags(pt, box) | calcY1Flags(pt, box); }
  static B2D_INLINE uint32_t calcXYFlags(const Point& pt, const Box& box) noexcept { return calcXFlags(pt, box) | calcYFlags(pt, box); }

  static B2D_INLINE void clampPoint(Point& p, const Box& clipBox) noexcept {
    p._x = Math::bound(p.x(), clipBox.x0(), clipBox.x1());
    p._y = Math::bound(p.y(), clipBox.y0(), clipBox.y1());
  }
}

// ============================================================================
// [b2d::RasterEngine::EdgeBuilderFlags]
// ============================================================================

namespace EdgeBuilderFlags {
  enum FlushFlags : uint32_t {
    kFlushNothing            = 0x00000000u,
    kFlushBoundingBox        = 0x00000001u,
    kFlushBorderAccumulators = 0x00000002u,
    kFlushAll                = 0x00000003u
  };
};

// ============================================================================
// [b2d::RasterEngine::EdgeBuilderBase]
// ============================================================================

template<typename CoordT>
class EdgeBuilderBase {
public:
  static constexpr uint32_t kEdgeOffset = uint32_t(sizeof(EdgeVector<CoordT>) - sizeof(FixedPoint<CoordT>));
  static constexpr uint32_t kMinEdgeSize = uint32_t(sizeof(EdgeVector<CoordT>) + sizeof(FixedPoint<CoordT>));

  B2D_INLINE EdgeBuilderBase(Zone* zone, EdgeVector<CoordT>** bands, uint32_t fixedBandHeightShift) noexcept
    : _zone(zone),
      _bands(bands),
      _fixedBandHeightShift(fixedBandHeightShift),
      _ptr(nullptr),
      _end(nullptr) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { _ptr = nullptr; }

  // --------------------------------------------------------------------------
  // [Basics]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasSpace() const noexcept { return _ptr != _end; }

  // --------------------------------------------------------------------------
  // [Ascending Edge]
  // --------------------------------------------------------------------------

  B2D_INLINE Error ascendingOpen() noexcept {
    B2D_PROPAGATE(_zone->ensure(kMinEdgeSize));
    _ptr = _zone->end<FixedPoint<CoordT>>();
    _end = _zone->ptr<EdgeVector<CoordT>>()->pts;
    return kErrorOk;
  }

  B2D_INLINE void ascendingAddUnsafe(CoordT x, CoordT y) noexcept {
    B2D_ASSERT(hasSpace());
    _ptr--;
    _ptr->set(x, y);
  }

  B2D_INLINE Error ascendingAddChecked(CoordT x, CoordT y, uint32_t signBit = 1) noexcept {
    if (B2D_UNLIKELY(!hasSpace())) {
      const FixedPoint<CoordT>* last = ascendingLast();
      ascendingClose(signBit);
      B2D_PROPAGATE(ascendingOpen());
      _ptr--;
      _ptr->set(last->x(), last->y());
    }

    _ptr--;
    _ptr->set(x, y);
    return kErrorOk;
  }

  B2D_INLINE void ascendingClose(uint32_t signBit = 1) noexcept {
    EdgeVector<CoordT>* edge = reinterpret_cast<EdgeVector<CoordT>*>(reinterpret_cast<uint8_t*>(_ptr) - kEdgeOffset);
    edge->signBit = signBit;
    edge->count = (size_t)(_zone->end<FixedPoint<CoordT>>() - _ptr);

    _zone->setEnd(edge);
    _linkEdge(edge, _ptr[0].y());
  }

  B2D_INLINE FixedPoint<CoordT>* ascendingLast() const noexcept { return _ptr; }

  // --------------------------------------------------------------------------
  // [Descending Edge]
  // --------------------------------------------------------------------------

  B2D_INLINE Error descendingOpen() noexcept {
    B2D_PROPAGATE(_zone->ensure(kMinEdgeSize));
    _ptr = _zone->ptr<EdgeVector<CoordT>>()->pts;
    _end = _zone->end<FixedPoint<CoordT>>();
    return kErrorOk;
  }

  B2D_INLINE void descendingAddUnsafe(CoordT x, CoordT y) noexcept {
    B2D_ASSERT(hasSpace());
    _ptr->set(x, y);
    _ptr++;
  }

  B2D_INLINE Error descendingAddChecked(CoordT x, CoordT y, uint32_t signBit = 0) noexcept {
    B2D_ASSERT(_zone->ptr<EdgeVector<CoordT>>()->pts == _ptr || _ptr[-1].y() <= y);

    if (B2D_UNLIKELY(!hasSpace())) {
      const FixedPoint<CoordT>* last = descendingLast();
      descendingClose(signBit);
      B2D_PROPAGATE(descendingOpen());
      _ptr->set(last->x(), last->y());
      _ptr++;
    }

    _ptr->set(x, y);
    _ptr++;
    return kErrorOk;
  }

  B2D_INLINE void descendingClose(uint32_t signBit = 0) noexcept {
    EdgeVector<CoordT>* edge = _zone->ptr<EdgeVector<CoordT>>();
    edge->signBit = signBit;
    edge->count = (size_t)(_ptr - edge->pts);

    _zone->setPtr(_ptr);
    _linkEdge(edge, edge->pts[0].y());
  }

  B2D_INLINE void descendingCancel() noexcept {
    // Nothing needed here...
  }

  B2D_INLINE FixedPoint<CoordT>* descendingFirst() const noexcept { return _zone->ptr<EdgeVector<CoordT>>()->pts; };
  B2D_INLINE FixedPoint<CoordT>* descendingLast() const noexcept { return _ptr - 1; }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  B2D_INLINE Error addClosedLine(CoordT x0, CoordT y0, CoordT x1, CoordT y1, uint32_t signBit) noexcept {
    // Must be correct, the rasterizer won't check this.
    B2D_ASSERT(y0 < y1);

    EdgeVector<CoordT>* edge = static_cast<EdgeVector<CoordT>*>(_zone->alloc(kMinEdgeSize));
    if (B2D_UNLIKELY(!edge))
      return DebugUtils::errored(kErrorNoMemory);

    edge->pts[0].set(x0, y0);
    edge->pts[1].set(x1, y1);
    edge->signBit = signBit;
    edge->count = 2;

    _linkEdge(edge, y0);
    return kErrorOk;
  }

  B2D_INLINE void _linkEdge(EdgeVector<CoordT>* edge, int y0) noexcept {
    size_t bandId = size_t(unsigned(y0) >> _fixedBandHeightShift);
    edge->next = _bands[bandId];
    _bands[bandId] = edge;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone* _zone;                           //!< Zone memory used to allocate EdgeVector[].
  EdgeVector<CoordT>** _bands;           //!< Bands.
  uint32_t _fixedBandHeightShift;        //!< Shift to get bandId from fixed coordinate.
  FixedPoint<CoordT>* _ptr;              //!< Current point in edge-vector.
  FixedPoint<CoordT>* _end;              //!< Last point the builder can go.
};

// ============================================================================
// [b2d::RasterEngine::EdgeBuilderFromSource]
// ============================================================================

template<class SourceT>
class EdgeBuilderFromSource {
public:
  struct Appender {
    B2D_INLINE Appender(EdgeBuilderFromSource& ctx, uint32_t signBit = 0) noexcept
      : _ctx(ctx),
        _signBit(signBit) {}

    B2D_INLINE uint32_t signBit() noexcept { return _signBit; }
    B2D_INLINE void setSignBit(uint32_t signBit) noexcept { _signBit = signBit; }

    B2D_INLINE Error openAt(double x, double y) noexcept {
      int fx = Math::itrunc(x);
      int fy = Math::itrunc(y);

      B2D_PROPAGATE(_ctx._builder.descendingOpen());
      _ctx._builder.descendingAddUnsafe(fx, fy);

      return kErrorOk;
    }

    B2D_INLINE Error addLine(double x, double y) noexcept {
      int fx = Math::itrunc(x);
      int fy = Math::itrunc(y);

      return _ctx._builder.descendingAddChecked(fx, fy, _signBit);
    }

    B2D_INLINE Error close() noexcept {
      int fy0 = _ctx._builder.descendingFirst()->y();
      int fy1 = _ctx._builder.descendingLast()->y();

      // Rare but happens, degenerated h-lines make no contribution.
      if (fy0 == fy1) {
        _ctx._builder.descendingCancel();
      }
      else {
        _ctx._boundingBoxI._y0 = Math::min(_ctx._boundingBoxI.y0(), fy0);
        _ctx._boundingBoxI._y1 = Math::max(_ctx._boundingBoxI.y1(), fy1);
        _ctx._builder.descendingClose(_signBit);
      }

      return kErrorOk;
    }

    EdgeBuilderFromSource& _ctx;
    uint32_t _signBit;
  };

  // --------------------------------------------------------------------------
  // [Init]
  // --------------------------------------------------------------------------

  B2D_INLINE EdgeBuilderFromSource(EdgeStorage<int>* storage, Zone* zone, const Box& clipBox, double toleranceSq, SourceT& source) noexcept :
    _storage(storage),
    _clipBoxD(clipBox),
    _clipBoxI(Math::itrunc(clipBox.x0()),
              Math::itrunc(clipBox.y0()),
              Math::itrunc(clipBox.x1()),
              Math::itrunc(clipBox.y1())),
    _boundingBoxI(std::numeric_limits<int>::max(),
                  std::numeric_limits<int>::max(),
                  std::numeric_limits<int>::min(),
                  std::numeric_limits<int>::min()),
    _source(source),
    _a(0.0, 0.0),
    _aFlags(0),
    _borderAccX0Y0(clipBox.y0()),
    _borderAccX0Y1(clipBox.y0()),
    _borderAccX1Y0(clipBox.y0()),
    _borderAccX1Y1(clipBox.y0()),
    _builder(zone, storage->_bandEdges, storage->_fixedBandHeightShift),
    _flattenData(toleranceSq) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE SourceT& source() const noexcept { return _source; }

  // --------------------------------------------------------------------------
  // [Build]
  // --------------------------------------------------------------------------

  B2D_INLINE Error build() noexcept {
    while (_source.begin(_a)) {
      Point start(_a);
      Point b;

      bool done = false;
      _aFlags = Clip::calcXYFlags(_a, _clipBoxD);

      for (;;) {
        if (_source.isLineTo()) {
          _source.nextLineTo(b);
LineTo:
          B2D_PROPAGATE(lineTo(b));
          if (done) break;
        }
        else if (_source.isQuadTo()) {
          B2D_PROPAGATE(quadTo());
        }
        else if (_source.isCubicTo()) {
          B2D_PROPAGATE(cubicTo());
        }
        else {
          b = start;
          done = true;
          goto LineTo;
        }
      }
      _source.beforeNextBegin();
    }

    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [LineTo]
  // --------------------------------------------------------------------------

  // Terminology:
  //
  //   'a' - Line start point.
  //   'b' - List end point.
  //
  //   'd' - Difference between 'b' and 'a'.
  //   'p' - Clipped start point.
  //   'q' - Clipped end point.

  B2D_INLINE Error lineTo(Point b) noexcept {
    Point& a = _a;
    uint32_t& aFlags = _aFlags;

    Point p, d;
    uint32_t bFlags;

    // FixedPoint coordinates.
    int fx0, fy0;
    int fx1, fy1;

    do {
      if (!aFlags) {
        // --------------------------------------------------------------------
        // [Line - Unclipped]
        // --------------------------------------------------------------------

        bFlags = Clip::calcXYFlags(b, _clipBoxD);
        if (!bFlags) {
          fx0 = Math::itrunc(a.x());
          fy0 = Math::itrunc(a.y());
          fx1 = Math::itrunc(b.x());
          fy1 = Math::itrunc(b.y());

          for (;;) {
            if (fy0 < fy1) {
DescendingLineBegin:
              B2D_PROPAGATE(_builder.descendingOpen());
              _builder.descendingAddUnsafe(fx0, fy0);
              _builder.descendingAddUnsafe(fx1, fy1);
              _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy0);

              // Instead of processing one vertex and swapping a/b each time
              // we process two vertices and swap only upon loop termination.
              for (;;) {
DescendingLineLoopA:
                if (!_source.maybeNextLineTo(a)) {
                  _builder.descendingClose();
                  _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy1);
                  a = b;
                  return kErrorOk;
                }

                bFlags = Clip::calcXYFlags(a, _clipBoxD);
                if (bFlags) {
                  _builder.descendingClose();
                  std::swap(a, b);
                  goto BeforeClipEndPoint;
                }

                fx0 = Math::itrunc(a.x());
                fy0 = Math::itrunc(a.y());

                if (fy0 < fy1) {
                  _builder.descendingClose();
                  B2D_PROPAGATE(_builder.ascendingOpen());
                  _builder.ascendingAddUnsafe(fx1, fy1);
                  _builder.ascendingAddUnsafe(fx0, fy0);
                  _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy1);
                  goto AscendingLineLoopB;
                }
                B2D_PROPAGATE(_builder.descendingAddChecked(fx0, fy0));

DescendingLineLoopB:
                if (!_source.maybeNextLineTo(b)) {
                  _builder.descendingClose();
                  _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy0);
                  return kErrorOk;
                }

                bFlags = Clip::calcXYFlags(b, _clipBoxD);
                if (bFlags) {
                  _builder.descendingClose();
                  _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy0);
                  goto BeforeClipEndPoint;
                }

                fx1 = Math::itrunc(b.x());
                fy1 = Math::itrunc(b.y());

                if (fy1 < fy0) {
                  _builder.descendingClose();
                  B2D_PROPAGATE(_builder.ascendingOpen());
                  _builder.ascendingAddUnsafe(fx0, fy0);
                  _builder.ascendingAddUnsafe(fx1, fy1);
                  _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy0);
                  goto AscendingLineLoopA;
                }
                B2D_PROPAGATE(_builder.descendingAddChecked(fx1, fy1));
              }

              _builder.descendingClose();
              if (bFlags) break;
            }
            else if (fy0 > fy1) {
AscendingLineBegin:
              B2D_PROPAGATE(_builder.ascendingOpen());
              _builder.ascendingAddUnsafe(fx0, fy0);
              _builder.ascendingAddUnsafe(fx1, fy1);
              _boundingBoxI._y1 = Math::max(_boundingBoxI._y1, fy0);

              // Instead of processing one vertex and swapping a/b each time
              // we process two vertices and swap only upon loop termination.
              for (;;) {
AscendingLineLoopA:
                if (!_source.maybeNextLineTo(a)) {
                  _builder.ascendingClose();
                  _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy1);
                  a = b;
                  return kErrorOk;
                }

                bFlags = Clip::calcXYFlags(a, _clipBoxD);
                if (bFlags) {
                  _builder.ascendingClose();
                  std::swap(a, b);
                  goto BeforeClipEndPoint;
                }

                fx0 = Math::itrunc(a.x());
                fy0 = Math::itrunc(a.y());

                if (fy0 > fy1) {
                  _builder.ascendingClose();
                  B2D_PROPAGATE(_builder.descendingOpen());
                  _builder.descendingAddUnsafe(fx1, fy1);
                  _builder.descendingAddUnsafe(fx0, fy0);
                  _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy1);
                  goto DescendingLineLoopB;
                }
                B2D_PROPAGATE(_builder.ascendingAddChecked(fx0, fy0));

AscendingLineLoopB:
                if (!_source.maybeNextLineTo(b)) {
                  _builder.ascendingClose();
                  _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy0);
                  return kErrorOk;
                }

                bFlags = Clip::calcXYFlags(b, _clipBoxD);
                if (bFlags) {
                  _builder.ascendingClose();
                  _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy0);
                  goto BeforeClipEndPoint;
                }

                fx1 = Math::itrunc(b.x());
                fy1 = Math::itrunc(b.y());

                if (fy1 > fy0) {
                  _builder.ascendingClose();
                  B2D_PROPAGATE(_builder.descendingOpen());
                  _builder.descendingAddUnsafe(fx0, fy0);
                  _builder.descendingAddUnsafe(fx1, fy1);
                  _boundingBoxI._y0 = Math::min(_boundingBoxI._y0, fy0);
                  goto DescendingLineLoopA;
                }
                B2D_PROPAGATE(_builder.ascendingAddChecked(fx1, fy1));
              }

              _builder.ascendingClose();
              if (bFlags) break;
            }
            else {
              a = b;
              if (!_source.maybeNextLineTo(b))
                return kErrorOk;

              bFlags = Clip::calcXYFlags(b, _clipBoxD);
              if (bFlags) break;

              fx0 = fx1;
              fy0 = fy1;
              fx1 = Math::itrunc(b.x());
              fy1 = Math::itrunc(b.y());
            }
          }
        }

BeforeClipEndPoint:
        p = a;
        d = b - a;
      }
      else {
        // --------------------------------------------------------------------
        // [Line - Partically or Completely Clipped]
        // --------------------------------------------------------------------

        double borY0;
        double borY1;

RestartClipLoop:
        if (aFlags & Clip::kFlagY0) {
          // Quickly skip all lines that are out of ClipBox or at its border.
          for (;;) {
            if (_clipBoxD.y0() < b.y()) break;           // xxxxxxxxxxxxxxxxxxx
            a = b;                                       // .                 .
            if (!_source.maybeNextLineTo(b)) {           // .                 .
              aFlags = Clip::calcXFlags(a, _clipBoxD) |  // .                 .
                       Clip::calcY0Flags(a, _clipBoxD);  // .                 .
              return kErrorOk;                           // .                 .
            }                                            // ...................
          }

          // Calculate flags we haven't updated inside the loop.
          aFlags = Clip::calcXFlags(a, _clipBoxD) | Clip::calcY0Flags(a, _clipBoxD);
          bFlags = Clip::calcXFlags(b, _clipBoxD) | Clip::calcY1Flags(b, _clipBoxD);

          borY0 = _clipBoxD.y0();
          uint32_t commonFlags = aFlags & bFlags;

          if (commonFlags) {
            borY1 = Math::min(_clipBoxD.y1(), b.y());
            if (commonFlags & Clip::kFlagX0)
              B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));
            else
              B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));

            a = b;
            aFlags = bFlags;
            continue;
          }
        }
        else if (aFlags & Clip::kFlagY1) {
          // Quickly skip all lines that are out of ClipBox or at its border.
          for (;;) {
            if (_clipBoxD.y1() > b.y()) break;           // ...................
            a = b;                                       // .                 .
            if (!_source.maybeNextLineTo(b)) {           // .                 .
              aFlags = Clip::calcXFlags(a, _clipBoxD) |  // .                 .
                       Clip::calcY1Flags(a, _clipBoxD);  // .                 .
              return kErrorOk;                           // .                 .
            }                                            // xxxxxxxxxxxxxxxxxxx
          }

          // Calculate flags we haven't updated inside the loop.
          aFlags = Clip::calcXFlags(a, _clipBoxD) | Clip::calcY1Flags(a, _clipBoxD);
          bFlags = Clip::calcXFlags(b, _clipBoxD) | Clip::calcY0Flags(b, _clipBoxD);

          borY0 = _clipBoxD.y1();
          uint32_t commonFlags = aFlags & bFlags;

          if (commonFlags) {
            borY1 = Math::max(_clipBoxD.y0(), b.y());
            if (commonFlags & Clip::kFlagX0)
              B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));
            else
              B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));

            a = b;
            aFlags = bFlags;
            continue;
          }
        }
        else if (aFlags & Clip::kFlagX0) {
          borY0 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());

          // Quickly skip all lines that are out of ClipBox or at its border.
          for (;;) {                                     // x..................
            if (_clipBoxD.x0() < b.x()) break;           // x                 .
            a = b;                                       // x                 .
            if (!_source.maybeNextLineTo(b)) {           // x                 .
              aFlags = Clip::calcYFlags(a, _clipBoxD) |  // x                 .
                       Clip::calcX0Flags(a, _clipBoxD);  // x..................
              borY1 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());
              if (borY0 != borY1)
                B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));
              return kErrorOk;
            }
          }

          borY1 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());
          if (borY0 != borY1)
            B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));

          // Calculate flags we haven't updated inside the loop.
          aFlags = Clip::calcX0Flags(a, _clipBoxD) | Clip::calcYFlags(a, _clipBoxD);
          bFlags = Clip::calcX1Flags(b, _clipBoxD) | Clip::calcYFlags(b, _clipBoxD);

          if (aFlags & bFlags)
            goto RestartClipLoop;

          borY0 = borY1;
        }
        else {
          borY0 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());

          // Quickly skip all lines that are out of ClipBox or at its border.
          for (;;) {                                     // ..................x
            if (_clipBoxD.x1() > b.x()) break;           // .                 x
            a = b;                                       // .                 x
            if (!_source.maybeNextLineTo(b)) {           // .                 x
              aFlags = Clip::calcYFlags(a, _clipBoxD) |  // .                 x
                       Clip::calcX1Flags(a, _clipBoxD);  // ..................x
              borY1 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());
              if (borY0 != borY1)
                B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));
              return kErrorOk;
            }
          }

          double borY1 = Math::bound(a.y(), _clipBoxD.y0(), _clipBoxD.y1());
          if (borY0 != borY1)
            B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));

          // Calculate flags we haven't updated inside the loop.
          aFlags = Clip::calcX1Flags(a, _clipBoxD) | Clip::calcYFlags(a, _clipBoxD);
          bFlags = Clip::calcX0Flags(b, _clipBoxD) | Clip::calcYFlags(b, _clipBoxD);

          if (aFlags & bFlags)
            goto RestartClipLoop;

          borY0 = borY1;
        }

        // --------------------------------------------------------------------
        // [Line - Clip Start Point]
        // --------------------------------------------------------------------

        // The start point of the line requires clipping.
        d = b - a;
        p._x = _clipBoxD.x1();
        p._y = _clipBoxD.y1();

        switch (aFlags) {
          case Clip::kNoFlags:
            p = a;
            break;

          case Clip::kFlagX0Y0:
            p._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1Y0:
            p._y = a.y() + (p.x() - a.x()) * d.y() / d.x();
            aFlags = Clip::calcYFlags(p, _clipBoxD);

            if (p.y() >= _clipBoxD.y0())
              break;
            B2D_FALLTHROUGH;

          case Clip::kFlagY0:
            p._y = _clipBoxD.y0();
            p._x = a.x() + (p.y() - a.y()) * d.x() / d.y();

            aFlags = Clip::calcXFlags(p, _clipBoxD);
            break;

          case Clip::kFlagX0Y1:
            p._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1Y1:
            p._y = a.y() + (p.x() - a.x()) * d.y() / d.x();
            aFlags = Clip::calcYFlags(p, _clipBoxD);

            if (p.y() <= _clipBoxD.y1())
              break;
            B2D_FALLTHROUGH;

          case Clip::kFlagY1:
            p._y = _clipBoxD.y1();
            p._x = a.x() + (p.y() - a.y()) * d.x() / d.y();

            aFlags = Clip::calcXFlags(p, _clipBoxD);
            break;

          case Clip::kFlagX0:
            p._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1:
            p._y = a.y() + (p.x() - a.x()) * d.y() / d.x();

            aFlags = Clip::calcYFlags(p, _clipBoxD);
            break;

          default:
            B2D_NOT_REACHED();
        }

        if (aFlags) {
          borY1 = Math::bound(b.y(), _clipBoxD.y0(), _clipBoxD.y1());
          if (p.x() <= _clipBoxD.x0())
            B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));
          else if (p.x() >= _clipBoxD.x1())
            B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));

          a = b;
          aFlags = bFlags;
          continue;
        }

        borY1 = Math::bound(p.y(), _clipBoxD.y0(), _clipBoxD.y1());
        if (borY0 != borY1) {
          if (p.x() <= _clipBoxD.x0())
            B2D_PROPAGATE(accumulateLeftBorder(borY0, borY1));
          else
            B2D_PROPAGATE(accumulateRightBorder(borY0, borY1));
        }

        if (!bFlags) {
          a = b;
          aFlags = 0;

          fx0 = Math::itrunc(p.x());
          fy0 = Math::itrunc(p.y());
          fx1 = Math::itrunc(b.x());
          fy1 = Math::itrunc(b.y());

          if (fy0 == fy1)
            continue;

          if (fy0 < fy1)
            goto DescendingLineBegin;
          else
            goto AscendingLineBegin;
        }
      }

      {
        // --------------------------------------------------------------------
        // [Line - Clip End Point]
        // --------------------------------------------------------------------

        Point q(_clipBoxD.x1(), _clipBoxD.y1());

        B2D_ASSERT(bFlags != 0);
        switch (bFlags) {
          case Clip::kFlagX0Y0:
            q._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1Y0:
            q._y = a.y() + (q.x() - a.x()) * d.y() / d.x();
            if (q.y() >= _clipBoxD.y0())
              break;
            B2D_FALLTHROUGH;

          case Clip::kFlagY0:
            q._y = _clipBoxD.y0();
            q._x = a.x() + (q.y() - a.y()) * d.x() / d.y();
            break;

          case Clip::kFlagX0Y1:
            q._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1Y1:
            q._y = a.y() + (q.x() - a.x()) * d.y() / d.x();
            if (q.y() <= _clipBoxD.y1())
              break;
            B2D_FALLTHROUGH;

          case Clip::kFlagY1:
            q._y = _clipBoxD.y1();
            q._x = a.x() + (q.y() - a.y()) * d.x() / d.y();
            break;

          case Clip::kFlagX0:
            q._x = _clipBoxD.x0();
            B2D_FALLTHROUGH;

          case Clip::kFlagX1:
            q._y = a.y() + (q.x() - a.x()) * d.y() / d.x();
            break;

          default:
            B2D_NOT_REACHED();
        }

        B2D_PROPAGATE(addLineSegment(p.x(), p.y(), q.x(), q.y()));
        double clippedBY = Math::bound(b.y(), _clipBoxD.y0(), _clipBoxD.y1());

        if (q.y() != clippedBY) {
          if (q.x() == _clipBoxD.x0())
            B2D_PROPAGATE(accumulateLeftBorder(q.y(), clippedBY));
          else
            B2D_PROPAGATE(accumulateRightBorder(q.y(), clippedBY));
        }
      }

      a = b;
      aFlags = bFlags;
    } while (_source.maybeNextLineTo(b));

    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [QuadTo / QuadSpline]
  // --------------------------------------------------------------------------

  // Terminology:
  //
  //   'p0' - Quad start point.
  //   'p1' - Quad control point.
  //   'p2' - Quad end point.

  B2D_INLINE Error quadTo() noexcept {
    // 2 extremas, 1 cusp, and 1 terminating `1.0` value.
    constexpr uint32_t kMaxTCount = 2 + 1 + 1;

    Point spline[kMaxTCount * 2 + 1];
    Point& p0 = _a;
    Point& p1 = spline[1];
    Point& p2 = spline[2];

    uint32_t& p0Flags = _aFlags;
    _source.nextQuadTo(p1, p2);

    for (;;) {
      uint32_t p1Flags = Clip::calcXYFlags(p1, _clipBoxD);
      uint32_t p2Flags = Clip::calcXYFlags(p2, _clipBoxD);
      uint32_t commonFlags = p0Flags & p1Flags & p2Flags;

      // Fast reject.
      if (commonFlags) {
        uint32_t end = 0;

        if (commonFlags & Clip::kFlagY0) {
          // CLIPPED OUT: Above top (fast).
          for (;;) {
            p0 = p2;
            end = !_source.isQuadTo();
            if (end) break;

            _source.nextQuadTo(p1, p2);
            if (!((p1.y() <= _clipBoxD.y0()) & (p2.y() <= _clipBoxD.y0())))
              break;
          }
        }
        else if (commonFlags & Clip::kFlagY1) {
          // CLIPPED OUT: Below bottom (fast).
          for (;;) {
            p0 = p2;
            end = !_source.isQuadTo();
            if (end) break;

            _source.nextQuadTo(p1, p2);
            if (!((p1.y() >= _clipBoxD.y1()) & (p2.y() >= _clipBoxD.y1())))
              break;
          }
        }
        else {
          // CLIPPED OUT: Before left or after right (border-line required).
          double y0 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());

          if (commonFlags & Clip::kFlagX0) {
            for (;;) {
              p0 = p2;
              end = !_source.isQuadTo();
              if (end) break;

              _source.nextQuadTo(p1, p2);
              if (!((p1.x() <= _clipBoxD.x0()) & (p2.x() <= _clipBoxD.x0())))
                break;
            }

            double y1 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());
            B2D_PROPAGATE(accumulateLeftBorder(y0, y1));
          }
          else {
            for (;;) {
              p0 = p2;
              end = !_source.isQuadTo();
              if (end) break;

              _source.nextQuadTo(p1, p2);
              if (!((p1.x() >= _clipBoxD.x1()) & (p2.x() >= _clipBoxD.x1())))
                break;
            }

            double y1 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());
            B2D_PROPAGATE(accumulateRightBorder(y0, y1));
          }
        }

        p0Flags = Clip::calcXYFlags(p0, _clipBoxD);
        if (end)
          return kErrorOk;
        continue;
      }

      Point Pa, Pb, Pc;
      spline[0] = p0;

      double tArray[kMaxTCount];
      size_t tCount = 0;

      {
        Point extremaTs = (p0 - p1) / (p0 - p1 * 2.0 + p2);
        Geom2D::extractQuadPolynomial(spline, Pa, Pb, Pc);

        double extremaT0 = Math::min(extremaTs.x(), extremaTs.y());
        double extremaT1 = Math::max(extremaTs.x(), extremaTs.y());

        tArray[0] = extremaT0;
        tCount = size_t((extremaT0 > 0.0) & (extremaT0 < 1.0));

        tArray[tCount] = extremaT1;
        tCount += size_t((extremaT1 > Math::max(extremaT0, 0.0)) & (extremaT1 < 1.0));
      }

      Point* splinePtr = spline;
      Point* splineEnd = spline + 2;

      // Split the curve into a spline, if necessary.
      if (tCount) {
        Point last = p2;

        tArray[tCount++] = 1.0;
        B2D_ASSERT(tCount <= kMaxTCount);

        size_t tIndex = 0;
        double tCut = 0.0;
        splineEnd = splinePtr;

        do {
          double tVal = tArray[tIndex];
          B2D_ASSERT(tVal >  0.0);
          B2D_ASSERT(tVal <= 1.0);

          double dt = (tVal - tCut) * 0.5;

          // Derivative: 2a*t + b.
          Point cp { (Pa * (tVal * 2.0) + Pb) * dt };
          Point tp { (Pa * tVal + Pb) * tVal + Pc };

          // The last point must be exact.
          if (++tIndex == tCount)
            tp = last;

          splineEnd[1].reset(tp - cp);
          splineEnd[2].reset(tp);
          splineEnd += 2;

          tCut = tVal;
        } while (tIndex != tCount);
      }

      Appender appender(*this);
      FlattenMonoQuad monoCurve(_flattenData);

      uint32_t anyFlags = p0Flags | p1Flags | p2Flags;
      if (anyFlags) {
        // One or more quad may need clipping.
        do {
          uint32_t signBit = splinePtr[0].y() > splinePtr[2].y();
          B2D_PROPAGATE(
            flattenUnsafeMonoCurve<FlattenMonoQuad>(monoCurve, appender, splinePtr, signBit)
          );
        } while ((splinePtr += 2) != splineEnd);

        p0 = splineEnd[0];
        p0Flags = p2Flags;
      }
      else {
        // No clipping - optimized fast-path.
        do {
          uint32_t signBit = splinePtr[0].y() > splinePtr[2].y();
          B2D_PROPAGATE(
            flattenSafeMonoCurve<FlattenMonoQuad>(monoCurve, appender, splinePtr, signBit)
          );
        } while ((splinePtr += 2) != splineEnd);

        p0 = splineEnd[0];
      }

      if (!_source.maybeNextQuadTo(p1, p2))
        return kErrorOk;
    }
  }

  // --------------------------------------------------------------------------
  // [CubicTo / CubicSpline]
  // --------------------------------------------------------------------------

  // Terminology:
  //
  //   'p0' - Cubic start point.
  //   'p1' - Cubic first control point.
  //   'p2' - Cubic second control point.
  //   'p3' - Cubic end point.

  B2D_INLINE Error cubicTo() noexcept {
    // 4 extremas, 2 inflections, 1 cusp, and 1 terminating `1.0` value.
    constexpr uint32_t kMaxTCount = 4 + 2 + 1 + 1;

    Point spline[kMaxTCount * 3 + 1];
    Point& p0 = _a;
    Point& p1 = spline[1];
    Point& p2 = spline[2];
    Point& p3 = spline[3];

    uint32_t& p0Flags = _aFlags;
    _source.nextCubicTo(p1, p2, p3);

    for (;;) {
      uint32_t p1Flags = Clip::calcXYFlags(p1, _clipBoxD);
      uint32_t p2Flags = Clip::calcXYFlags(p2, _clipBoxD);
      uint32_t p3Flags = Clip::calcXYFlags(p3, _clipBoxD);
      uint32_t commonFlags = p0Flags & p1Flags & p2Flags & p3Flags;

      // Fast reject.
      if (commonFlags) {
        uint32_t end = 0;

        if (commonFlags & Clip::kFlagY0) {
          // CLIPPED OUT: Above top (fast).
          for (;;) {
            p0 = p3;
            end = !_source.isCubicTo();
            if (end) break;

            _source.nextCubicTo(p1, p2, p3);
            if (!((p1.y() <= _clipBoxD.y0()) & (p2.y() <= _clipBoxD.y0()) & (p3.y() <= _clipBoxD.y0())))
              break;
          }
        }
        else if (commonFlags & Clip::kFlagY1) {
          // CLIPPED OUT: Below bottom (fast).
          for (;;) {
            p0 = p3;
            end = !_source.isCubicTo();
            if (end) break;

            _source.nextCubicTo(p1, p2, p3);
            if (!((p1.y() >= _clipBoxD.y1()) & (p2.y() >= _clipBoxD.y1()) & (p3.y() >= _clipBoxD.y1())))
              break;
          }
        }
        else {
          // CLIPPED OUT: Before left or after right (border-line required).
          double y0 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());

          if (commonFlags & Clip::kFlagX0) {
            for (;;) {
              p0 = p3;
              end = !_source.isCubicTo();
              if (end) break;

              _source.nextCubicTo(p1, p2, p3);
              if (!((p1.x() <= _clipBoxD.x0()) & (p2.x() <= _clipBoxD.x0()) & (p3.x() <= _clipBoxD.x0())))
                break;
            }

            double y1 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());
            B2D_PROPAGATE(accumulateLeftBorder(y0, y1));
          }
          else {
            for (;;) {
              p0 = p3;
              end = !_source.isCubicTo();
              if (end) break;

              _source.nextCubicTo(p1, p2, p3);
              if (!((p1.x() >= _clipBoxD.x1()) & (p2.x() >= _clipBoxD.x1()) & (p3.x() >= _clipBoxD.x1())))
                break;
            }

            double y1 = Math::bound(p0.y(), _clipBoxD.y0(), _clipBoxD.y1());
            B2D_PROPAGATE(accumulateRightBorder(y0, y1));
          }
        }

        p0Flags = Clip::calcXYFlags(p0, _clipBoxD);
        if (end)
          return kErrorOk;
        continue;
      }

      Point Pa, Pb, Pc, Pd;
      spline[0] = p0;

      double tArray[kMaxTCount];
      size_t tCount = 0;

      {
        Geom2D::extractCubicPolynomial(spline, Pa, Pb, Pc, Pd);

        double q0 = Math::cross(Pb, Pa);
        double q1 = Math::cross(Pc, Pa);
        double q2 = Math::cross(Pc, Pb);

        // Find cusp.
        double tCusp = (q1 / q0) * -0.5;
        tArray[0] = tCusp;
        tCount = (tCusp > 0.0) & (tCusp < 1.0);

        // Find inflections.
        tCount += Math::quadRoots(tArray + tCount, q0 * 6.0, q1 * 6.0, q2 * 2.0, Math::kAfter0, Math::kBefore1);

        // Find extremas.
        Point Da, Db, Dc;
        Geom2D::extractCubicDerivative(spline, Da, Db, Dc);

        tCount += Math::quadRoots(tArray + tCount, Da.x(), Db.x(), Dc.x(), Math::kAfter0, Math::kBefore1);
        tCount += Math::quadRoots(tArray + tCount, Da.y(), Db.y(), Dc.y(), Math::kAfter0, Math::kBefore1);
      }

      Point* splinePtr = spline;
      Point* splineEnd = spline + 3;

      // Split the curve into a spline, if necessary.
      if (tCount) {
        Point last = p3;

        Support::iSort(tArray, tCount);
        tArray[tCount++] = 1.0;
        B2D_ASSERT(tCount <= kMaxTCount);

        size_t tIndex = 0;
        double tCut = 0.0;
        splineEnd = splinePtr;

        do {
          double tVal = tArray[tIndex++];
          B2D_ASSERT(tVal >  0.0);
          B2D_ASSERT(tVal <= 1.0);

          // Ignore all Ts which are the same as the previous one (border case).
          if (tVal == tCut)
            continue;

          constexpr double k1Div3 = 1.0 / 3.0;
          double dt = (tVal - tCut) * k1Div3;

          Point tp { ((Pa * tVal + Pb) * tVal + Pc) * tVal + Pd };

          // The last point must be exact.
          if (tIndex == tCount)
            tp = last;

          // Derivative: 3At^2 + 2Bt + c
          //             (3At + 2B)t + c
          Point cp1 { ((Pa * (tCut * 3.0) + Pb * 2.0) * tCut + Pc) * dt };
          Point cp2 { ((Pa * (tVal * 3.0) + Pb * 2.0) * tVal + Pc) * dt };

          splineEnd[1].reset(splineEnd[0] + cp1);
          splineEnd[2].reset(tp - cp2);
          splineEnd[3].reset(tp);
          splineEnd += 3;

          tCut = tVal;
        } while (tIndex != tCount);
      }

      Appender appender(*this);
      FlattenMonoCubic monoCurve(_flattenData);

      uint32_t anyFlags = p0Flags | p1Flags | p2Flags | p3Flags;
      if (anyFlags) {
        // One or more cubic may need clipping.
        do {
          uint32_t signBit = splinePtr[0].y() > splinePtr[3].y();
          B2D_PROPAGATE(
            flattenUnsafeMonoCurve<FlattenMonoCubic>(monoCurve, appender, splinePtr, signBit)
          );
        } while ((splinePtr += 3) != splineEnd);

        p0 = splineEnd[0];
        p0Flags = p3Flags;
      }
      else {
        // No clipping - optimized fast-path.
        do {
          uint32_t signBit = splinePtr[0].y() > splinePtr[3].y();
          B2D_PROPAGATE(
            flattenSafeMonoCurve<FlattenMonoCubic>(monoCurve, appender, splinePtr, signBit)
          );
        } while ((splinePtr += 3) != splineEnd);

        p0 = splineEnd[0];
      }

      if (!_source.maybeNextCubicTo(p1, p2, p3))
        return kErrorOk;
    }
  }

  // --------------------------------------------------------------------------
  // [Curve Utilities]
  // --------------------------------------------------------------------------

  template<typename MonoCurveT>
  B2D_INLINE Error flattenSafeMonoCurve(MonoCurveT& monoCurve, Appender& appender, const Point* src, uint32_t signBit) noexcept {
    monoCurve.begin(src, signBit);
    appender.setSignBit(signBit);

    if (monoCurve.isLeftToRight())
      monoCurve.boundLeftToRight();
    else
      monoCurve.boundRightToLeft();

    B2D_PROPAGATE(appender.openAt(monoCurve.first().x(), monoCurve.first().y()));
    for (;;) {
      typename MonoCurveT::SplitStep step;
      if (!monoCurve.isFlat(step)) {
        if (monoCurve.canPush()) {
          monoCurve.split(step);
          monoCurve.push(step);
          continue;
        }
        else {
          // The curve is either invalid or the tolerance is too strict.
          // We shouldn't get INF nor NaNs here as we know we are within
          // the clipBox.
          B2D_ASSERT(step.isFinite());
        }
      }

      B2D_PROPAGATE(appender.addLine(monoCurve.last().x(), monoCurve.last().y()));
      if (!monoCurve.canPop())
        break;
      monoCurve.pop();
    }

    appender.close();
    return kErrorOk;
  }

  // Clips and flattens a monotonic curve - works for both quadratics and cubics.
  //
  // The idea behind this function is to quickly subdivide to find the intersection
  // with ClipBox. When the intersection is found the intersecting line is clipped
  // and the subdivision continues until the end of the curve or until another
  // intersection is found, which would be the end of the curve. The algorithm
  // handles all cases and accumulates border lines when necessary.
  template<typename MonoCurveT>
  B2D_INLINE Error flattenUnsafeMonoCurve(MonoCurveT& monoCurve, Appender& appender, const Point* src, uint32_t signBit) noexcept {
    monoCurve.begin(src, signBit);
    appender.setSignBit(signBit);

    double yStart = monoCurve.first().y();
    double yEnd = Math::min(monoCurve.last().y(), _clipBoxD.y1());

    if ((yStart >= yEnd) | (yEnd <= _clipBoxD.y0()))
      return kErrorOk;

    uint32_t completelyOut = 0;
    typename MonoCurveT::SplitStep step;

    if (monoCurve.isLeftToRight()) {
      // Left-To-Right
      // ------------>
      //
      //  ...__
      //       --._
      //           *_
      monoCurve.boundLeftToRight();

      if (yStart < _clipBoxD.y0()) {
        yStart = _clipBoxD.y0();
        for (;;) {
          // CLIPPED OUT: Above ClipBox.y0
          // -----------------------------

          completelyOut = (monoCurve.first().x() >= _clipBoxD.x1());
          if (completelyOut)
            break;

          if (!monoCurve.isFlat(step)) {
            monoCurve.split(step);

            if (step.midPoint().y() <= _clipBoxD.y0()) {
              monoCurve.discardAndAdvance(step);
              continue;
            }

            if (monoCurve.canPush()) {
              monoCurve.push(step);
              continue;
            }
          }

          if (monoCurve.last().y() > _clipBoxD.y0()) {
            if (monoCurve.last().x() < _clipBoxD.x0())
              goto LeftToRight_BeforeX0_Pop;

            double xClipped = monoCurve.first().x() + (_clipBoxD.y0() - monoCurve.first().y()) * dx_div_dy(monoCurve.last() - monoCurve.first());
            if (xClipped <= _clipBoxD.x0())
              goto LeftToRight_BeforeX0_Clip;

            completelyOut = (xClipped >= _clipBoxD.x1());
            if (completelyOut)
              break;

            B2D_PROPAGATE(appender.openAt(xClipped, _clipBoxD.y0()));
            goto LeftToRight_AddLine;
          }

          if (!monoCurve.canPop())
            break;

          monoCurve.pop();
        }
        completelyOut <<= Clip::kShiftX1;
      }
      else if (yStart < _clipBoxD.y1()) {
        if (monoCurve.first().x() < _clipBoxD.x0()) {
          // CLIPPED OUT: Before ClipBox.x0
          // ------------------------------

          for (;;) {
            completelyOut = (monoCurve.first().y() >= _clipBoxD.y1());
            if (completelyOut)
              break;

            if (!monoCurve.isFlat(step)) {
              monoCurve.split(step);

              if (step.midPoint().x() <= _clipBoxD.x0()) {
                monoCurve.discardAndAdvance(step);
                continue;
              }

              if (monoCurve.canPush()) {
                monoCurve.push(step);
                continue;
              }
            }

            if (monoCurve.last().x() > _clipBoxD.x0()) {
LeftToRight_BeforeX0_Clip:
              double yClipped = monoCurve.first().y() + (_clipBoxD.x0() - monoCurve.first().x()) * dy_div_dx(monoCurve.last() - monoCurve.first());
              completelyOut = (yClipped >= yEnd);

              if (completelyOut)
                break;

              if (yStart < yClipped)
                B2D_PROPAGATE(accumulateLeftBorder(yStart, yClipped, signBit));

              B2D_PROPAGATE(appender.openAt(_clipBoxD.x0(), yClipped));
              goto LeftToRight_AddLine;
            }

            completelyOut = (monoCurve.last().y() >= yEnd);
            if (completelyOut)
              break;

LeftToRight_BeforeX0_Pop:
            if (!monoCurve.canPop())
              break;

            monoCurve.pop();
          }
          completelyOut <<= Clip::kShiftX0;
        }
        else if (monoCurve.first().x() < _clipBoxD.x1()) {
          // VISIBLE CASE
          // ------------

          B2D_PROPAGATE(appender.openAt(monoCurve.first().x(), monoCurve.first().y()));
          for (;;) {
            if (!monoCurve.isFlat(step)) {
              monoCurve.split(step);

              if (monoCurve.canPush()) {
                monoCurve.push(step);
                continue;
              }
            }

LeftToRight_AddLine:
            completelyOut = monoCurve.last().x() > _clipBoxD.x1();
            if (completelyOut) {
              double yClipped = monoCurve.first().y() + (_clipBoxD.x1() - monoCurve.first().x()) * dy_div_dx(monoCurve.last() - monoCurve.first());
              if (yClipped <= yEnd) {
                yStart = yClipped;
                B2D_PROPAGATE(appender.addLine(_clipBoxD.x1(), yClipped));
                break;
              }
            }

            completelyOut = monoCurve.last().y() >= _clipBoxD.y1();
            if (completelyOut) {
              double xClipped = Math::min(monoCurve.first().x() + (_clipBoxD.y1() - monoCurve.first().y()) * dx_div_dy(monoCurve.last() - monoCurve.first()), _clipBoxD.x1());
              B2D_PROPAGATE(appender.addLine(xClipped, _clipBoxD.y1()));

              completelyOut = 0;
              break;
            }

            B2D_PROPAGATE(appender.addLine(monoCurve.last().x(), monoCurve.last().y()));
            if (!monoCurve.canPop())
              break;
            monoCurve.pop();
          }
          appender.close();
          completelyOut <<= Clip::kShiftX1;
        }
        else {
          completelyOut = Clip::kFlagX1;
        }
      }
      else {
        // Below bottom or invalid, ignore this part...
      }
    }
    else {
      // Right-To-Left
      // <------------
      //
      //        __...
      //    _.--
      //  _*
      monoCurve.boundRightToLeft();

      if (yStart < _clipBoxD.y0()) {
        yStart = _clipBoxD.y0();
        for (;;) {
          // CLIPPED OUT: Above ClipBox.y0
          // -----------------------------

          completelyOut = (monoCurve.first().x() <= _clipBoxD.x0());
          if (completelyOut)
            break;

          if (!monoCurve.isFlat(step)) {
            monoCurve.split(step);

            if (step.midPoint().y() <= _clipBoxD.y0()) {
              monoCurve.discardAndAdvance(step);
              continue;
            }

            if (monoCurve.canPush()) {
              monoCurve.push(step);
              continue;
            }
          }

          if (monoCurve.last().y() > _clipBoxD.y0()) {
            if (monoCurve.last().x() > _clipBoxD.x1())
              goto RightToLeft_AfterX1_Pop;

            double xClipped = monoCurve.first().x() + (_clipBoxD.y0() - monoCurve.first().y()) * dx_div_dy(monoCurve.last() - monoCurve.first());
            if (xClipped >= _clipBoxD.x1())
              goto RightToLeft_AfterX1_Clip;

            completelyOut = (xClipped <= _clipBoxD.x0());
            if (completelyOut)
              break;

            B2D_PROPAGATE(appender.openAt(xClipped, _clipBoxD.y0()));
            goto RightToLeft_AddLine;
          }

          if (!monoCurve.canPop())
            break;

          monoCurve.pop();
        }
        completelyOut <<= Clip::kShiftX0;
      }
      else if (yStart < _clipBoxD.y1()) {
        if (monoCurve.first().x() > _clipBoxD.x1()) {
          // CLIPPED OUT: After ClipBox.x1
          // ------------------------------

          for (;;) {
            completelyOut = (monoCurve.first().y() >= _clipBoxD.y1());
            if (completelyOut)
              break;

            if (!monoCurve.isFlat(step)) {
              monoCurve.split(step);

              if (step.midPoint().x() >= _clipBoxD.x1()) {
                monoCurve.discardAndAdvance(step);
                continue;
              }

              if (monoCurve.canPush()) {
                monoCurve.push(step);
                continue;
              }
            }

            if (monoCurve.last().x() < _clipBoxD.x1()) {
RightToLeft_AfterX1_Clip:
              double yClipped = monoCurve.first().y() + (_clipBoxD.x1() - monoCurve.first().x()) * dy_div_dx(monoCurve.last() - monoCurve.first());
              completelyOut = (yClipped >= yEnd);

              if (completelyOut)
                break;

              if (yStart < yClipped)
                B2D_PROPAGATE(accumulateRightBorder(yStart, yClipped, signBit));

              B2D_PROPAGATE(appender.openAt(_clipBoxD.x1(), yClipped));
              goto RightToLeft_AddLine;
            }

            completelyOut = (monoCurve.last().y() >= yEnd);
            if (completelyOut)
              break;

RightToLeft_AfterX1_Pop:
            if (!monoCurve.canPop())
              break;

            monoCurve.pop();
          }
          completelyOut <<= Clip::kShiftX1;
        }
        else if (monoCurve.first().x() > _clipBoxD.x0()) {
          // VISIBLE CASE
          // ------------

          B2D_PROPAGATE(appender.openAt(monoCurve.first().x(), monoCurve.first().y()));
          for (;;) {
            if (!monoCurve.isFlat(step)) {
              monoCurve.split(step);

              if (monoCurve.canPush()) {
                monoCurve.push(step);
                continue;
              }
            }

RightToLeft_AddLine:
            completelyOut = monoCurve.last().x() < _clipBoxD.x0();
            if (completelyOut) {
              double yClipped = monoCurve.first().y() + (_clipBoxD.x0() - monoCurve.first().x()) * dy_div_dx(monoCurve.last() - monoCurve.first());
              if (yClipped <= yEnd) {
                yStart = yClipped;
                B2D_PROPAGATE(appender.addLine(_clipBoxD.x0(), yClipped));
                break;
              }
            }

            completelyOut = monoCurve.last().y() >= _clipBoxD.y1();
            if (completelyOut) {
              double xClipped = Math::max(monoCurve.first().x() + (_clipBoxD.y1() - monoCurve.first().y()) * dx_div_dy(monoCurve.last() - monoCurve.first()), _clipBoxD.x0());
              B2D_PROPAGATE(appender.addLine(xClipped, _clipBoxD.y1()));

              completelyOut = 0;
              break;
            }

            B2D_PROPAGATE(appender.addLine(monoCurve.last().x(), monoCurve.last().y()));
            if (!monoCurve.canPop())
              break;
            monoCurve.pop();
          }
          appender.close();
          completelyOut <<= Clip::kShiftX0;
        }
        else {
          completelyOut = Clip::kFlagX0;
        }
      }
      else {
        // Below bottom or invalid, ignore this part...
      }
    }

    if (completelyOut && yStart < yEnd) {
      if (completelyOut & Clip::kFlagX0)
        B2D_PROPAGATE(accumulateLeftBorder(yStart, yEnd, signBit));
      else
        B2D_PROPAGATE(accumulateRightBorder(yStart, yEnd, signBit));
    }

    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Generic Utilities]
  // --------------------------------------------------------------------------

  B2D_INLINE Error addLineSegment(double x0, double y0, double x1, double y1) noexcept {
    int fx0 = Math::itrunc(x0);
    int fy0 = Math::itrunc(y0);
    int fx1 = Math::itrunc(x1);
    int fy1 = Math::itrunc(y1);

    if (fy0 == fy1)
      return kErrorOk;

    if (fy0 < fy1) {
      _boundingBoxI._y0 = Math::min(_boundingBoxI.y0(), fy0);
      _boundingBoxI._y1 = Math::max(_boundingBoxI.y1(), fy1);
      return _builder.addClosedLine(fx0, fy0, fx1, fy1, 0);
    }
    else {
      _boundingBoxI._y0 = Math::min(_boundingBoxI.y0(), fy1);
      _boundingBoxI._y1 = Math::max(_boundingBoxI.y1(), fy0);
      return _builder.addClosedLine(fx1, fy1, fx0, fy0, 1);
    }
  }

  // --------------------------------------------------------------------------
  // [Flush]
  // --------------------------------------------------------------------------

  B2D_INLINE Error flush(uint32_t flushFlags) noexcept {
    if (flushFlags & EdgeBuilderFlags::kFlushBorderAccumulators)
      B2D_PROPAGATE(flushBorderAccumulators());

    if (flushFlags & EdgeBuilderFlags::kFlushBoundingBox)
      flushBoundingBox();

    return kErrorOk;
  }

  B2D_INLINE Error flushBorderAccumulators() noexcept {
    B2D_PROPAGATE(_emitLeftBorder());
    B2D_PROPAGATE(_emitRightBorder());

    resetBorderAccumulators();
    return kErrorOk;
  }

  B2D_INLINE void flushBoundingBox() noexcept {
    IntBox::bound(_storage->_boundingBox, _storage->_boundingBox, _boundingBoxI);
  }

  // --------------------------------------------------------------------------
  // [Border Accumulators]
  // --------------------------------------------------------------------------

  B2D_INLINE void resetBorderAccumulators() noexcept {
    _borderAccX0Y0 = _borderAccX0Y1;
    _borderAccX1Y0 = _borderAccX1Y1;
  }

  B2D_INLINE Error accumulateLeftBorder(double y0, double y1) noexcept {
    if (_borderAccX0Y1 == y0) {
      _borderAccX0Y1 = y1;
      return kErrorOk;
    }

    B2D_PROPAGATE(_emitLeftBorder());
    _borderAccX0Y0 = y0;
    _borderAccX0Y1 = y1;
    return kErrorOk;
  }

  B2D_INLINE Error accumulateLeftBorder(double y0, double y1, uint32_t signBit) noexcept {
    if (signBit == 0)
      return accumulateLeftBorder(y0, y1);
    else
      return accumulateLeftBorder(y1, y0);
  }

  B2D_INLINE Error accumulateRightBorder(double y0, double y1) noexcept {
    if (_borderAccX1Y1 == y0) {
      _borderAccX1Y1 = y1;
      return kErrorOk;
    }

    B2D_PROPAGATE(_emitRightBorder());
    _borderAccX1Y0 = y0;
    _borderAccX1Y1 = y1;
    return kErrorOk;
  }

  B2D_INLINE Error accumulateRightBorder(double y0, double y1, uint32_t signBit) noexcept {
    if (signBit == 0)
      return accumulateRightBorder(y0, y1);
    else
      return accumulateRightBorder(y1, y0);
  }

  B2D_INLINE Error _emitLeftBorder() noexcept {
    int accY0 = Math::itrunc(_borderAccX0Y0);
    int accY1 = Math::itrunc(_borderAccX0Y1);

    if (accY0 == accY1)
      return kErrorOk;

    int minY = Math::min(accY0, accY1);
    int maxY = Math::max(accY0, accY1);

    _boundingBoxI._y0 = Math::min(_boundingBoxI.y0(), minY);
    _boundingBoxI._y1 = Math::max(_boundingBoxI.y1(), maxY);

    return _builder.addClosedLine(_clipBoxI.x0(), minY, _clipBoxI.x0(), maxY, accY0 > accY1);
  }

  B2D_INLINE Error _emitRightBorder() noexcept {
    int accY0 = Math::itrunc(_borderAccX1Y0);
    int accY1 = Math::itrunc(_borderAccX1Y1);

    if (accY0 == accY1)
      return kErrorOk;

    int minY = Math::min(accY0, accY1);
    int maxY = Math::max(accY0, accY1);

    _boundingBoxI._y0 = Math::min(_boundingBoxI.y0(), minY);
    _boundingBoxI._y1 = Math::max(_boundingBoxI.y1(), maxY);

    return _builder.addClosedLine(_clipBoxI.x1(), minY, _clipBoxI.x1(), maxY, accY0 > accY1);
  }

  // TODO: This should go somewhere else, also the name doesn't make much sense...
  static B2D_INLINE double dx_div_dy(const Point& d) noexcept { return d.x() / d.y(); }
  static B2D_INLINE double dy_div_dx(const Point& d) noexcept { return d.y() / d.x(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  EdgeStorage<int>* _storage;
  Box _clipBoxD;
  IntBox _clipBoxI;
  IntBox _boundingBoxI;
  SourceT& _source;

  Point _a;
  Point _z;

  uint32_t _aFlags;
  uint32_t _zFlags;

  double _borderAccX0Y0;
  double _borderAccX0Y1;
  double _borderAccX1Y0;
  double _borderAccX1Y1;

  EdgeBuilderBase<int> _builder;
  FlattenMonoData _flattenData;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_RASTERENGINE_EDGEBUILDER_P_H
