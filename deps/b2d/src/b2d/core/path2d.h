// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_PATH_H
#define _B2D_CORE_PATH_H

// [Dependencies]
#include "../core/any.h"
#include "../core/array.h"
#include "../core/carray.h"
#include "../core/container.h"
#include "../core/contextdefs.h"
#include "../core/geom2d.h"
#include "../core/geomtypes.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Matrix2D;
class Region;

// ============================================================================
// [b2d::Path2DInfo]
// ============================================================================

// How dirty flags work? Each `Path2DInfo` contains info-flags, each dirty
// flag has a corresponding category flag (mapped to the same value), which
// signalizes that the information about that particular category is dirty.
//
// When a `Path2D` is modified in a way that would break the information
// stored in `Path2DInfo` the `Path2DInfo::makeDirty()` function must be
// called with all flags representing the modified category set. For example
// when a quad curve is added to the path it would break bounding-box info
// and commands info, so `makeDirty(kCommandFlags | kGeometryFlags)` must be
// called; alternatively `makeDirty(kAllFlags)` would also work, but may see
// more dirty bits when necessary.
//
// Then when the `Path2DInfo` is queried the query first checks all dirty
// flags and calls respective functions to update each category separately.
// When all flags for the given category are computed a `fetch_and` operation
// is performed, which would clear all dirty flags like `kCommandDirty` and
// keep all computed flags that should be true (and zero all flags that should
// be false).
struct Path2DInfo {
  //! Information flags (what is provided and what is dirty).
  enum Flags : uint32_t {
    kCommandInfo          = 0x00000001u, //!< Path commands category.
    kCommandDirty         = 0x00000001u, //!< Path commands information is dirty.
    kCommandQuadTo        = 0x00000002u, //!< Path contains one or more QuadTo command.
    kCommandCubicTo       = 0x00000004u, //!< Path contains one or more CubicTo command.
    kCommandInvalid       = 0x00000008u, //!< Path contains one or more invalid command.
    kCommandFlags         = 0x0000000Fu, //!< All flags related to path commands information.

    kGeometryInfo         = 0x00000010u, //!< Path geometry category.
    kGeometryDirty        = 0x00000010u, //!< Path geometry information is dirty.
    kGeometryEmpty        = 0x00000020u, //!< Path geometry is empty (no vertices, empty bounding-box).
    kGeometryValid        = 0x00000040u, //!< Path geometry is valid (valid bounding-box as well).
    kGeometryFlags        = 0x00000070u, //!< All flags related to path geometry information.

    kNoFlags              = 0x00000000u,
    kAllFlags             = 0xFFFFFFFFu,

    //! Flags assigned to each created Path2D (valid flags of empty paths in general).
    kInitialFlags         = kGeometryEmpty
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2DInfo() noexcept = default;

  B2D_INLINE Path2DInfo(uint32_t flags, const Box& boundingBox) noexcept
    : _flagsNonAtomic(flags),
      _reserved(0),
      _boundingBox(boundingBox) {}

  B2D_INLINE Path2DInfo(const Path2DInfo& other) noexcept
    : _flagsNonAtomic(other._flagsNonAtomic),
      _reserved(other._reserved),
      _boundingBox(other._boundingBox) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _flagsNonAtomic = 0;
    _reserved = 0;
    _boundingBox.resetToNaN();
  }

  // NOTE: MakeDirty doesn't need an atomic access because Path2D can only be
  // modified when its reference count is equal to 1, which means it's not
  // shared across multiple threads. However, cleanDirty MUST use atomics as
  // updating the path information can happen at any time and the path could
  // be shared at this moment.

  B2D_INLINE void makeDirty(uint32_t flags) noexcept { _flagsNonAtomic |= flags; }
  B2D_INLINE void cleanDirty(uint32_t flags) noexcept { _flagsAtomic.fetch_and(flags, std::memory_order_relaxed); }

  B2D_INLINE uint32_t flags() const noexcept { return _flagsAtomic.load(std::memory_order_relaxed); }
  B2D_INLINE bool hasFlag(uint32_t flag) const noexcept { return (flags() & flag) != 0; }
  B2D_INLINE bool hasAllFlags(uint32_t f) const noexcept { return (flags() & f) == f; }

  B2D_INLINE const Box& boundingBox() const noexcept { return _boundingBox; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2DInfo& operator=(const Path2DInfo& other) noexcept {
    _flagsNonAtomic = other._flagsNonAtomic;
    _reserved = other._reserved;
    _boundingBox = other._boundingBox;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint32_t _flagsNonAtomic;            //!< Info flags with non-atomic access.
    std::atomic<uint32_t> _flagsAtomic;  //!< Info flags with atomic access.
  };

  uint32_t _reserved;                    //!< Reserved for future use.
  Box _boundingBox;                      //!< Path bounding box.
};

// ============================================================================
// [b2d::Path2DImpl]
// ============================================================================

//! Path2D (impl).
struct Path2DImpl : public SimpleImpl {
  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  static constexpr size_t sizeOf(size_t capacity) noexcept {
    return sizeof(Path2DImpl) + capacity * (sizeof(Point) + sizeof(uint8_t));
  }

