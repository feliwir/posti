// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_GRADIENT_H
#define _B2D_CORE_GRADIENT_H

// [Dependencies]
#include "../core/allocator.h"
#include "../core/any.h"
#include "../core/argb.h"
#include "../core/array.h"
#include "../core/container.h"
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/pixelformat.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct GradientStop;

// ============================================================================
// [b2d::kGradientCache]
// ============================================================================

// TODO: Remove this from global namespace

//! Gradient cache constants.
enum kGradientCache {
  //! \internal
  kGradientCacheBits = 9,
  //! Count of pixels in gradient cache (do not change).
  kGradientCacheSize = 1 << kGradientCacheBits
};

// ============================================================================
// [b2d::GradientOps]
// ============================================================================

//! \internal
struct GradientOps {
  void (B2D_CDECL* interpolate32)(uint32_t* dPtr, uint32_t dSize, const GradientStop* sPtr, size_t sSize);
};

//! \internal
B2D_VARAPI GradientOps _bGradientOps;

// ============================================================================
// [b2d::GradientStop]
// ============================================================================

//! Gradient stop.
struct GradientStop {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline GradientStop() noexcept = default;
  constexpr GradientStop(const GradientStop& other) noexcept = default;

  B2D_INLINE GradientStop(double offset, const Argb32& argb32) noexcept
    : _offset(offset),
      _argb(argb32) {}

  B2D_INLINE GradientStop(double offset, const Argb64& argb64) noexcept
    : _offset(offset),
      _argb(argb64) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isValid() const noexcept {
    return Math::isUnitInterval(_offset);
  }

  B2D_INLINE void reset() noexcept {
    _offset = 0.0;
    _argb.reset();
  }

  B2D_INLINE void reset(double offset, const Argb32& argb32) noexcept { _offset = offset; _argb = argb32; }
  B2D_INLINE void reset(double offset, const Argb64& argb64) noexcept { _offset = offset; _argb = argb64; }

  B2D_INLINE double offset() const noexcept { return _offset; }
  B2D_INLINE Argb32 argb32() const noexcept { return Argb32::from64(_argb); }
  B2D_INLINE Argb64 argb64() const noexcept { return _argb; }

  B2D_INLINE void setOffset(double offset) noexcept { _offset = offset; }
  B2D_INLINE void setArgb32(const Argb32& argb32) noexcept { _argb = argb32; }
  B2D_INLINE void setArgb64(const Argb64& argb64) noexcept { _argb = argb64; }

