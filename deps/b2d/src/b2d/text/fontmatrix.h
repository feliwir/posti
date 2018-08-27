// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTMATRIX_H
#define _B2D_TEXT_FONTMATRIX_H

// [Dependencies]
#include "../core/matrix2d.h"
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontMatrix]
// ============================================================================

//! Font matrix.
//!
//! Font matrix is a 2x2 transform. It's similar to `Matrix2D`, however, it
//! doesn't provide a translation part.
struct FontMatrix {
  //! Matrix data indexes.
  enum Index : uint32_t {
    k00 = Matrix2D::k00,
    k01 = Matrix2D::k01,
    k10 = Matrix2D::k10,
    k11 = Matrix2D::k11
  };

  inline FontMatrix() noexcept = default;

  constexpr FontMatrix(const FontMatrix& other) noexcept = default;
  constexpr FontMatrix(double a00, double a01, double a10, double a11) noexcept
    : m { a00, a01, a10, a11 } {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset matrix to `src`.
  B2D_INLINE void reset(const FontMatrix& src) noexcept { reset(src.m00(), src.m01(), src.m10(), src.m11()); }

  //! Reset matrix to `a00`, `a01`, `a10`, `a11`, `a20` and `a21`.
  B2D_INLINE void reset(double a00, double a01, double a10, double a11) noexcept {
    m[k00] = a00; m[k01] = a01;
    m[k10] = a10; m[k11] = a11;
  }

  //! Reset matrix to identity.
  B2D_INLINE void resetToIdentity() noexcept { reset(1.0, 0.0, 0.0, 1.0); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr double m00() const noexcept { return m[k00]; }
  constexpr double m01() const noexcept { return m[k01]; }
  constexpr double m10() const noexcept { return m[k10]; }
  constexpr double m11() const noexcept { return m[k11]; }

  B2D_INLINE void setM00(double v) noexcept { m[k00] = v; }
  B2D_INLINE void setM01(double v) noexcept { m[k01] = v; }
  B2D_INLINE void setM10(double v) noexcept { m[k10] = v; }
  B2D_INLINE void setM11(double v) noexcept { m[k11] = v; }

  constexpr Matrix2D toMatrix2D() const noexcept { return Matrix2D { m[0], m[1], m[2], m[3], 0.0, 0.0 }; }

  B2D_INLINE Matrix2D multiplied(const Matrix2D& right) const noexcept {
    const FontMatrix& left = *this;

    double t00 = left.m00() * right.m00() + left.m01() * right.m10();
    double t01 = left.m00() * right.m01() + left.m01() * right.m11();
    double t10 = left.m10() * right.m00() + left.m11() * right.m10();
    double t11 = left.m10() * right.m01() + left.m11() * right.m11();

    return Matrix2D { t00, t01, t10, t11, right.m20(), right.m21() };
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double m[4];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTMATRIX_H