  static B2D_INLINE Path2DImpl* new_(size_t capacity) noexcept {
    size_t implSize = sizeOf(capacity);
    Path2DImpl* impl = AnyInternal::allocSizedImplT<Path2DImpl>(implSize);
    return impl ? impl->_initImpl(Any::kTypeIdPath2D, capacity) : nullptr;
  }

  static B2D_INLINE Path2DImpl* newTemporary(const Support::Temporary& temporary) noexcept {
    return temporary.data<Path2DImpl>()->_initImpl(
      Any::kTypeIdPath2D | Any::kFlagIsTemporary, temporary.size());
  }

  B2D_INLINE Path2DImpl* _initImpl(uint32_t objectInfo, size_t capacity) noexcept {
    _commonData.reset(objectInfo);
    _size = 0;
    _capacity = capacity;
    _pathInfo.reset();
    _initVertexDataPtr();
    return this;
  }

  B2D_INLINE void _initVertexDataPtr() noexcept {
    _vertexData = reinterpret_cast<Point*>(Support::alignUp<uint8_t*>(_commandData + _capacity, 16));
  }

  template<typename T = Path2DImpl>
  B2D_INLINE T* addRef() noexcept { _commonData.incRef(); return this; }

  B2D_INLINE void release() noexcept {
    if (deref() && !isTemporary())
      Allocator::systemRelease(this);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE Point* vertexData() noexcept { return _vertexData; }
  B2D_INLINE const Point* vertexData() const noexcept { return _vertexData; }

  B2D_INLINE uint8_t* commandData() noexcept { return _commandData; }
  B2D_INLINE const uint8_t* commandData() const noexcept { return _commandData; }

  B2D_INLINE Path2DInfo& pathInfo() noexcept { return _pathInfo; }
  B2D_INLINE const Path2DInfo& pathInfo() const noexcept { return _pathInfo; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _capacity;                      //!< Capacity (Point & PathCmd).
  CommonData _commonData;                //!< Common data.
  size_t _size;                          //!< Number of commands and points.
  Path2DInfo _pathInfo;                  //!< Path information (possibly cached).
  Point* _vertexData;                    //!< Path vertex array (16-byte aligned).
  uint8_t _commandData[16];              //!< Path command array (plus space for aligning).
};

// ============================================================================
// [b2d::Path2D]
// ============================================================================

//! Path2D.
class Path2D : public AnyBase {
public:
  enum : uint32_t {
    kTypeId = Any::kTypeIdPath2D
  };

  //! Path2D limits.
  enum Limits : size_t {
    kInitSize = 16,
    kMaxSize = (std::numeric_limits<size_t>::max() - sizeof(Path2DImpl)) / (sizeof(Point) + sizeof(uint8_t))
  };

  //! Direction of path or a sub-path.
  enum Dir : uint32_t {
    kDirNone        = 0,                 //!< No direction specified.
    kDirCW          = 1,                 //!< Clockwise direction.
    kDirCCW         = 2                  //!< Counter-clockwise direction.
  };

  //! Path commands.
  enum Cmd : uint8_t {
    kCmdMoveTo      = 0x00u,             //!< Move-to.
    kCmdLineTo      = 0x01u,             //!< Line-to.
    kCmdQuadTo      = 0x02u,             //!< Quad-to, followed by one `kCmdControl` command.
    kCmdCubicTo     = 0x03u,             //!< Cubic-to, followed by two `kCmdControl` commands.
    kCmdClose       = 0x04u,             //!< Close.
    kCmdControl     = 0x05u,             //!< Control point, used after `kCmdQuadTo` (one) and `kCmdCubicTo` (two).
    kCmdCount       = 0x06u,             //!< Count of valid path commands.
    _kCmdInvalid    = 0xFFu,             //!< Invalid command (used only internally, never use).
    _kCmdHasInitial = 0x08u              //!< Used internally during `Path2D` manipulation.
  };

  //! Get whether the given path command `cmd` is `kCmdMoveTo` or `kCmdLineTo`.
  static constexpr bool isMoveOrLineCmd(uint32_t cmd) noexcept { return cmd >= kCmdMoveTo && cmd <= kCmdLineTo; }
  //! Get whether the given path command `cmd` is `kCmdLineto`, `kCmdQuadTo` or `kCmdCubicTo`.
  static constexpr bool isLineOrCurveCmd(uint32_t cmd) noexcept { return cmd >= kCmdLineTo && cmd <= kCmdCubicTo; }
  //! Get whether the given path command `cmd` is `kCmdQuadTo` or `kCmdCubicTo`.
  static constexpr bool isQuadOrCubicCmd(uint32_t cmd) noexcept { return cmd >= kCmdQuadTo && cmd <= kCmdCubicTo; }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2D() noexcept
    : AnyBase(none().impl()) {}

  B2D_INLINE Path2D(const Path2D& other) noexcept
    : AnyBase(other.impl()->addRef()) {}

  B2D_INLINE Path2D(Path2D&& other) noexcept
    : AnyBase(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Path2D(Path2DImpl* impl) noexcept
    : AnyBase(impl) {}

  B2D_INLINE explicit Path2D(const Support::Temporary& temporary) noexcept
    : AnyBase(Path2DImpl::newTemporary(temporary)) {}

  B2D_INLINE Path2D(const Support::Temporary& temporary, const Path2D& other) noexcept
    : Path2D(temporary) { addPath(other); }

  B2D_INLINE ~Path2D() noexcept { AnyInternal::releaseAnyImpl(impl()); }

  static B2D_INLINE const Path2D& none() noexcept { return Any::noneOf<Path2D>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = Path2DImpl>
  B2D_INLINE T* impl() const noexcept { return AnyBase::impl<T>(); }

  //! \internal
  B2D_API Error _detach() noexcept;

  //! \copydoc Doxygen::Implicit::detach().
  B2D_INLINE Error detach() noexcept { return isShared() ? _detach() : Error(kErrorOk); }

  // --------------------------------------------------------------------------
  // [Container]
  // --------------------------------------------------------------------------

  //! Get whether the path is empty.
  B2D_INLINE bool empty() const noexcept { return impl()->size() == 0; }
  //! Get path size (count of vertices used).
  B2D_INLINE size_t size() const noexcept { return impl()->size(); }
  //! Get path capacity (count of allocated vertices).
  B2D_INLINE size_t capacity() const noexcept { return impl()->capacity(); }

  B2D_API Error reserve(size_t capacity) noexcept;
  B2D_API Error compact() noexcept;

  B2D_API size_t _prepare(uint32_t op, size_t size, uint32_t dirtyFlags) noexcept;
  B2D_API size_t _add(size_t size, uint32_t dirtyFlags) noexcept;
  B2D_API size_t _updateAndGrow(uint8_t* cmdEnd, size_t n) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get path vertices array (const).
  B2D_INLINE const Point* vertexData() const noexcept { return impl()->vertexData(); }
  //! Get path commands array (const).
  B2D_INLINE const uint8_t* commandData() const noexcept { return impl()->commandData(); }

  B2D_INLINE Point vertexAt(size_t index) const noexcept {
    B2D_ASSERT(index < size());
    return vertexData()[index];
  }

  B2D_INLINE uint8_t commandAt(size_t index) const noexcept {
    B2D_ASSERT(index < size());
    return commandData()[index];
  }

  B2D_API Point lastVertex() const noexcept;

  B2D_API Error _setAt(size_t index, const Point& p) noexcept;

  B2D_INLINE Error setAt(size_t index, const Point& p) noexcept {
    B2D_ASSERT(index < size());
    return _setAt(index, p);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const Path2D& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }

  B2D_API Error setPath(const Path2D& other) noexcept;
  B2D_API Error setDeep(const Path2D& other) noexcept;

  // --------------------------------------------------------------------------
  // [SubPath]
  // --------------------------------------------------------------------------

  //! Get range of a subpath at the given `index`. (range is from the @a index
  //! to next 'move-to' command or to the end of the path).
  B2D_API Error subpathRange(size_t index, Range& dst) const noexcept;

  // --------------------------------------------------------------------------
  // [MoveTo]
  // --------------------------------------------------------------------------

  //! Move to `p0` (absolute).
  B2D_INLINE Error moveTo(const Point& p0) noexcept { return moveTo(p0.x(), p0.y()); }
  //! Move to `p0` (relative).
  B2D_INLINE Error moveToRel(const Point& p0) noexcept { return moveTo(p0.x(), p0.y()); }

  //! Move to `x0, y0` (absolute).
  B2D_API Error moveTo(double x0, double y0) noexcept;
  //! Move to `x0, y0` (relative).
  B2D_API Error moveToRel(double x0, double y0) noexcept;

  // --------------------------------------------------------------------------
  // [LineTo]
  // --------------------------------------------------------------------------

  //! Line to `p1` (absolute).
  B2D_INLINE Error lineTo(const Point& p1) noexcept { return lineTo(p1.x(), p1.y()); }
  //! Line to `p1` (relative).
  B2D_INLINE Error lineToRel(const Point& p1) noexcept { return lineToRel(p1.x(), p1.y()); }

  //! Line to `x1, y1` (absolute).
  B2D_API Error lineTo(double x1, double y1) noexcept;
  //! Line to `x1, y1` (relative).
  B2D_API Error lineToRel(double x1, double y1) noexcept;

  //! Horizontal line to `x1` (absolute).
  B2D_API Error hlineTo(double x1) noexcept;
  //! Horizontal line to `x1` (relative).
  B2D_API Error hlineToRel(double x1) noexcept;

  //! Horizontal line to `y1` (absolute).
  B2D_API Error vlineTo(double y1) noexcept;
  //! Horizontal line to `y1` (relative).
  B2D_API Error vlineToRel(double y1) noexcept;

  // --------------------------------------------------------------------------
  // [PolyTo]
  // --------------------------------------------------------------------------

  //! Polyline to `pts` (absolute).
  B2D_API Error polyTo(const Point* poly, size_t count) noexcept;
  //! Polyline to `pts` (relative).
  B2D_API Error polyToRel(const Point* poly, size_t count) noexcept;

  // --------------------------------------------------------------------------
  // [QuadTo]
  // --------------------------------------------------------------------------

  //! Quadratic curve to `p1` and `p2` (absolute).
  B2D_INLINE Error quadTo(const Point& p1, const Point& p2) noexcept { return quadTo(p1.x(), p1.y(), p2.x(), p2.y()); }
  //! Quadratic curve to `p1` and `p2` (relative).
  B2D_INLINE Error quadToRel(const Point& p1, const Point& p2) noexcept { return quadToRel(p1.x(), p1.y(), p2.x(), p2.y()); }

  //! Quadratic curve to `x1, y1` and `x2, y2` (absolute).
  B2D_API Error quadTo(double x1, double y1, double x2, double y2) noexcept;
  //! Quadratic curve to `x1, y1` and `x2, y2` (relative).
  B2D_API Error quadToRel(double x1, double y1, double x2, double y2) noexcept;

  //! Smooth quadratic curve to `p2`, calculating `p1` from last points (absolute).
  B2D_INLINE Error smoothQuadTo(const Point& p2) noexcept { return smoothQuadTo(p2.x(), p2.y()); }
  //! Smooth quadratic curve to `p2`, calculating `p1` from last points (relative).
  B2D_INLINE Error smoothQuadToRel(const Point& p2) noexcept { return smoothQuadToRel(p2.x(), p2.y()); }

  //! Smooth quadratic curve to `x2, y2`, calculating `x1, y1` from last points (absolute).
  B2D_API Error smoothQuadTo(double x2, double y2) noexcept;
  //! Smooth quadratic curve to `x2, y2`, calculating `x1, y1` from last points (relative).
  B2D_API Error smoothQuadToRel(double x2, double y2) noexcept;

  // --------------------------------------------------------------------------
  // [CubicTo]
  // --------------------------------------------------------------------------

  //! Cubic curve to `p1`, `p2`, `p3` (absolute).
  B2D_INLINE Error cubicTo(const Point& p1, const Point& p2, const Point& p3) noexcept { return cubicTo(p1.x(), p1.y(), p2.x(), p2.y(), p3.x(), p3.y()); }
  //! Cubic curve to `p1`, `p2`, `p3` (relative).
  B2D_INLINE Error cubicToRel(const Point& p1, const Point& p2, const Point& p3) noexcept { return cubicToRel(p1.x(), p1.y(), p2.x(), p2.y(), p3.x(), p3.y()); }

  //! Cubic curve to `x1, y1`, `x2, y2`, and `x3, y3` (absolute).
  B2D_API Error cubicTo(double x1, double y1, double x2, double y2, double x3, double y3) noexcept;
  //! Cubic curve to `x1, y1`, `x2, y2`, and `x3, y3` (relative).
  B2D_API Error cubicToRel(double x1, double y1, double x2, double y2, double x3, double y3) noexcept;

  //! Smooth cubic curve to `p2` and `p3`, calculating `p1` from last points (absolute).
  B2D_INLINE Error smoothCubicTo(const Point& p2, const Point& p3) noexcept { return smoothCubicTo(p2.x(), p2.y(), p3.x(), p3.y()); }
  //! Smooth cubic curve to `p2` and `p3`, calculating `p1` from last points (relative).
  B2D_INLINE Error smoothCubicToRel(const Point& p2, const Point& p3) noexcept { return smoothCubicToRel(p2.x(), p2.y(), p3.x(), p3.y()); }

  //! Smooth cubic curve to `x2, y2` and `x3, y3`, calculating `x1, y1` from last points (absolute).
  B2D_API Error smoothCubicTo(double x2, double y2, double x3, double y3) noexcept;
  //! Smooth cubic curve to `x2, y2` and `x3, y3`, calculating `x1, y1` from last points (relative).
  B2D_API Error smoothCubicToRel(double x2, double y2, double x3, double y3) noexcept;

  // --------------------------------------------------------------------------
  // [ArcTo]
  // --------------------------------------------------------------------------

  B2D_API Error arcTo(const Point& cp, const Point& rp, double start, double sweep, bool startPath = false) noexcept;
  B2D_API Error arcToRel(const Point& cp, const Point& rp, double start, double sweep, bool startPath = false) noexcept;

  B2D_API Error svgArcTo(const Point& rp, double angle, bool largeArcFlag, bool sweepFlag, const Point& p2) noexcept;
  B2D_API Error svgArcToRel(const Point& rp, double angle, bool largeArcFlag, bool sweepFlag, const Point& p2) noexcept;

  // --------------------------------------------------------------------------
  // [Close]
  // --------------------------------------------------------------------------

  B2D_API Error close() noexcept;

  // --------------------------------------------------------------------------
  // [AddBox / AddRect]
  // --------------------------------------------------------------------------

  //! Add a closed box to the path.
  B2D_API Error addBox(const IntBox& box, uint32_t dir = kDirCW) noexcept;
  //! Add a closed box to the path.
  B2D_API Error addBox(const Box& box, uint32_t dir = kDirCW) noexcept;
  //! \overload
  B2D_INLINE Error addBox(double x0, double y0, double x1, double y1, uint32_t dir = kDirCW) noexcept { return addBox(Box(x0, y0, x1, y1), dir); }

  //! Add a closed rectangle to the path.
  B2D_API Error addRect(const IntRect& rect, uint32_t dir = kDirCW) noexcept;
  //! Add a closed rectangle to the path.
  B2D_API Error addRect(const Rect& rect, uint32_t dir = kDirCW) noexcept;
  //! \overload
  B2D_INLINE Error addRect(double x, double y, double w, double h, uint32_t dir = kDirCW) noexcept { return addRect(Rect(x, y, w, h), dir); }

  // --------------------------------------------------------------------------
  // [AddArg]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Add a shape to the path.
  B2D_API Error _addArg(uint32_t id, const void* data, const Matrix2D* m, uint32_t dir) noexcept;

  //! Add an unclosed line to the path.
  B2D_INLINE Error addLine(const Line& line, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgLine, &line, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addLine(const Line& line, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgLine, &line, &m, dir); }

  //! Add an unclosed arc to the path.
  B2D_INLINE Error addArc(const Arc& arc, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArc, &arc, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addArc(const Arc& arc, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArc, &arc, &m, dir); }

  //! Add a closed circle to the path.
  B2D_INLINE Error addCircle(const Circle& circle, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgCircle, &circle, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addCircle(const Circle& circle, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgCircle, &circle, &m, dir); }

  //! Add a closed ellipse to the path.
  B2D_INLINE Error addEllipse(const Ellipse& ellipse, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgEllipse, &ellipse, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addEllipse(const Ellipse& ellipse, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgEllipse, &ellipse, &m, dir); }

  //! Add a closed rounded ractangle to the path.
  B2D_INLINE Error addRound(const Round& round, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgRound, &round, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addRound(const Round& round, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgRound, &round, &m, dir); }

  //! Add a closed chord to the path.
  B2D_INLINE Error addChord(const Arc& chord, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgChord, &chord, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addChord(const Arc& chord, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgChord, &chord, &m, dir); }

  //! Add a closed pie to the path.
  B2D_INLINE Error addPie(const Arc& pie, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgPie, &pie, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addPie(const Arc& pie, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgPie, &pie, &m, dir); }

  //! Add a closed triangle.
  B2D_INLINE Error addTriangle(const Triangle& triangle, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgTriangle, &triangle, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addTriangle(const Triangle& triangle, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgTriangle, &triangle, &m, dir); }

  //! Add a polyline.
  B2D_INLINE Error addPolyline(const CArray<Point>& poly, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgPolyline, &poly, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const CArray<Point>& poly, const Matrix2D& m, uint32_t dir) { return _addArg(kGeomArgPolyline, &poly, &m, dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const Point* poly, size_t count, uint32_t dir = kDirCW) noexcept { return addPolyline(CArray<Point>(poly, count), dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const Point* poly, size_t count, const Matrix2D& m, uint32_t dir) { return addPolyline(CArray<Point>(poly, count), m, dir); }

  //! Add a polyline.
  B2D_INLINE Error addPolyline(const CArray<IntPoint>& poly, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgIntPolyline, &poly, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const CArray<IntPoint>& poly, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgIntPolyline, &poly, &m, dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const IntPoint* poly, size_t count, uint32_t dir = kDirCW) noexcept { return addPolyline(CArray<IntPoint>(poly, count), dir); }
  //! \overload
  B2D_INLINE Error addPolyline(const IntPoint* poly, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addPolyline(CArray<IntPoint>(poly, count), m, dir); }

  //! Add a polygon.
  B2D_INLINE Error addPolygon(const CArray<Point>& poly, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgPolygon, &poly, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const CArray<Point>& poly, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgPolygon, &poly, &m, dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const Point* poly, size_t count, uint32_t dir = kDirCW) noexcept { return addPolygon(CArray<Point>(poly, count), dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const Point* poly, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addPolygon(CArray<Point>(poly, count), m, dir); }

  //! Add a polygon.
  B2D_INLINE Error addPolygon(const CArray<IntPoint>& poly, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgIntPolygon, &poly, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const CArray<IntPoint>& poly, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgIntPolygon, &poly, &m, dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const IntPoint* poly, size_t count, uint32_t dir = kDirCW) noexcept { return addPolygon(CArray<IntPoint>(poly, count), dir); }
  //! \overload
  B2D_INLINE Error addPolygon(const IntPoint* poly, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addPolygon(CArray<IntPoint>(poly, count), m, dir); }

  //! Add an array of closed boxes.
  B2D_INLINE Error addBoxArray(const CArray<Box>& array, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayBox, &array, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const CArray<Box>& array, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayBox, &array, &m, dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const Box* data, size_t count, uint32_t dir = kDirCW) noexcept { return addBoxArray(CArray<Box>(data, count), dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const Box* data, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addBoxArray(CArray<Box>(data, count), m, dir); }

  //! Add an array of closed boxes.
  B2D_INLINE Error addBoxArray(const CArray<IntBox>& array, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayIntBox, &array, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const CArray<IntBox>& array, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayIntBox, &array, &m, dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const IntBox* data, size_t count, uint32_t dir = kDirCW) noexcept { return addBoxArray(CArray<IntBox>(data, count), dir); }
  //! \overload
  B2D_INLINE Error addBoxArray(const IntBox* data, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addBoxArray(CArray<IntBox>(data, count), m, dir); }

  //! Add an array of closed rectangles.
  B2D_INLINE Error addRectArray(const CArray<Rect>& array, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayRect, &array, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const CArray<Rect>& array, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayRect, &array, &m, dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const Rect* data, size_t count, uint32_t dir = kDirCW) noexcept { return addRectArray(CArray<Rect>(data, count), dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const Rect* data, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addRectArray(CArray<Rect>(data, count), m, dir); }

  //! Add an array of closed rectangles.
  B2D_INLINE Error addRectArray(const CArray<IntRect>& array, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayIntRect, &array, nullptr, dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const CArray<IntRect>& array, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgArrayIntRect, &array, &m, dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const IntRect* data, size_t count, uint32_t dir = kDirCW) noexcept { return addRectArray(CArray<IntRect>(data, count), dir); }
  //! \overload
  B2D_INLINE Error addRectArray(const IntRect* data, size_t count, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return addRectArray(CArray<IntRect>(data, count), m, dir); }

  //! Add a closed region (converted to set of rectangles).
  B2D_INLINE Error addRegion(const Region& region, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgRegion, &region, nullptr, dir); }
  //! Add a closed region (converted to set of rectangles).
  B2D_INLINE Error addRegion(const Region& region, const Matrix2D& m, uint32_t dir = kDirCW) noexcept { return _addArg(kGeomArgRegion, &region, &m, dir); }

  // --------------------------------------------------------------------------
  // [Path]
  // --------------------------------------------------------------------------

  //! Add other `path` to the path.
  B2D_API Error addPath(const Path2D& path) noexcept;
  //! Add other `path` sliced by the given `range` to the path.
  B2D_API Error addPath(const Path2D& path, const Range& range) noexcept;

  //! Add other `path` to the path translated by `p`.
  B2D_API Error addPath(const Path2D& path, const Point& p) noexcept;
  //! Add other `path` sliced by the given `range` to the path translated by `p`.
  B2D_API Error addPath(const Path2D& path, const Range& range, const Point& p) noexcept;

  //! Add other `path` to the path transformed by `m`.
  B2D_API Error addPath(const Path2D& path, const Matrix2D& m) noexcept;
  //! Add other `path` sliced by the given `range` to the path transformed by `m`.
  B2D_API Error addPath(const Path2D& path, const Range& range, const Matrix2D& m) noexcept;

  // --------------------------------------------------------------------------
  // [ClosestVertex]
  // --------------------------------------------------------------------------

  B2D_API size_t closestVertex(const Point& p, double maxDistance, double* distance = nullptr) const noexcept;

  // --------------------------------------------------------------------------
  // [Transform]
  // --------------------------------------------------------------------------

  //! Translate the path by `p`.
  B2D_API Error translate(const Point& p) noexcept;
  //! Translate a part of the path specified by `range` by `p`.
  B2D_API Error translate(const Point& p, const Range& range) noexcept;

  //! Transform the path by matrix `m`.
  B2D_API Error transform(const Matrix2D& m) noexcept;
  //! Transform a part of the path specified by `range` by `m`.
  B2D_API Error transform(const Matrix2D& m, const Range& range) noexcept;

  //! Fit path into the given `rect`.
  B2D_API Error fitTo(const Rect& rect) noexcept;

  //! Scale each vertex by `p`.
  B2D_INLINE Error scale(const Point& p, bool keepStartPos = false) noexcept { return scale(p.x(), p.y(), keepStartPos); }
  //! Scale each vertex by `xy`.
  B2D_INLINE Error scale(double xy, bool keepStartPos = false) noexcept { return scale(xy, xy, keepStartPos); }
  //! Scale each vertex by `x` and `y`.
  B2D_API Error scale(double x, double y, bool keepStartPos = false) noexcept;

  B2D_API Error flipX(double x0, double x1) noexcept;
  B2D_API Error flipY(double y0, double y1) noexcept;

  // --------------------------------------------------------------------------
  // [Equality]
  // --------------------------------------------------------------------------

  static B2D_API bool B2D_CDECL _eq(const Path2D* a, const Path2D* b) noexcept;
  B2D_INLINE bool eq(const Path2D& other) const noexcept { return _eq(this, &other); }

  // --------------------------------------------------------------------------
  // [Path Info]
  // --------------------------------------------------------------------------

  B2D_INLINE void makeDirty(uint32_t dirtyFlags) const noexcept {
    B2D_ASSERT(!isShared());
    impl()->pathInfo().makeDirty(dirtyFlags);
  }

  //! \internal
  //!
  //! Updates `Path2DInfo` based on `infoFlags` and returns new flags.
  B2D_API uint32_t _updatePathInfo(uint32_t infoFlags) const noexcept;

  //! Makes sure that all `infoFlags` in `Path2DInfo` are updated.
  //!
  //! If all flags are already present then it's essentially a nop, however,
  //! if one or more flags are not present then `_updatePathInfo()` is called
  //! to trigger an update.
  B2D_INLINE uint32_t _queryPathInfo(uint32_t infoFlags) const noexcept {
    uint32_t currentFlags = impl()->pathInfo().flags();
    return (currentFlags & infoFlags) ? _updatePathInfo(infoFlags) : currentFlags;
  }

  B2D_INLINE const Path2DInfo& pathInfo(uint32_t infoFlags = Path2DInfo::kAllFlags) const noexcept {
    _queryPathInfo(infoFlags);
    return impl()->pathInfo();
  }

  B2D_INLINE bool hasCommandInfo() const noexcept { return !impl()->pathInfo().hasFlag(Path2DInfo::kCommandDirty); }
  B2D_INLINE bool hasGeometryInfo() const noexcept { return !impl()->pathInfo().hasFlag(Path2DInfo::kGeometryDirty); }

  //! Get whether the path contains quadratic curves.
  B2D_INLINE bool hasQuads() const noexcept {
    return (_queryPathInfo(Path2DInfo::kCommandInfo) & Path2DInfo::kCommandQuadTo) != 0;
  }

  //! Get whether the path contains cubic curves.
  B2D_INLINE bool hasCubics() const noexcept {
    return (_queryPathInfo(Path2DInfo::kCommandInfo) & Path2DInfo::kCommandCubicTo) != 0;
  }

  //! Get whether the path contains quadratic or cubic curves.
  B2D_INLINE bool hasCurves() const noexcept {
    return (_queryPathInfo(Path2DInfo::kCommandInfo) & (Path2DInfo::kCommandQuadTo | Path2DInfo::kCommandCubicTo)) != 0;
  }

  B2D_INLINE bool hasBoundingBox() const noexcept {
    return (_queryPathInfo(Path2DInfo::kGeometryInfo) & Path2DInfo::kGeometryValid) != 0;
  }

  B2D_INLINE bool getBoundingBox(Box& out) const noexcept {
    if (!(_queryPathInfo(Path2DInfo::kGeometryInfo) & Path2DInfo::kGeometryValid))
      return false;

    out.reset(impl()->pathInfo().boundingBox());
    return true;
  }

  B2D_INLINE bool getBoundingRect(Rect& out) const noexcept {
    if (!(_queryPathInfo(Path2DInfo::kGeometryInfo) & Path2DInfo::kGeometryValid))
      return false;

    out.resetToBox(impl()->pathInfo().boundingBox());
    return true;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2D& operator=(const Path2D& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Path2D& operator=(Path2D&& other) noexcept {
    Path2DImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const Path2D& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Path2D& other) const noexcept { return !eq(other); }
};

// ============================================================================
// [b2d::Path2DTmp<>]
// ============================================================================

//! Path2D (temporary).
template<size_t N = (Globals::kMemTmpSize - sizeof(Path2DImpl)) / (sizeof(Point) + sizeof(uint8_t))>
class Path2DTmp : public Path2D {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2DTmp() noexcept
    : Path2D(Support::Temporary(Support::alignUp(_storage, 16), N)) {}

  B2D_INLINE Path2DTmp(const Path2D& other) noexcept
    : Path2D(Support::Temporary(Support::alignUp(_storage, 16), N), other) {}

  B2D_INLINE Path2DTmp(const Path2DTmp& other) noexcept
    : Path2D(Support::Temporary(Support::alignUp(_storage, 16), N), other) {}

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Path2DTmp& operator=(const Path2D& other) noexcept { return static_cast<Path2DTmp&>(Path2D::operator=(other)); }
  B2D_INLINE Path2DTmp& operator=(const Path2DTmp& other) noexcept { return static_cast<Path2DTmp&>(Path2D::operator=(other)); }

  B2D_INLINE bool operator==(const Path2D& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Path2D& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Temporary storage.
  uint8_t _storage[Path2DImpl::sizeOf(N)];
};

// ============================================================================
// [b2d::Path2DAppender]
// ============================================================================

//! Low-level interface that can be used to append vertices & commands to an
//! existing path fast. The interface is designed in a way that the user using
//! it must reserve enough space and then call `append...()` functions that
//! can only be called when there is enough storage left for that command. The
//! storage requirements are specified by `start()` or by `ensure()`. The latter
//! is mostly used to reallocate the array in case more vertices are needed than
//! initially passed to `start()`.
//!
//! When designing a loop the appender can be used in the following way, where
//! `SourceT` is an iterable object that can provide us some path vertices and
//! commands:
//!
//! ```
//! template<typename SourceT>
//! Error appendToPath(Path2D& dst, const SourceT& src) noexcept {
//!   Path2DAppender appender;
//!   B2D_PROPAGATE(appender.startReplace(dst, 32));
//!
//!   while (!src.end()) {
//!     // Number of vertices required by a cubic curve is 3.
//!     B2D_PROPAGATE(appender.ensure(dst, 3));
//!
//!     switch (src.command()) {
//!       case Path2D::kCmdMoveTo : appender.appendMoveTo(src[0]); break;
//!       case Path2D::kCmdLineTo : appender.appendLineTo(src[0]); break;
//!       case Path2D::kCmdQuadTo : appender.appendQuadTo(src[0], src[1]); break;
//!       case Path2D::kCmdCubicTo: appender.appendCubicTo(src[0], src[1], src[2]); break;
//!       case Path2D::kCmdClose  : appender.appendClose(); break;
//!     }
//!
//!     src.advance();
//!   }
//!
//!   appender.end(dst);
//! }
//! ```
class Path2DAppender {
public:
  B2D_INLINE Path2DAppender() noexcept
    : _vtxPtr(nullptr) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _vtxPtr == nullptr; }
  B2D_INLINE size_t remainingSize() const noexcept { return (size_t)(_cmdEnd - _cmdPtr); }

  B2D_INLINE size_t currentIndex(const Path2D& dst) const noexcept {
    return (size_t)(_cmdPtr - dst.impl()->commandData());
  }

  B2D_INLINE uint8_t* commandPtr() const noexcept { return _cmdPtr; }
  B2D_INLINE Point* vertexPtr() const noexcept { return _vtxPtr; }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { _vtxPtr = nullptr; }

  // --------------------------------------------------------------------------
  // [Start / End]
  // --------------------------------------------------------------------------

  B2D_INLINE Error start(Path2D& dst, uint32_t op, size_t n, uint32_t dirtyFlags) noexcept {
    size_t index = dst._prepare(op, n, dirtyFlags);
    if (B2D_UNLIKELY(index == Globals::kInvalidIndex))
      return kErrorNoMemory;

    Path2DImpl* dstI = dst.impl();
    _vtxPtr = dstI->vertexData() + index;
    _cmdPtr = dstI->commandData() + index;
    _cmdEnd = dstI->commandData() + dstI->capacity();
    return kErrorOk;
  }

  B2D_INLINE Error startReplace(Path2D& dst, size_t n) noexcept {
    return start(dst, kContainerOpReplace, n, Path2DInfo::kAllFlags);
  }

  B2D_INLINE Error startAppend(Path2D& dst, size_t n, uint32_t dirtyFlags) noexcept {
    return start(dst, kContainerOpAppend, n, dirtyFlags);
  }

  B2D_INLINE void end(Path2D& dst) noexcept {
    sync(dst);
    reset();
  }

  // --------------------------------------------------------------------------
  // [Sync / Ensure]
  // --------------------------------------------------------------------------

  B2D_INLINE void sync(Path2D& dst) noexcept {
    B2D_ASSERT(!empty());

    Path2DImpl* dstI = dst.impl();
    size_t size = (size_t)(_cmdPtr - dstI->commandData());

    B2D_ASSERT(size < dstI->capacity());
    dstI->_size = size;
  }

  B2D_INLINE Error ensure(Path2D& dst, size_t n) noexcept {
    if (B2D_LIKELY(remainingSize() >= n))
      return kErrorOk;

    size_t index = dst._updateAndGrow(_cmdPtr, n);
    if (B2D_UNLIKELY(index == Globals::kInvalidIndex))
      return kErrorNoMemory;

    Path2DImpl* dstI = dst.impl();
    _vtxPtr = dstI->vertexData() + index;
    _cmdPtr = dstI->commandData() + index;
    _cmdEnd = dstI->commandData() + dstI->capacity();
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Path Construction]
  // --------------------------------------------------------------------------

  B2D_INLINE void appendMoveTo(const Point& p0) noexcept {
    appendMoveTo(p0.x(), p0.y());
  }

  B2D_INLINE void appendMoveTo(double x0, double y0) noexcept {
    B2D_ASSERT(remainingSize() >= 1);
    _cmdPtr[0] = Path2D::kCmdMoveTo;
    _vtxPtr[0].reset(x0, y0);
    _cmdPtr++;
    _vtxPtr++;
  }

  B2D_INLINE void appendLineTo(const Point& p1) noexcept {
    appendLineTo(p1.x(), p1.y());
  }

  B2D_INLINE void appendLineTo(double x1, double y1) noexcept {
    B2D_ASSERT(remainingSize() >= 1);
    _cmdPtr[0] = Path2D::kCmdLineTo;
    _vtxPtr[0].reset(x1, y1);
    _cmdPtr++;
    _vtxPtr++;
  }

  B2D_INLINE void appendQuadTo(const Point& p1, const Point& p2) noexcept {
    appendQuadTo(p1.x(), p1.y(), p2.x(), p2.y());
  }

  B2D_INLINE void appendQuadTo(double x1, double y1, double x2, double y2) noexcept {
    B2D_ASSERT(remainingSize() >= 2);
    _cmdPtr[0] = Path2D::kCmdQuadTo;
    _cmdPtr[1] = Path2D::kCmdControl;
    _vtxPtr[0].reset(x1, y1);
    _vtxPtr[1].reset(x2, y2);
    _cmdPtr += 2;
    _vtxPtr += 2;
  }

  B2D_INLINE void appendCubicTo(const Point& p1, const Point& p2, const Point& p3) noexcept {
    return appendCubicTo(p1.x(), p1.y(), p2.x(), p2.y(), p3.x(), p3.y());
  }

  B2D_INLINE void appendCubicTo(double x1, double y1, double x2, double y2, double x3, double y3) noexcept {
    B2D_ASSERT(remainingSize() >= 3);
    _cmdPtr[0] = Path2D::kCmdCubicTo;
    _cmdPtr[1] = Path2D::kCmdControl;
    _cmdPtr[2] = Path2D::kCmdControl;
    _vtxPtr[0].reset(x1, y1);
    _vtxPtr[1].reset(x2, y2);
    _vtxPtr[2].reset(x3, y3);
    _cmdPtr += 3;
    _vtxPtr += 3;
  }

  B2D_INLINE void appendVertex(uint8_t cmd, const Point& p) noexcept {
    appendVertex(cmd, p.x(), p.y());
  }

  B2D_INLINE void appendVertex(uint8_t cmd, double x, double y) noexcept {
    B2D_ASSERT(remainingSize() >= 1);
    _cmdPtr[0] = cmd;
    _vtxPtr[0].reset(x, y);
    _cmdPtr++;
    _vtxPtr++;
  }

  B2D_INLINE void appendClose() noexcept {
    B2D_ASSERT(remainingSize() >= 1);
    _cmdPtr[0] = Path2D::kCmdClose;
    _vtxPtr[0].resetToNaN();
    _cmdPtr++;
    _vtxPtr++;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point* _vtxPtr;
  uint8_t* _cmdPtr;
  uint8_t* _cmdEnd;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_PATH_H
