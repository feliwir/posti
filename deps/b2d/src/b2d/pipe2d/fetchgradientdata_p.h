// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHGRADIENTDATA_P_H
#define _B2D_PIPE2D_FETCHGRADIENTDATA_P_H

// [Dependencies]
#include "./tables_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchGradientData]
// ============================================================================

//! Gradient fetch data.
struct alignas(16) FetchGradientData {
  // --------------------------------------------------------------------------
  // [Init - Any]
  // --------------------------------------------------------------------------

  inline void initTable(const void* data, uint32_t size) noexcept {
    table.data = data;
    table.size = size;
    table.prec = Support::ctz(size);
  }

  // --------------------------------------------------------------------------
  // [Init - Linear]
  // --------------------------------------------------------------------------

  uint32_t initLinear(uint32_t extend, Point p0, Point p1, const Matrix2D& m, const Matrix2D& mInv) noexcept {
    uint32_t size = table.size;
    B2D_ASSERT(size > 0);
    B2D_ASSERT(extend < Gradient::kExtendCount);

    bool isPad     = extend == Gradient::kExtendPad;
    bool isReflect = extend == Gradient::kExtendReflect;

    // Distance between [x0, y0] and [x1, y1], before transform.
    double ax = p1.x() - p0.x();
    double ay = p1.y() - p0.y();
    double dist = ax * ax + ay * ay;

    // Invert origin and move it to the center of the pixel.
    Point o;
    m.mapPoint(o, p0);

    o._x = 0.5 - o.x();
    o._y = 0.5 - o.y();

    double dt = ax * mInv.m00() + ay * mInv.m01();
    double dy = ax * mInv.m10() + ay * mInv.m11();

    double scale = double(int64_t(uint64_t(size) << 32)) / dist;
    double offset = o.x() * dt + o.y() * dy;

    dt *= scale;
    dy *= scale;
    offset *= scale;

    linear.dy.i64 = Math::i64floor(dy);
    linear.dt.i64 = Math::i64floor(dt);
    linear.dt2.u64 = linear.dt.u64 << 1;
    linear.pt[0].i64 = Math::i64floor(offset);
    linear.pt[1].u64 = linear.pt[0].u64 + linear.dt.u64;

    uint32_t rorSize = isReflect ? size * 2u : size;
    linear.rep.u32Hi = isPad     ? uint32_t(0xFFFFFFFFu) : uint32_t(rorSize - 1u);
    linear.rep.u32Lo = 0xFFFFFFFFu;
    linear.msk.u     = isPad     ? (size - 1u) * 0x00010001u : (size * 2u - 1u) * 0x00010001u;

    return isPad ? kFetchTypeGradientLinearPad : kFetchTypeGradientLinearRoR;
  }

  // --------------------------------------------------------------------------
  // [Init - Radial]
  // --------------------------------------------------------------------------

  // The radial gradient uses the following equation:
  //
  //    b = x * fx + y * fy
  //    d = x^2 * (r^2 - fy^2) + y^2 * (r^2 - fx^2) + x*y * (2*fx*fy)
  //
  //    pos = ((b + sqrt(d))) * scale)
  //
  // Simplified to:
  //
  //    C1 = r^2 - fy^2
  //    C2 = r^2 - fx^2
  //    C3 = 2 * fx * fy
  //
  //    b = x*fx + y*fy
  //    d = x^2 * C1 + y^2 * C2 + x*y * C3
  //
  //    pos = ((b + sqrt(d))) * scale)
  //
  // Radial gradient function can be defined as follows:
  //
  //    D = C1*(x^2) + C2*(y^2) + C3*(x*y)
  //
  // Which could be rewritten to:
  //
  //    D = D1 + D2 + D3
  //
  //    Where: D1 = C1*(x^2)
  //           D2 = C2*(y^2)
  //           D3 = C3*(x*y)
  //
  // The variables `x` and `y` increase linearly, thus we can use multiple
  // differentiation to get delta (d) and delta-of-delta (dd).
  //
  // Deltas for `C*(x^2)` at `t`:
  //
  //   C*x*x: 1st delta `d`  at step `t`: C*(t^2) + 2*C*x
  //   C*x*x: 2nd delta `dd` at step `t`: 2*C *t^2
  //
  //   ( Hint, use Mathematica DifferenceDelta[x*x*C, {x, 1, t}] )
  //
  // Deltas for `C*(x*y)` at `t`:
  //
  //   C*x*y: 1st delta `d`  at step `tx/ty`: C*x*ty + C*y*tx + C*tx*ty
  //   C*x*y: 2nd delta `dd` at step `tx/ty`: 2*C * tx*ty
  uint32_t initRadial(uint32_t extend, Point c, Point f, double r, const Matrix2D& m, const Matrix2D& mInv) noexcept {
    uint32_t tableSize = table.size;

    B2D_ASSERT(extend < Gradient::kExtendCount);
    B2D_ASSERT(tableSize != 0);

    Point fOrig = f;
    f -= c;

    double fxfx = f.x() * f.x();
    double fyfy = f.y() * f.y();

    double rr = r * r;
    double dd = rr - fxfx - fyfy;

    // If the focal point is near the border we move it slightly to prevent
    // division by zero. This idea comes from AntiGrain library.
    if (Math::isNearZero(dd)) {
      if (!Math::isNearZero(f.x())) f._x += (f.x() < 0.0) ? 0.5 : -0.5;
      if (!Math::isNearZero(f.y())) f._y += (f.y() < 0.0) ? 0.5 : -0.5;

      fxfx = f.x() * f.x();
      fyfy = f.y() * f.y();
      dd = rr - fxfx - fyfy;
    }

    double scale = double(int(tableSize)) / dd;
    double ax = rr - fyfy;
    double ay = rr - fxfx;

    radial.ax = ax;
    radial.ay = ay;
    radial.fx = f.x();
    radial.fy = f.y();

    double xx = mInv.m00();
    double xy = mInv.m01();
    double yx = mInv.m10();
    double yy = mInv.m11();

    radial.xx = xx;
    radial.xy = xy;
    radial.yx = yx;
    radial.yy = yy;
    radial.ox = (mInv.m20() - fOrig.x()) + 0.5 * (xx + yx);
    radial.oy = (mInv.m21() - fOrig.y()) + 0.5 * (xy + yy);

    double ax_xx = ax * xx;
    double ay_xy = ay * xy;
    double fx_xx = f.x() * xx;
    double fy_xy = f.y() * xy;

    radial.dd = ax_xx * xx + ay_xy * xy + 2.0 * (fx_xx * fy_xy);
    radial.bd = fx_xx + fy_xy;

    radial.ddx = 2.0 * (ax_xx + fy_xy * f.x());
    radial.ddy = 2.0 * (ay_xy + fx_xx * f.y());

    radial.ddd = 2.0 * radial.dd;
    radial.scale = scale;

    bool isReflecting = (extend == Gradient::kExtendReflect);
    radial.maxi = isReflecting ? int(tableSize * 2 - 1)
                               : int(tableSize     - 1);

    return kFetchTypeGradientRadialPad + extend;
  }