  B2D_INLINE bool eq(const GradientStop& other) const noexcept {
    return (_offset == other._offset) &
           (_argb   == other._argb  ) ;
  }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void normalize() noexcept { _offset = Math::bound<double>(_offset, 0.0, 1.0); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE GradientStop& operator=(const GradientStop& other) noexcept = default;

  B2D_INLINE bool operator==(const GradientStop& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const GradientStop& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _offset;
  Argb64 _argb;
};
static_assert(sizeof(GradientStop) == 16, "b2d::GradientStop size must be exactly 16 bytes");

// ============================================================================
// [b2d::GradientCache]
// ============================================================================

class GradientCache {
public:
  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  static B2D_INLINE GradientCache* alloc(uint32_t size, uint32_t pixelSize) noexcept {
    GradientCache* cache = static_cast<GradientCache*>(
      Allocator::systemAlloc(sizeof(GradientCache) + size * pixelSize));

    if (B2D_UNLIKELY(!cache))
      return nullptr;

    cache->_refCount.store(1, std::memory_order_relaxed);
    cache->_size = size;

    return cache;
  }

  static B2D_INLINE void destroy(GradientCache* cache) noexcept {
    Allocator::systemRelease(cache);
  }

  // --------------------------------------------------------------------------
  // [AddRef / Release]
  // --------------------------------------------------------------------------

  B2D_INLINE GradientCache* addRef() noexcept {
    _refCount.fetch_add(1, std::memory_order_relaxed);
    return this;
  }

  B2D_INLINE void release() noexcept {
    if (_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
      destroy(this);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T = void>
  inline T* data() const noexcept { return reinterpret_cast<T*>(const_cast<GradientCache*>(this) + 1); }

  inline uint32_t size() const noexcept { return _size; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Reference count.
  mutable std::atomic<size_t> _refCount;

  //! Cache size (always set to kGradientCacheSize).
  uint32_t _size;
  uint8_t _padding[B2D_ARCH_BITS >= 64 ? 4 : 8];

  // Data[]
};

// ============================================================================
// [b2d::GradientImpl]
// ============================================================================

//! Gradient [Impl].
class B2D_VIRTAPI GradientImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(GradientImpl)

  enum ExtraId : uint32_t {
    kExtraIdExtend = 0,
    kExtraIdMatrixType = 3
  };

  static constexpr size_t sizeOf(size_t capacity) noexcept {
    return sizeof(GradientImpl) - sizeof(GradientStop) + capacity * sizeof(GradientStop);
  }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API GradientImpl(uint32_t objectInfo, size_t capacity) noexcept;
  B2D_API ~GradientImpl() noexcept;

  static B2D_INLINE GradientImpl* new_(uint32_t objectInfo, size_t capacity) noexcept {
    size_t implSize = sizeOf(capacity);
    return AnyInternal::newSizedImplT<GradientImpl>(implSize, objectInfo, capacity);
  }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = GradientImpl>
  B2D_INLINE T* addRef() noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Cache]
  // --------------------------------------------------------------------------

  B2D_INLINE GradientCache* cache32() const noexcept { return _cache32.load(std::memory_order_relaxed); }

  B2D_API GradientCache* _ensureCache32() const noexcept;
  B2D_INLINE GradientCache* ensureCache32() const noexcept {
    GradientCache* cache = cache32();
    return cache ? cache : _ensureCache32();
  }

  B2D_INLINE void _destroyCache() noexcept {
    GradientCache* cache = _cache32.exchange(nullptr);
    if (cache)
      cache->release();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE uint32_t type() const noexcept { return typeId() - Any::_kTypeIdGradientFirst; }

  B2D_INLINE uint32_t extend() const noexcept { return _commonData._extra8[kExtraIdExtend]; }
  B2D_INLINE void setExtend(uint32_t extend) noexcept {_commonData._extra8[kExtraIdExtend] = uint8_t(extend); }

  B2D_INLINE const Matrix2D& matrix() const noexcept { return _matrix; }
  B2D_INLINE void setMatrix(const Matrix2D& m) noexcept { _matrix = m; }

  B2D_INLINE uint32_t matrixType() const noexcept { return _commonData._extra8[kExtraIdMatrixType]; }
  B2D_INLINE void setMatrixType(uint32_t matrixType) noexcept { _commonData._extra8[kExtraIdMatrixType] = uint8_t(matrixType); }

  B2D_INLINE double scalar(uint32_t scalarId) const noexcept { return _scalar[scalarId]; }
  B2D_INLINE void setScalar(uint32_t scalarId, double value) noexcept { _scalar[scalarId] = value; }

  B2D_INLINE Point vertex(uint32_t vertexId) const noexcept { return _vertex[vertexId]; }
  B2D_INLINE void setVertex(uint32_t vertexId, const Point& vertex) noexcept { _vertex[vertexId].reset(vertex); }

  B2D_INLINE GradientStop* stops() noexcept { return _stops; }
  B2D_INLINE const GradientStop* stops() const noexcept { return _stops; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _size;                          //!< Stops count.
  size_t _capacity;                      //!< Stops capacity.
  Matrix2D _matrix;                      //!< Matrix.

  //! Gradient cache (32-bit).
  mutable std::atomic<GradientCache*> _cache32;

  //! Gradient scalar/vertex values.
  //!
  //! See `Gradient::ScalarId` and `Gradient::VertexId`.
  union {
    double _scalar[6];
    Point _vertex[3];
  };

  //! Gradient stops.
  GradientStop _stops[1];
};

// ============================================================================
// [b2d::Gradient]
// ============================================================================

//! Gradient base class, inherited by `LinearGradient`, `GradialGradient`, and
//! `ConicalGradient`.
class Gradient : public Object {
public:
  //! Gradient limits.
  enum Limits : size_t {
    kInitSize = 4,
    kMaxSize = (std::numeric_limits<size_t>::max() - sizeof(GradientImpl)) / sizeof(GradientStop)
  };

  //! Gradient type.
  enum Type : uint32_t {
    kTypeLinear          = 0,            //!< Linear gradient type.
    kTypeRadial          = 1,            //!< Radial gradient type.
    kTypeConical         = 2,            //!< Conical gradient type.

    kTypeCount           = 3             //!< Count of gradient types.
  };

  static constexpr uint32_t gradientTypeFromTypeId(uint32_t typeId) noexcept { return typeId - Any::_kTypeIdGradientFirst; }
  static constexpr uint32_t typeIdFromGradientType(uint32_t grType) noexcept { return grType + Any::_kTypeIdGradientFirst; }

  //! Gradient extend.
  enum Extend : uint32_t {
    kExtendPad           = 0,            //!< Pad extend (default).
    kExtendRepeat        = 1,            //!< Repeat extend.
    kExtendReflect       = 2,            //!< Reflect extend.

    kExtendCount         = 3,            //!< Count of extend options.
    kExtendDefault       = 0             //!< Default extend option.
  };

  //! Gradient filter.
  enum Filter : uint32_t {
    kFilterNearest       = 0,            //!< Nearest neighbor (default).

    kFilterCount         = 1,            //!< Count of filters.
    kFilterDefault       = 0             //!< Default filter.
  };

  //! Scalar data index.
  //!
  //! Linear gradient:
  //!
  //! - `_scalar[0]` - x0.
  //! - `_scalar[1]` - y0.
  //! - `_scalar[2]` - x1.
  //! - `_scalar[3]` - y1.
  //!
  //! - `_vertex[0]` - [x0, y0].
  //! - `_vertex[1]` - [x1, y1].
  //!
  //! Radial gradient:
  //!
  //! - `_scalar[0]` - cx.
  //! - `_scalar[1]` - cy.
  //! - `_scalar[2]` - fx.
  //! - `_scalar[3]` - fy.
  //! - `_scalar[4]` - cr.
  //!
  //! - `_vertex[0]` - [cx, cy].
  //! - `_vertex[1]` - [fx, fy].
  //!
  //! Conical gradient:
  //!
  //! - `_scalar[0]` - cx.
  //! - `_scalar[1]` - cy.
  //! - `_scalar[2]` - angle.
  //!
  //! - `_vertex[0]` - [cx, cy].
  enum ScalarId : uint32_t {
    kScalarIdLinearX0 = 0,
    kScalarIdLinearY0 = 1,
    kScalarIdLinearX1 = 2,
    kScalarIdLinearY1 = 3,

    kScalarIdRadialCx = 0,
    kScalarIdRadialCy = 1,
    kScalarIdRadialFx = 2,
    kScalarIdRadialFy = 3,
    kScalarIdRadialCr = 4,

    kScalarIdConicalCx = 0,
    kScalarIdConicalCy = 1,
    kScalarIdConicalAngle = 2,

    kScalarIdCount = 6
  };

  //! Vertex data index.
  enum VertexId : uint32_t {
    kVertexIdLinearX0Y0  = kScalarIdLinearX0  / 2,
    kVertexIdLinearX1Y1  = kScalarIdLinearX1  / 2,
    kVertexIdRadialCxCy  = kScalarIdRadialCx  / 2,
    kVertexIdRadialFxFy  = kScalarIdRadialFx  / 2,
    kVertexIdConicalCxCy = kScalarIdConicalCx / 2,
    kVertexIdCount = 3
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Gradient(const Gradient& other) noexcept
    : Object(other) {}

  B2D_INLINE Gradient(Gradient&& other) noexcept
    : Object(other) {}

  B2D_INLINE explicit Gradient(GradientImpl* impl) noexcept
    : Object(impl) {}

  //! Create a gradient of type-id `typeId`.
  B2D_INLINE Gradient(uint32_t typeId) noexcept
    : Object(Any::noneByTypeId<Gradient>(typeId).impl()) {}

  //! Create a gradient of type-id `typeId` having scalar values initialized to `v0`, `v1`, `v2`, `v3`, `v4`, and `v5`.
  B2D_API Gradient(uint32_t typeId, double v0, double v1, double v2, double v3, double v4 = 0.0, double v5 = 0.0) noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = GradientImpl>
  B2D_INLINE T* impl() const noexcept { return Object::impl<T>(); }

  // --------------------------------------------------------------------------
  // [Sharing]
  // --------------------------------------------------------------------------

  //! \internal
  B2D_API Error _detach() noexcept;

  //! \copydoc Doxygen::Implicit::detach().
  B2D_INLINE Error detach() noexcept {
    return isShared() ? _detach() : Error(kErrorOk);
  }

  // --------------------------------------------------------------------------
  // [Container]
  // --------------------------------------------------------------------------

  //! Reserve capacity for at least `n` stops.
  B2D_API Error reserve(size_t n) noexcept;
  //! Compact.
  B2D_API Error compact() noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_API Error reset(uint32_t resetPolicy = Globals::kResetSoft) noexcept;

  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  B2D_API Error _setDeep(const Gradient& other) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get gradient type, see \ref Gradient::Type.
  //!
  //! NOTE: Gradient type is an immutable property. It's set by either \ref
  //! LinearGradient or \ref RadialGradient constructors and never changed.
  B2D_INLINE uint32_t type() const noexcept { return impl()->type(); }
  //! Get gradient mode, see \ref Gradient::Extend.
  B2D_INLINE uint32_t extend() const noexcept { return impl()->extend(); }

  //! \internal
  B2D_INLINE double scalar(uint32_t scalarId) const noexcept { return impl()->scalar(scalarId); }
  //! \internal
  B2D_INLINE Point vertex(uint32_t vertexId) const noexcept { return impl()->vertex(vertexId); }

  //! Set gradient mode, see \ref Gradient::Extend.
  B2D_API Error setExtend(uint32_t mode) noexcept;

  //! Set a single gradient scalar value at `index` to `v`.
  B2D_API Error setScalar(uint32_t scalarId, double v) noexcept;
  //! Set all gradient scalar values to `v0`, `v1`, `v2`, `v3`, `v4`, and `v5`.
  B2D_API Error setScalars(double v0, double v1, double v2, double v3, double v4 = 0.0, double v5 = 0.0) noexcept;

  //! Set gradient vertex at `vertexId` to `vertex`.
  B2D_INLINE Error setVertex(uint32_t vertexId, const Point& vertex) noexcept { return setVertex(vertexId, vertex.x(), vertex.y()); }
  //! Set gradient vertex at `vertexId` to `x` and `y`.
  B2D_API Error setVertex(uint32_t vertexId, double x, double y) noexcept;


  // --------------------------------------------------------------------------
  // [Stops]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return impl()->size() != 0; }
  B2D_INLINE size_t stopCount() const noexcept { return impl()->size(); }

  B2D_INLINE const GradientStop* stops() const noexcept { return impl()->_stops; }
  B2D_INLINE const GradientStop& stopAt(size_t i) const noexcept {
    B2D_ASSERT(i < impl()->size());
    return impl()->_stops[i];
  }

  B2D_API Error resetStops() noexcept;

  B2D_API Error setStops(const Array<GradientStop>& stops) noexcept;
  B2D_API Error setStops(const GradientStop* stops, size_t size) noexcept;

  B2D_API Error setStop(double offset, Argb32 argb32, bool replace = true) noexcept;
  B2D_API Error setStop(double offset, Argb64 argb64, bool replace = true) noexcept;
  B2D_API Error setStop(const GradientStop& stop, bool replace = true) noexcept;

  B2D_INLINE Error addStop(double offset, const Argb32& argb32) noexcept {
    return setStop(offset, argb32, false);
  }

  B2D_INLINE Error addStop(double offset, const Argb64& argb64) noexcept {
    return setStop(offset, argb64, false);
  }

  B2D_INLINE Error addStop(const GradientStop& stop) noexcept {
    return setStop(stop, false);
  }

  B2D_API Error removeStop(double offset, bool all = true) noexcept;
  B2D_API Error removeStop(const GradientStop& stop, bool all = true) noexcept;

  B2D_API Error removeIndex(size_t index) noexcept;
  B2D_API Error removeRange(const Range& range) noexcept;
  B2D_API Error removeRange(double tMin, double tMax) noexcept;

  B2D_API size_t indexOf(double offset) const noexcept;
  B2D_API size_t indexOf(const GradientStop& stop) const noexcept;

  // --------------------------------------------------------------------------
  // [Matrix]
  // --------------------------------------------------------------------------

  B2D_INLINE const Matrix2D& matrix() const noexcept { return impl()->matrix(); }
  B2D_INLINE uint32_t matrixType() const noexcept { return impl()->matrixType(); }
  B2D_INLINE bool hasMatrix() const noexcept { return matrixType() != Matrix2D::kTypeIdentity; }

  B2D_API Error setMatrix(const Matrix2D& m) noexcept;
  B2D_API Error resetMatrix() noexcept;
  B2D_API Error _matrixOp(uint32_t op, const void* params) noexcept;

  B2D_INLINE Error translate(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpTranslateP, &p); }
  B2D_INLINE Error translate(double x, double y) noexcept { return translate(Point(x, y)); }

  B2D_INLINE Error translateAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpTranslateA, &p); }
  B2D_INLINE Error translateAppend(double x, double y) noexcept { return translateAppend(Point(x, y)); }

  B2D_INLINE Error scale(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpScaleP, &p); }
  B2D_INLINE Error scale(double xy) noexcept { return scale(Point(xy, xy)); }
  B2D_INLINE Error scale(double x, double y) noexcept { return scale(Point(x, y)); }

  B2D_INLINE Error scaleAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpScaleA, &p); }
  B2D_INLINE Error scaleAppend(double xy) noexcept { return scaleAppend(Point(xy, xy)); }
  B2D_INLINE Error scaleAppend(double x, double y) noexcept { return scaleAppend(Point(x, y)); }

