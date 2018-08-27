// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_MATRIX_H
#define _B2D_CORE_MATRIX_H

// [Dependencies]
#include "../core/geomtypes.h"
#include "../core/math.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Box;
struct Matrix2D;
class Path2D;

// ============================================================================
// [b2d::Matrix2D - Ops]
// ============================================================================

//! Matrix2D accelerated operations.
struct Matrix2DOps {
  void (B2D_CDECL* mapPoints[8])(const Matrix2D* self, Point* dst, const Point* src, size_t size);
};
B2D_VARAPI Matrix2DOps _bMatrix2DOps;

// ============================================================================
// [b2d::Matrix2D]
// ============================================================================

//! Matrix2D.
struct Matrix2D {
public:
  //! Matrix data indexes.
  enum Index : uint32_t {
    k00 = 0,
    k01 = 1,
    k10 = 2,
    k11 = 3,
    k20 = 4,
    k21 = 5
  };

  //! Matrix type that can be obtained by calling `Matrix2D::type()`.
  //!
  //! ```
  //!  Identity Translation  Scaling    Swap     Affine
  //!   [1  0]     [1  0]     [.  0]   [0  .]    [.  .]
  //!   [0  1]     [0  1]     [0  .]   [.  0]    [.  .]
  //!   [0  0]     [.  .]     [.  .]   [.  .]    [.  .]
  //! ```
  enum Type : uint32_t {
    kTypeIdentity    = 0,    //!< Identity matrix.
    kTypeTranslation = 1,    //!< Translation matrix - has no scaling, but contains translation part.
    kTypeScaling     = 2,    //!< Scaling matrix - has scaling and translation part.
    kTypeSwap        = 3,    //!< Swapping matrix.
    kTypeAffine      = 4,    //!< Generic affine matrix.
    kTypeInvalid     = 5,    //!< Invalid matrix (degenerated).
    kTypeCount       = 6     //!< Count of matrix types.
  };

  //! Matrix operation.
  enum Op : uint32_t {
    kOpTranslateP    = 0,    //!< Translate the matrix by \ref Point (prepend).
    kOpTranslateA    = 1,    //!< Translate the matrix by \ref Point (append).
    kOpScaleP        = 2,    //!< Scale the matrix by \ref Point (prepend).
    kOpScaleA        = 3,    //!< Scale the matrix by \ref Point (append).
    kOpSkewA         = 4,    //!< Skew the matrix by \ref Point (prepend).
    kOpSkewP         = 5,    //!< Skew the matrix by \ref Point (append).
    kOpRotateP       = 6,    //!< Rotate the matrix about [0, 0] (prepend).
    kOpRotateA       = 7,    //!< Rotate the matrix about [0, 0] (append).
    kOpRotatePtP     = 8,    //!< Rotate the matrix about aref Point (prepend).
    kOpRotatePtA     = 9,    //!< Rotate the matrix about aref Point (append).
    kOpMultiplyP     = 10,   //!< Multiply with other \ref Matrix2D (prepend).
    kOpMultiplyA     = 11,   //!< Multiply with other \ref Matrix2D (append).
    kOpCount         = 12    //!< Count of matrix operations.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new identity matrix:
  //!
  //! ```
  //!   [1 0]
  //!   [0 1]
  //!   [0 0]
  //! ```
  constexpr Matrix2D() noexcept
    : m { 1.0, 0.0,
          0.0, 1.0,
          0.0, 0.0 } {}

  //! Create a new matrix:
  //!
  //! ```
  //!   [a00 a01]
  //!   [a10 a11]
  //!   [a20 a21]
  //! ```
  constexpr Matrix2D(double a00, double a01, double a10, double a11, double a20, double a21) noexcept
    : m { a00, a01, a10, a11, a20, a21 } {}

  //! Create a copy of `src` matrix.
  constexpr Matrix2D(const Matrix2D& src) noexcept = default;

  //! Create a new uninitialized matrix.
  inline explicit Matrix2D(NoInit_) noexcept {}