  // --------------------------------------------------------------------------
  // [Init - Conical]
  // --------------------------------------------------------------------------

  uint32_t initConical(uint32_t extend, Point c, double angle, const Matrix2D& m, const Matrix2D& mInv) noexcept {
    uint32_t precision = table.prec;
    uint32_t tableId = precision - 8;
    B2D_ASSERT(tableId < Constants::kTableCount);

    // Invert the origin and move it to the center of the pixel.
    m.mapPoint(c, c);
    c._x = 0.5 - c.x();
    c._y = 0.5 - c.y();

    conical.xx = mInv.m00();
    conical.xy = mInv.m01();
    conical.yx = mInv.m10();
    conical.yy = mInv.m11();
    conical.ox = mInv.m20() + c.x() * mInv.m00() + c.y() * mInv.m10();
    conical.oy = mInv.m21() + c.x() * mInv.m01() + c.y() * mInv.m11();
    conical.consts = &gConstants.xmm_f_con[tableId];

    return kFetchTypeGradientConical;
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Precomputed color table, used by all gradient fetchers.
  struct Table {
    const void* data;                  //!< Pixel data, array of either 32-bit or 64-bit pixels.
    uint32_t size;                     //!< Number of pixels stored in `data`, must be power of 2.

    // TODO: Remove `prec`, should not be needed anymore
    uint32_t prec;                     //!< Precision (number of pixels as `1 << bits`).
  };

  //! Linear gradient data.
  struct alignas(16) Linear {
    Value64 pt[2];                     //!< Gradient offset of the pixel at [0, 0].
    Value64 dy;                        //!< One Y step.
    Value64 dt;                        //!< One X step.
    Value64 dt2;                       //!< Two X steps.
    Value64 rep;                       //!< Reflect/Repeat mask (repeated/reflected size - 1).
    Value32 msk;                       //!< Size mask (gradient size - 1).
  };

  //! Radial gradient data.
  struct alignas(16) Radial {
    double xx, xy;                     //!< Gradient X/Y increments (horizontal).
    double yx, yy;                     //!< Gradient X/Y increments (vertical).
    double ox, oy;                     //!< Gradient X/Y offsets of the pixel at [0, 0].

    double ax, ay;
    double fx, fy;

    double dd, bd;
    double ddx, ddy;
    double ddd, scale;

    int maxi;
  };

  //! Conical gradient data.
  struct alignas(16) Conical {
    double xx, xy;                     //!< Gradient X/Y increments (horizontal).
    double yx, yy;                     //!< Gradient X/Y increments (vertical).
    double ox, oy;                     //!< Gradient X/Y offsets of the pixel at [0, 0].
    const Constants::Conical* consts;  //!< Atan2 approximation constants.
  };

  Table table;                         //!< Precomputed color table.
  union {
    Linear linear;                     //!< Linear gradient specific data.
    Radial radial;                     //!< Radial gradient specific data.
    Conical conical;                   //!< Conical gradient specific data.
  };
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHGRADIENTDATA_P_H