  B2D_INLINE Error skew(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpSkewP, &p); }
  B2D_INLINE Error skew(double x, double y) noexcept { return skew(Point(x, y)); }

  B2D_INLINE Error skewAppend(const Point& p) noexcept { return _matrixOp(Matrix2D::kOpSkewA, &p); }
  B2D_INLINE Error skewAppend(double x, double y) noexcept { return skewAppend(Point(x, y)); }

  B2D_INLINE Error rotate(double angle) noexcept { return _matrixOp(Matrix2D::kOpRotateP, &angle); }
  B2D_INLINE Error rotateAppend(double angle) noexcept { return _matrixOp(Matrix2D::kOpRotateA, &angle); }

  B2D_INLINE Error rotate(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotate(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotateAppend(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error rotateAppend(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error transform(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyP, &m); }
  B2D_INLINE Error transformAppend(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyA, &m); }

  // --------------------------------------------------------------------------
  // [Equality]
  // --------------------------------------------------------------------------

  static B2D_API bool B2D_CDECL _eq(const Gradient* a, const Gradient* b) noexcept;
  B2D_INLINE bool eq(const Gradient& other) const noexcept { return _eq(this, &other); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

protected:
  B2D_INLINE Gradient& operator=(const Gradient& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }


public:
  B2D_INLINE bool operator==(const Gradient& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Gradient& other) const noexcept { return !eq(other); }
};

// ============================================================================
// [b2d::LinearGradient]
// ============================================================================

//! Linear gradient.
class LinearGradient : public Gradient {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdLinearGradient };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE LinearGradient() noexcept
    : Gradient(none().impl()) {}

  B2D_INLINE LinearGradient(const LinearGradient& other) noexcept
    : Gradient(other) {}

  B2D_INLINE LinearGradient(LinearGradient&& other) noexcept
    : Gradient(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE LinearGradient(double x0, double y0, double x1, double y1) noexcept
    : Gradient(Any::kTypeIdLinearGradient, x0, y0, x1, y1) {}

  B2D_INLINE LinearGradient(const Point& p0, const Point& p1) noexcept
    : Gradient(Any::kTypeIdLinearGradient, p0.x(), p0.y(), p1.x(), p1.y()) {}

  B2D_INLINE explicit LinearGradient(GradientImpl* d) noexcept
    : Gradient(d) {}

  static B2D_INLINE const LinearGradient& none() noexcept { return Any::noneOf<LinearGradient>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const LinearGradient& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }
  B2D_INLINE Error setDeep(const LinearGradient& other) noexcept { return _setDeep(other); }

  B2D_INLINE double x0() const noexcept { return scalar(kScalarIdLinearX0); }
  B2D_INLINE double y0() const noexcept { return scalar(kScalarIdLinearY0); }
  B2D_INLINE double x1() const noexcept { return scalar(kScalarIdLinearX1); }
  B2D_INLINE double y1() const noexcept { return scalar(kScalarIdLinearY1); }

  B2D_INLINE Point start() const noexcept { return vertex(kVertexIdLinearX0Y0); }
  B2D_INLINE Point end() const noexcept { return vertex(kVertexIdLinearX1Y1); }

  B2D_INLINE Error setX0(double x0) noexcept { return setScalar(kScalarIdLinearX0, x0); }
  B2D_INLINE Error setY0(double y0) noexcept { return setScalar(kScalarIdLinearY0, y0); }
  B2D_INLINE Error setX1(double x1) noexcept { return setScalar(kScalarIdLinearX1, x1); }
  B2D_INLINE Error setY1(double y1) noexcept { return setScalar(kScalarIdLinearY1, y1); }

  B2D_INLINE Error setStart(const Point& p0) noexcept { return setVertex(kVertexIdLinearX0Y0, p0); }
  B2D_INLINE Error setStart(double x0, double y0) noexcept { return setVertex(kVertexIdLinearX0Y0, x0, y0); }

  B2D_INLINE Error setEnd(const Point& p1) noexcept { return setVertex(kVertexIdLinearX1Y1, p1); }
  B2D_INLINE Error setEnd(double x1, double y1) noexcept { return setVertex(kVertexIdLinearX1Y1, x1, y1); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE LinearGradient& operator=(const LinearGradient& other) noexcept {
    return static_cast<LinearGradient&>(Gradient::operator=(other));
  }

  B2D_INLINE LinearGradient& operator=(LinearGradient&& other) noexcept {
    GradientImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

// ============================================================================
// [b2d::RadialGradient]
// ============================================================================

//! Radial gradient.
class RadialGradient : public Gradient {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdRadialGradient };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE RadialGradient() noexcept
    : Gradient(none().impl()) {}

  B2D_INLINE RadialGradient(const RadialGradient& other) noexcept
    : Gradient(other) {}

  B2D_INLINE RadialGradient(RadialGradient&& other) noexcept
    : Gradient(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE RadialGradient(double cx, double cy, double fx, double fy, double r) noexcept
    : Gradient(Any::kTypeIdRadialGradient, cx, cy, fx, fy, r, 0.0) {}

  B2D_INLINE RadialGradient(const Point& cp, const Point& fp, double r) noexcept
    : Gradient(Any::kTypeIdRadialGradient, cp.x(), cp.y(), fp.x(), fp.y(), r, 0.0) {}

  B2D_INLINE explicit RadialGradient(GradientImpl* d) noexcept
    : Gradient(d) {}

  static B2D_INLINE const RadialGradient& none() noexcept { return Any::noneOf<RadialGradient>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const RadialGradient& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }
  B2D_INLINE Error setDeep(const RadialGradient& other) noexcept { return _setDeep(other); }

  B2D_INLINE double cx() const noexcept { return scalar(kScalarIdRadialCx); }
  B2D_INLINE double cy() const noexcept { return scalar(kScalarIdRadialCy); }
  B2D_INLINE double fx() const noexcept { return scalar(kScalarIdRadialFx); }
  B2D_INLINE double fy() const noexcept { return scalar(kScalarIdRadialFy); }
  B2D_INLINE double cr() const noexcept { return scalar(kScalarIdRadialCr); }

  B2D_INLINE Error setCx(double cx) noexcept { return setScalar(kScalarIdRadialCx, cx); }
  B2D_INLINE Error setCy(double cy) noexcept { return setScalar(kScalarIdRadialCy, cy); }
  B2D_INLINE Error setFx(double fx) noexcept { return setScalar(kScalarIdRadialFx, fx); }
  B2D_INLINE Error setFy(double fy) noexcept { return setScalar(kScalarIdRadialFy, fy); }
  B2D_INLINE Error setCr(double cr) noexcept { return setScalar(kScalarIdRadialCr, cr); }

  B2D_INLINE Point center() const noexcept { return vertex(kVertexIdRadialCxCy); }
  B2D_INLINE Point focal() const noexcept { return vertex(kVertexIdRadialFxFy); }

  B2D_INLINE Error setCenter(const Point& cp) noexcept { return setVertex(kVertexIdRadialCxCy, cp); }
  B2D_INLINE Error setCenter(double cx, double cy) noexcept { return setVertex(kVertexIdRadialCxCy, cx, cy); }

  B2D_INLINE Error setFocal(const Point& fp) noexcept { return setVertex(kVertexIdRadialFxFy, fp); }
  B2D_INLINE Error setFocal(double fx, double fy) noexcept { return setVertex(kVertexIdRadialFxFy, fx, fy); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE RadialGradient& operator=(const RadialGradient& other) noexcept {
    return static_cast<RadialGradient&>(Gradient::operator=(other));
  }

  B2D_INLINE RadialGradient& operator=(RadialGradient&& other) noexcept {
    GradientImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

// ============================================================================
// [b2d::ConicalGradient]
// ============================================================================

//! Conical gradient.
class ConicalGradient : public Gradient {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdConicalGradient };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ConicalGradient() noexcept
    : Gradient(none().impl()) {}

  B2D_INLINE ConicalGradient(const ConicalGradient& other) noexcept
    : Gradient(other) {}

  B2D_INLINE ConicalGradient(ConicalGradient&& other) noexcept
    : Gradient(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE ConicalGradient(double cx, double cy, double angle) noexcept
    : Gradient(Any::kTypeIdConicalGradient, cx, cy, angle, 0.0) {}

  B2D_INLINE ConicalGradient(const Point& cp, double angle) noexcept
    : Gradient(Any::kTypeIdConicalGradient, cp.x(), cp.y(), angle, 0.0) {}

  B2D_INLINE explicit ConicalGradient(GradientImpl* d) noexcept
    : Gradient(d) {}

  static B2D_INLINE const ConicalGradient& none() noexcept { return Any::noneOf<ConicalGradient>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Error assign(const ConicalGradient& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }
  B2D_INLINE Error setDeep(const ConicalGradient& other) noexcept { return _setDeep(other); }

  B2D_INLINE double cx() const noexcept { return scalar(kScalarIdConicalCx); }
  B2D_INLINE double cy() const noexcept { return scalar(kScalarIdConicalCy); }
  B2D_INLINE double angle() const noexcept { return scalar(kScalarIdConicalAngle); }

  B2D_INLINE Error setCx(double cx) noexcept { return setScalar(kScalarIdConicalCx, cx); }
  B2D_INLINE Error setCy(double cy) noexcept { return setScalar(kScalarIdConicalCy, cy); }
  B2D_INLINE Error setAngle(double angle) noexcept { return setScalar(kScalarIdConicalAngle, angle); }

  B2D_INLINE Point center() const noexcept { return vertex(kVertexIdConicalCxCy); }
  B2D_INLINE Error setCenter(const Point& cp) noexcept { return setVertex(kVertexIdConicalCxCy, cp); }
  B2D_INLINE Error setCenter(double cx, double cy) noexcept { return setVertex(kVertexIdConicalCxCy, cx, cy); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE ConicalGradient& operator=(const ConicalGradient& other) noexcept {
    return static_cast<ConicalGradient&>(Gradient::operator=(other));
  }

  B2D_INLINE ConicalGradient& operator=(ConicalGradient&& other) noexcept {
    GradientImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_GRADIENT_H