  static constexpr Matrix2D identity() noexcept { return Matrix2D { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; }

  //! Create a new translation matrix.
  static constexpr Matrix2D translation(const Point& p) noexcept { return Matrix2D { 1.0, 0.0, 0.0, 1.0, p.x(), p.y() }; }
  //! Create a new translation matrix.
  static constexpr Matrix2D translation(const IntPoint& p) noexcept { return Matrix2D { 1.0, 0.0, 0.0, 1.0, double(p.x()), double(p.y()) }; }
  //! \overload
  static constexpr Matrix2D translation(double x, double y) noexcept { return Matrix2D { 1.0, 0.0, 0.0, 1.0, x, y }; }

  //! \overload
  static constexpr Matrix2D scaling(const Point& p) noexcept { return Matrix2D { p.x(), 0.0, 0.0, p.y(), 0.0, 0.0 }; }
  //! \overload
  static constexpr Matrix2D scaling(const IntPoint& p) noexcept { return Matrix2D { double(p.x()), 0.0, 0.0, double(p.y()), 0.0, 0.0 }; }
  //! Create a new scaling matrix.
  static constexpr Matrix2D scaling(double xy) noexcept { return Matrix2D { xy, 0.0, 0.0, xy, 0.0, 0.0 }; }
  //! \overload
  static constexpr Matrix2D scaling(double x, double y) noexcept { return Matrix2D { x, 0.0, 0.0, y, 0.0, 0.0 }; }

  //! Create a new rotation matrix.
  static B2D_INLINE Matrix2D rotation(double angle) noexcept {
    double aSin = std::sin(angle);
    double aCos = std::cos(angle);
    return Matrix2D(aCos, aSin, -aSin, aCos, 0.0, 0.0);
  }

  //! Create a new rotation matrix.
  static B2D_INLINE Matrix2D rotation(double angle, const Point& p) noexcept {
    double aSin = std::sin(angle);
    double aCos = std::cos(angle);
    return Matrix2D(aCos, aSin, -aSin, aCos, p.x(), p.y());
  }

  //! Create a new rotation matrix.
  static B2D_INLINE Matrix2D rotation(double angle, double x, double y) noexcept {
    double aSin = std::sin(angle);
    double aCos = std::cos(angle);
    return Matrix2D(aCos, aSin, -aSin, aCos, x, y);
  }

  //! Create a new skewing matrix.
  static B2D_INLINE Matrix2D skewing(double x, double y) noexcept {
    double xTan = std::tan(x);
    double yTan = std::tan(y);
    return Matrix2D(1.0, yTan, xTan, 1.0, 0.0, 0.0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Reset matrix to identity.
  B2D_INLINE void reset() noexcept { reset(1.0, 0.0, 0.0, 1.0, 0.0, 0.0); }
  //! Reset matrix to `src`.
  B2D_INLINE void reset(const Matrix2D& src) noexcept { reset(src.m00(), src.m01(), src.m10(), src.m11(), src.m20(), src.m21()); }
  //! Reset matrix to `a00`, `a01`, `a10`, `a11`, `a20` and `a21`.
  B2D_INLINE void reset(double a00, double a01, double a10, double a11, double a20, double a21) noexcept {
    m[k00] = a00; m[k01] = a01;
    m[k10] = a10; m[k11] = a11;
    m[k20] = a20; m[k21] = a21;
  }

  //! Reset matrix to translation.
  B2D_INLINE void resetToTranslation(const Point& p) noexcept { reset(1.0, 0.0, 0.0, 1.0, p.x(), p.y()); }
  //! Reset matrix to translation.
  B2D_INLINE void resetToTranslation(const IntPoint& p) noexcept { reset(1.0, 0.0, 0.0, 1.0, double(p.x()), double(p.y())); }
  //! Reset matrix to translation.
  B2D_INLINE void resetToTranslation(double x, double y) noexcept { reset(1.0, 0.0, 0.0, 1.0, x, y); }

  //! Reset matrix to scaling.
  B2D_INLINE void resetToScaling(const Point& p) noexcept { reset(p.x(), 0.0, 0.0, p.y(), 0.0, 0.0); }
  //! Reset matrix to scaling.
  B2D_INLINE void resetToScaling(const IntPoint& p) noexcept { reset(double(p.x()), 0.0, 0.0, double(p.y()), 0.0, 0.0); }
  //! Reset matrix to scaling.
  B2D_INLINE void resetToScaling(double xy) noexcept { reset(xy, 0.0, 0.0, xy, 0.0, 0.0); }
  //! Reset matrix to scaling.
  B2D_INLINE void resetToScaling(double x, double y) noexcept { reset(x, 0.0, 0.0, y, 0.0, 0.0); }

  //! Reset matrix to rotation.
  B2D_INLINE void resetToRotation(double angle) noexcept { resetToRotation(angle, 0.0, 0.0); }
  //! Reset matrix to rotation around a point `p`.
  B2D_INLINE void resetToRotation(double angle, const Point& p) noexcept { resetToRotation(angle, p.x(), p.y()); }
  //! Reset matrix to rotation around a point `[x, y]`.
  B2D_INLINE void resetToRotation(double angle, double x, double y) noexcept {
    double aSin = std::sin(angle);
    double aCos = std::cos(angle);
    reset(aCos, aSin, -aSin, aCos, x, y);
  }

  //! Reset matrix to skewing.
  B2D_INLINE void resetToSkewing(const Point& p) noexcept { resetToSkewing(p.x(), p.y()); }
  //! Reset matrix to skewing.
  B2D_INLINE void resetToSkewing(double x, double y) noexcept {
    double xTan = std::tan(x);
    double yTan = std::tan(y);
    reset(1.0, yTan, xTan, 1.0, 0.0, 0.0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr double m00() const noexcept { return m[k00]; }
  constexpr double m01() const noexcept { return m[k01]; }
  constexpr double m10() const noexcept { return m[k10]; }
  constexpr double m11() const noexcept { return m[k11]; }
  constexpr double m20() const noexcept { return m[k20]; }
  constexpr double m21() const noexcept { return m[k21]; }

  B2D_INLINE void setM00(double v) noexcept { m[k00] = v; }
  B2D_INLINE void setM01(double v) noexcept { m[k01] = v; }
  B2D_INLINE void setM10(double v) noexcept { m[k10] = v; }
  B2D_INLINE void setM11(double v) noexcept { m[k11] = v; }
  B2D_INLINE void setM20(double v) noexcept { m[k20] = v; }
  B2D_INLINE void setM21(double v) noexcept { m[k21] = v; }

  //! Get matrix determinant.
  constexpr double determinant() const noexcept { return m00() * m11() - m01() * m10(); }
  //! Get a translation part of the matrix.
  constexpr Point translation() const noexcept { return Point { m20(), m21() }; }

  //! Get absolute scaling.
  B2D_INLINE Point absoluteScaling() const noexcept {
    return Point(std::hypot(m00(), m10()), std::hypot(m01(), m11()));
  }

  //! Get the average scale (by X and Y).
  //!
  //! Basically used to calculate the approximation scale when decomposing
  //! curves into line segments.
  B2D_INLINE double averageScaling() const noexcept {
    double x = m00() + m10();
    double y = m01() + m11();
    return std::sqrt((x * x + y * y) * 0.5);
  }

  //! Get rotation angle.
  B2D_INLINE double rotationAngle() const noexcept {
    return std::atan2(m00(), m01());
  }

  //! \internal
  //!
  //! Get a matrix type, see \ref Matrix2D::Type.
  B2D_API uint32_t type() const noexcept;

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  B2D_API Matrix2D& _matrixOp(uint32_t op, const void* data) noexcept;

  B2D_INLINE void translate(const Point& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translateAppend(const Point& p) noexcept { translateAppend(p.x(), p.y()); }

  B2D_INLINE void translate(double x, double y) noexcept {
    m[k20] += x * m[k00] + y * m[k10];
    m[k21] += x * m[k01] + y * m[k11];
  }

  B2D_INLINE void translateAppend(double x, double y) noexcept {
    m[k20] += x;
    m[k21] += y;
  }

  B2D_INLINE void scale(const Point& p) noexcept { scale(p.x(), p.y()); }
  B2D_INLINE void scale(double xy) noexcept { scale(xy, xy); }

  B2D_INLINE void scale(double x, double y) noexcept {
    m[k00] *= x; m[k01] *= y;
    m[k10] *= x; m[k11] *= y;
  }

  B2D_INLINE void scaleAppend(const Point& p) noexcept { scaleAppend(p.x(), p.y()); }
  B2D_INLINE void scaleAppend(double xy) noexcept { scaleAppend(xy, xy); }

  B2D_INLINE void scaleAppend(double x, double y) noexcept {
    m[k00] *= x; m[k01] *= y;
    m[k10] *= x; m[k11] *= y;
    m[k20] *= x; m[k21] *= y;
  }

  B2D_INLINE void skew(const Point& p) noexcept { _matrixOp(kOpSkewP, &p); }
  B2D_INLINE void skew(double x, double y) noexcept { skew(Point { x, y }); }

  B2D_INLINE void skewAppend(const Point& p) noexcept { _matrixOp(kOpSkewA, &p); }
  B2D_INLINE void skewAppend(double x, double y) noexcept { skewAppend(Point { x, y }); }

  B2D_INLINE void rotate(double angle) noexcept { _matrixOp(kOpRotateP, &angle); }
  B2D_INLINE void rotateAppend(double angle) noexcept { _matrixOp(kOpRotateA, &angle); }

  B2D_INLINE void rotate(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    _matrixOp(kOpRotatePtP, params);
  }

  B2D_INLINE void rotate(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    _matrixOp(kOpRotatePtP, params);
  }

  B2D_INLINE void rotateAppend(double angle, const Point& p) noexcept {
    double params[3] = { angle, p.x(), p.y() };
    _matrixOp(kOpRotatePtA, params);
  }

  B2D_INLINE void rotateAppend(double angle, double x, double y) noexcept {
    double params[3] = { angle, x, y };
    _matrixOp(kOpRotatePtA, params);
  }

  B2D_INLINE void transform(const Matrix2D& m) noexcept { _matrixOp(kOpMultiplyP, &m); }
  B2D_INLINE void transformAppend(const Matrix2D& m) noexcept { _matrixOp(kOpMultiplyA, &m); }

  // --------------------------------------------------------------------------
  // [Multiply / Premultiply]
  // --------------------------------------------------------------------------

  static void multiply(Matrix2D& dst, const Matrix2D& a, const Matrix2D& b) noexcept {
    double t00 = a.m00() * b.m00() + a.m01() * b.m10();
    double t01 = a.m00() * b.m01() + a.m01() * b.m11();

    double t10 = a.m10() * b.m00() + a.m11() * b.m10();
    double t11 = a.m10() * b.m01() + a.m11() * b.m11();

    double t20 = a.m20() * b.m00() + a.m21() * b.m10() + b.m20();
    double t21 = a.m20() * b.m01() + a.m21() * b.m11() + b.m21();

    dst.reset(t00, t01, t10, t11, t20, t21);
  }

  B2D_INLINE Matrix2D& multiply(const Matrix2D& m) noexcept {
    multiply(*this, *this, m);
    return *this;
  }

  B2D_INLINE Matrix2D& premultiply(const Matrix2D& m) noexcept {
    multiply(*this, m, *this);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Invert]
  // --------------------------------------------------------------------------

  //! Invert `src` matrix and store the result in `dst`.
  //!
  //! Returns `true` if the matrix has been inverted, otherwise the return is
  //! be false and destination matrix `dst` is unchanged.
  static B2D_API bool invert(Matrix2D& dst, const Matrix2D& src) noexcept;

  //! Invert the matrix.
  //!
  //! Returns `true` if the matrix has been inverted, otherwise the return is
  //! be false and the matrix is unchanged.
  B2D_INLINE bool invert() noexcept { return invert(*this, *this); }

  // --------------------------------------------------------------------------
  // [MapPoint]
  // --------------------------------------------------------------------------

  B2D_INLINE void mapPoint(Point& pt) const noexcept {
    double x = pt.x();
    double y = pt.y();
    pt.reset(x * m00() + y * m10() + m20(), x * m01() + y * m11() + m21());
  }

  B2D_INLINE void mapPoint(Point& dst, const Point& src) const noexcept {
    double x = src.x();
    double y = src.y();
    dst.reset(x * m00() + y * m10() + m20(), x * m01() + y * m11() + m21());
  }

  // --------------------------------------------------------------------------
  // [MapVector]
  // --------------------------------------------------------------------------

  B2D_INLINE void mapVector(Point& pt) const noexcept {
    double x = pt.x();
    double y = pt.y();
    pt.reset(x * m00() + y * m10(), x * m01() + y * m11());
  }

  B2D_INLINE void mapVector(Point& dst, const Point& src) const noexcept {
    double x = src.x();
    double y = src.y();
    dst.reset(x * m00() + y * m10(), x * m01() + y * m11());
  }

  // --------------------------------------------------------------------------
  // [MapBox]
  // --------------------------------------------------------------------------

  B2D_INLINE void mapBox(Box& box) const noexcept { mapBox(box, box); }

  B2D_INLINE void mapBox(Box& dst, const Box& src) const noexcept {
    double x0a = src.x0() * m00();
    double y0a = src.y0() * m10();
    double x1a = src.x1() * m00();
    double y1a = src.y1() * m10();

    double x0b = src.x0() * m01();
    double y0b = src.y0() * m11();
    double x1b = src.x1() * m01();
    double y1b = src.y1() * m11();

    double x0 = Math::min(x0a, x1a) + Math::min(y0a, y1a) + m20();
    double y0 = Math::min(x0b, x1b) + Math::min(y0b, y1b) + m21();
    double x1 = Math::max(x0a, x1a) + Math::max(y0a, y1a) + m20();
    double y1 = Math::max(x0b, x1b) + Math::max(y0b, y1b) + m21();

    dst.reset(x0, y0, x1, y1);
  }

  // --------------------------------------------------------------------------
  // [MapPoints]
  // --------------------------------------------------------------------------

  B2D_INLINE void mapPointsByType(uint32_t mType, Point* pts, size_t count) noexcept {
    B2D_ASSERT(mType < kTypeCount);
    _bMatrix2DOps.mapPoints[mType](this, pts, pts, count);
  }

  B2D_INLINE void mapPointsByType(uint32_t mType, Point* dst, const Point* src, size_t count) const noexcept {
    B2D_ASSERT(mType < kTypeCount);
    _bMatrix2DOps.mapPoints[mType](this, dst, src, count);
  }

  // --------------------------------------------------------------------------
  // [MapPath]
  // --------------------------------------------------------------------------

  B2D_API Error mapPath(Path2D& dst, const Path2D& src) const noexcept;

  // --------------------------------------------------------------------------
  // [Equals]
  // --------------------------------------------------------------------------

  B2D_INLINE bool eq(const Matrix2D& other) const noexcept {
    return (m[0] == other.m[0]) &
           (m[1] == other.m[1]) &
           (m[2] == other.m[2]) &
           (m[3] == other.m[3]) &
           (m[4] == other.m[4]) &
           (m[5] == other.m[5]) ;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Matrix2D& operator=(const Matrix2D& other) noexcept = default;

  B2D_INLINE bool operator==(const Matrix2D& other) const noexcept { return  eq(other); }
  B2D_INLINE bool operator!=(const Matrix2D& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Matrix data, use `Index` to index a particular element of the matrix.
  double m[6];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_MATRIX_H
