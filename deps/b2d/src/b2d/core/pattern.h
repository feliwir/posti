// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_TEXTURE_H
#define _B2D_CORE_TEXTURE_H

// [Dependencies]
#include "../core/any.h"
#include "../core/geomtypes.h"
#include "../core/image.h"
#include "../core/matrix2d.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::PatternImpl]
// ============================================================================

class B2D_VIRTAPI PatternImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(PatternImpl)

  enum ExtraId : uint32_t {
    kExtraIdExtend = 0,
    kExtraIdMatrixType = 3
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API PatternImpl() noexcept;
  B2D_API PatternImpl(const Image& image, const IntRect& area, uint32_t extend, const Matrix2D& m, uint32_t mType) noexcept;
  B2D_API virtual ~PatternImpl() noexcept;

  static B2D_INLINE PatternImpl* new_() noexcept {
    return AnyInternal::newImplT<PatternImpl>();
  }

  static B2D_INLINE PatternImpl* new_(const Image& image, const IntRect& area, uint32_t extend, const Matrix2D& m, uint32_t mType) noexcept {
    return AnyInternal::newImplT<PatternImpl>(image, area, extend, m, mType);
  }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T>
  B2D_INLINE T* addRef() const noexcept { return ObjectImpl::addRef<T>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Image& image() const noexcept { return _image; }
  B2D_INLINE void setImage(const Image& image) noexcept { _image = image; }

  B2D_INLINE const IntRect& area() const noexcept { return _area; }
  B2D_INLINE void setArea(const IntRect& area) noexcept { _area = area; }

  B2D_INLINE const Matrix2D& matrix() const noexcept { return _matrix; }
  B2D_INLINE void setMatrix(const Matrix2D& m) noexcept { _matrix = m; }

  B2D_INLINE uint32_t extend() const noexcept { return _commonData._extra8[kExtraIdExtend]; }
  B2D_INLINE void setExtend(uint8_t extend) noexcept { _commonData._extra8[kExtraIdExtend] = uint8_t(extend); }

  B2D_INLINE uint32_t matrixType() const noexcept { return _commonData._extra8[kExtraIdMatrixType]; }
  B2D_INLINE void setMatrixType(uint8_t mType) noexcept { _commonData._extra8[kExtraIdMatrixType] = uint8_t(mType); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Image _image;                          //!< Image.
  IntRect _area;                         //!< Image area.
  Matrix2D _matrix;                      //!< Transformation matrix.
};

// ============================================================================
// [b2d::Pattern]
// ============================================================================

class Pattern : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdPattern };

  //! Pattern extend.
  enum Extend : uint32_t {
    kExtendPadXPadY         = 0,         //!< Extend [pad-x    , pad-y    ].
    kExtendRepeatXRepeatY   = 1,         //!< Extend [repeat-x , repeat-y ].
    kExtendReflectXReflectY = 2,         //!< Extend [reflect-x, reflect-y].
    kExtendPadXRepeatY      = 3,         //!< Extend [pad-x    , repeat-y ].
    kExtendPadXReflectY     = 4,         //!< Extend [pad-x    , reflect-y].
    kExtendRepeatXPadY      = 5,         //!< Extend [repeat-x , pad-y    ].
    kExtendRepeatXReflectY  = 6,         //!< Extend [repeat-x , reflect-y].
    kExtendReflectXPadY     = 7,         //!< Extend [reflect-x, pad-y    ].
    kExtendReflectXRepeatY  = 8,         //!< Extend [reflect-x, repeat-y ].

    kExtendPad              = 0,         //!< Pad extend in both directions.
    kExtendRepeat           = 1,         //!< Repeat extend in both directions (default).
    kExtendReflect          = 2,         //!< Reflect extend in both directions

    kExtendCount            = 9,         //!< Count of pattern extend modes.
    kExtendDefault          = 1
  };

  //! Pattern filter.
  enum Filter : uint32_t {
    kFilterNearest          = 0,         //!< Nearest-neighbor filter.
    kFilterBilinear         = 1,         //!< Bilinear filter (default).

    kFilterCount            = 2,         //!< Count of filters.
    kFilterDefault          = 1          //!< Default filter.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Pattern() noexcept
    : Object(none().impl()) {}

  B2D_INLINE Pattern(const Pattern& other) noexcept
    : Object(other) {}

  B2D_INLINE Pattern(Pattern&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Pattern(PatternImpl* impl) noexcept
    : Object(impl) {}

  B2D_INLINE explicit Pattern(const Image& image) noexcept
    : Pattern(&image, nullptr, kExtendDefault, nullptr) {}

  B2D_INLINE Pattern(const Image& image, uint32_t extend) noexcept
    : Pattern(&image, nullptr, extend, nullptr) {}

  B2D_INLINE Pattern(const Image& image, uint32_t extend, const Matrix2D& m) noexcept
    : Pattern(&image, nullptr, extend, &m) {}

  B2D_INLINE Pattern(const Image& image, const IntRect& area) noexcept
    : Pattern(&image, &area, kExtendDefault, nullptr) {}

  B2D_INLINE Pattern(const Image& image, const IntRect& area, uint32_t extend) noexcept
    : Pattern(&image, &area, extend, nullptr) {}

  B2D_INLINE Pattern(const Image& image, const IntRect& area, uint32_t extend, const Matrix2D& m) noexcept
    : Pattern(&image, &area, extend, &m) {}

  //! \internal
  B2D_API Pattern(const Image* image, const IntRect* area, uint32_t extend, const Matrix2D* m) noexcept;

  static B2D_INLINE const Pattern& none() noexcept { return Any::noneOf<Pattern>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = PatternImpl>
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
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_API Error reset() noexcept;

  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  //! \internal
  B2D_API Error _setupPattern(const Image* image, const IntRect* area, uint32_t extend, const Matrix2D* m) noexcept;

  B2D_INLINE Error assign(const Pattern& other) noexcept { return AnyInternal::assignImpl(this, other.impl()); }
  B2D_API Error setDeep(const Pattern& other) noexcept;

  B2D_INLINE Error setPattern(const Image& image) noexcept {
    return _setupPattern(&image, nullptr, kExtendDefault, nullptr);
  }

  B2D_INLINE Error setPattern(const Image& image, uint32_t extend) noexcept {
    return _setupPattern(&image, nullptr, extend, nullptr);
  }

  B2D_INLINE Error setPattern(const Image& image, uint32_t extend, const Matrix2D& m) noexcept {
    return _setupPattern(&image, nullptr, extend, &m);
  }

  B2D_INLINE Error setPattern(const Image& image, const IntRect& area) noexcept {
    return _setupPattern(&image, &area, kExtendDefault, nullptr);
  }

  B2D_INLINE Error setPattern(const Image& image, const IntRect& area, uint32_t extend) noexcept {
    return _setupPattern(&image, &area, extend, nullptr);
  }

  B2D_INLINE Error setPattern(const Image& image, const IntRect& area, uint32_t extend, const Matrix2D& m) noexcept {
    return _setupPattern(&image, &area, extend, &m);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const Image& image() const noexcept { return impl()->image(); }
  B2D_API Error setImage(const Image& image) noexcept;
  B2D_API Error resetImage() noexcept;

  B2D_INLINE const IntRect& area() const noexcept { return impl()->area(); }
  B2D_API Error setArea(const IntRect& area) noexcept;
  B2D_API Error resetArea() noexcept;

  B2D_INLINE uint32_t extend() const noexcept { return impl()->extend(); }
  B2D_API Error setExtend(uint32_t extend) noexcept;
  B2D_API Error resetExtend() noexcept;

  // --------------------------------------------------------------------------
  // [Operations]
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
    double params[3] = { angle, p._x, p._y };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotate(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtP, params);
  }

  B2D_INLINE Error rotateAppend(double angle, const Point& p) noexcept {
    double params[3] = { angle, p._x, p._y };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error rotateAppend(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    return _matrixOp(Matrix2D::kOpRotatePtA, params);
  }

  B2D_INLINE Error transform(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyP, &m); }
  B2D_INLINE Error transformAppend(const Matrix2D& m) noexcept { return _matrixOp(Matrix2D::kOpMultiplyA, &m); }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static B2D_INLINE uint32_t extendXFromExtend(uint32_t extend) noexcept {
    B2D_ASSERT(extend < kExtendCount);

    static constexpr uint32_t kTable = (kExtendPad     <<  0) | // [pad-x     pad-y    ]
                                       (kExtendRepeat  <<  2) | // [repeat-x  repeat-y ]
                                       (kExtendReflect <<  4) | // [reflect-x reflect-y]
                                       (kExtendPad     <<  6) | // [pad-x     repeat-y ]
                                       (kExtendPad     <<  8) | // [pad-x     reflect-y]
                                       (kExtendRepeat  << 10) | // [repeat-x  pad-y    ]
                                       (kExtendRepeat  << 12) | // [repeat-x  reflect-y]
                                       (kExtendReflect << 14) | // [reflect-x pad-y    ]
                                       (kExtendReflect << 16) ; // [reflect-x repeat-y ]
    return (kTable >> (extend * 2u)) & 0x3u;
  }

  static B2D_INLINE uint32_t extendYFromExtend(uint32_t extend) noexcept {
    B2D_ASSERT(extend < kExtendCount);

    static constexpr uint32_t kTable = (kExtendPad     <<  0) | // [pad-x     pad-y    ]
                                       (kExtendRepeat  <<  2) | // [repeat-x  repeat-y ]
                                       (kExtendReflect <<  4) | // [reflect-x reflect-y]
                                       (kExtendRepeat  <<  6) | // [pad-x     repeat-y ]
                                       (kExtendReflect <<  8) | // [pad-x     reflect-y]
                                       (kExtendPad     << 10) | // [repeat-x  pad-y    ]
                                       (kExtendReflect << 12) | // [repeat-x  reflect-y]
                                       (kExtendPad     << 14) | // [reflect-x pad-y    ]
                                       (kExtendRepeat  << 16) ; // [reflect-x repeat-y ]
    return (kTable >> (extend * 2u)) & 0x3u;
  }

  // --------------------------------------------------------------------------
  // [Equality]
  // --------------------------------------------------------------------------

  static B2D_API bool B2D_CDECL _eq(const Pattern* a, const Pattern* b) noexcept;
  B2D_INLINE bool eq(const Pattern& other) const noexcept { return _eq(this, &other); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Pattern& operator=(const Pattern& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Pattern& operator=(Pattern&& other) noexcept {
    PatternImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }

  B2D_INLINE bool operator==(const Pattern& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Pattern& other) const noexcept { return !eq(other); }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_TEXTURE_H
