// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_GEOMTYPES_H
#define _B2D_CORE_GEOMTYPES_H

// [Dependencies]
#include "../core/math.h"
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct IntPoint;
struct IntSize;
struct IntBox;
struct IntRect;

struct Point;
struct Size;
struct Box;
struct Rect;

// ============================================================================
// [b2d::LineIntersection]
// ============================================================================

//! Line intersection result.
enum LineIntersection : uint32_t {
  kLineIntersectionNone = 0,
  kLineIntersectionBounded = 1,
  kLineIntersectionUnbounded = 2
};

// ============================================================================
// [b2d::IntPoint]
// ============================================================================

//! Point (int).
struct IntPoint {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline IntPoint() noexcept = default;
  constexpr IntPoint(const IntPoint& p) noexcept = default;

  constexpr IntPoint(int x, int y) noexcept
    : _x(x),
      _y(y) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0, 0); }
  B2D_INLINE void reset(const IntPoint& p) noexcept { *this = p; }
  B2D_INLINE void reset(int x, int y) noexcept { _x = x; _y = y; }

  constexpr int x() const noexcept { return _x; }
  constexpr int y() const noexcept { return _y; }

  B2D_INLINE void setX(int x) noexcept { _x = x; }
  B2D_INLINE void setY(int y) noexcept { _y = y; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void negate() noexcept { _x = -_x; _y = -_y; }
  constexpr IntPoint negated() const noexcept { return IntPoint { -_x, -_y }; }

  B2D_INLINE void add(const IntPoint& p) noexcept { add(p.x(), p.y()); }
  B2D_INLINE void sub(const IntPoint& p) noexcept { sub(p.x(), p.y()); }
  B2D_INLINE void mul(const IntPoint& p) noexcept { mul(p.x(), p.y()); }

  B2D_INLINE void add(int i) noexcept { add(i, i); }
  B2D_INLINE void sub(int i) noexcept { sub(i, i); }
  B2D_INLINE void mul(int i) noexcept { mul(i, i); }

  B2D_INLINE void add(int x, int y) noexcept { _x += x; _y += y; }
  B2D_INLINE void sub(int x, int y) noexcept { _x -= x; _y -= y; }
  B2D_INLINE void mul(int x, int y) noexcept { _x *= x; _y *= y; }

  B2D_INLINE void translate(const IntPoint& p) noexcept { add(p); }
  B2D_INLINE void translate(int x, int y) noexcept { add(x, y); }

  constexpr IntPoint translated(const IntPoint& p) const noexcept { return IntPoint { _x + p._x, _y + p._y }; }
  constexpr IntPoint translated(int tx, int ty) const noexcept { return IntPoint { _x + tx, _y + ty }; }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const IntPoint& p) const noexcept { return eq(p.x(), p.y()); }
  constexpr bool eq(int x, int y) const noexcept { return (_x == x) & (_y == y); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline IntPoint& operator=(const IntPoint& p) noexcept = default;

  inline IntPoint& operator+=(const IntPoint& p) noexcept { add(p.x(), p.y()); return *this; }
  inline IntPoint& operator-=(const IntPoint& p) noexcept { sub(p.x(), p.y()); return *this; }
  inline IntPoint& operator*=(const IntPoint& p) noexcept { mul(p.x(), p.y()); return *this; }

  inline IntPoint& operator+=(int i) noexcept { add(i); return *this; }
  inline IntPoint& operator-=(int i) noexcept { sub(i); return *this; }
  inline IntPoint& operator*=(int i) noexcept { mul(i); return *this; }

  constexpr IntPoint operator+(const IntPoint& p) const noexcept { return IntPoint { _x + p.x(), _y + p.y() }; }
  constexpr IntPoint operator-(const IntPoint& p) const noexcept { return IntPoint { _x - p.x(), _y - p.y() }; }
  constexpr IntPoint operator*(const IntPoint& p) const noexcept { return IntPoint { _x * p.x(), _y * p.y() }; }

  constexpr IntPoint operator+(int i) const noexcept { return IntPoint { _x + i, _y + i }; }
  constexpr IntPoint operator-(int i) const noexcept { return IntPoint { _x - i, _y - i }; }
  constexpr IntPoint operator*(int i) const noexcept { return IntPoint { _x * i, _y * i }; }

  constexpr bool operator==(const IntPoint& p) const noexcept { return  eq(p); }
  constexpr bool operator!=(const IntPoint& p) const noexcept { return !eq(p); }
  constexpr bool operator< (const IntPoint& p) const noexcept { return (y() <  p.y()) | ((y() <= p.y()) & (x() <  p.x())); }
  constexpr bool operator> (const IntPoint& p) const noexcept { return (y() >  p.y()) | ((y() <= p.y()) & (x() >  p.x())); }
  constexpr bool operator<=(const IntPoint& p) const noexcept { return (y() <  p.y()) | ((y() == p.y()) & (x() <= p.x())); }
  constexpr bool operator>=(const IntPoint& p) const noexcept { return (y() >  p.y()) | ((y() == p.y()) & (x() >= p.x())); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  int _x;
  int _y;
};

// ============================================================================
// [b2d::IntSize]
// ============================================================================

//! Size (int).
struct IntSize {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline IntSize() noexcept = default;
  constexpr IntSize(const IntSize& size) noexcept = default;

  constexpr IntSize(int w, int h) noexcept
    : _w(w),
      _h(h) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_w > 0) & (_h > 0); }

  B2D_INLINE void reset() noexcept { reset(0, 0); }
  B2D_INLINE void reset(const IntSize& size) noexcept { *this = size; }
  B2D_INLINE void reset(int w, int h) noexcept { _w = w; _h = h; }

  B2D_INLINE void resetToMax() noexcept {
    _w = std::numeric_limits<int>::max();
    _h = std::numeric_limits<int>::max();
  }

  constexpr int w() const noexcept { return _w; }
  constexpr int h() const noexcept { return _h; }

  constexpr int width() const noexcept { return _w; }
  constexpr int height() const noexcept { return _h; }

  B2D_INLINE void setWidth(int w) noexcept { _w = w; }
  B2D_INLINE void setHeight(int h) noexcept { _h = h; }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const IntSize& size) const noexcept { return eq(size.w(), size.h()); }
  constexpr bool eq(int w, int h) const noexcept { return (_w == w) & (_h == h); }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void add(const IntSize& other) noexcept { add(other.w(), other.h()); }
  B2D_INLINE void sub(const IntSize& other) noexcept { sub(other.w(), other.h()); }
  B2D_INLINE void mul(const IntSize& other) noexcept { mul(other.w(), other.h()); }

  B2D_INLINE void add(int i) noexcept { add(i, i); }
  B2D_INLINE void sub(int i) noexcept { sub(i, i); }
  B2D_INLINE void mul(int i) noexcept { mul(i, i); }

  B2D_INLINE void add(int w, int h) noexcept { _w += w; _h += h; }
  B2D_INLINE void sub(int w, int h) noexcept { _w -= w; _h -= h; }
  B2D_INLINE void mul(int w, int h) noexcept { _w *= w; _h *= h; }

  constexpr IntSize adjusted(int w, int h) const noexcept { return IntSize { _w + w, _h + h }; }
  constexpr IntSize adjusted(const IntSize& size) const noexcept { return IntSize { _w + size._w, _h + size._h }; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE IntSize& operator=(const IntSize& other) noexcept = default;
  B2D_INLINE IntSize& operator+=(const IntSize& other) noexcept { add(other); return *this; }
  B2D_INLINE IntSize& operator-=(const IntSize& other) noexcept { sub(other); return *this; }
  B2D_INLINE IntSize& operator*=(const IntSize& other) noexcept { mul(other); return *this; }

  B2D_INLINE IntSize& operator+=(int i) noexcept { add(i); return *this; }
  B2D_INLINE IntSize& operator-=(int i) noexcept { sub(i); return *this; }
  B2D_INLINE IntSize& operator*=(int i) noexcept { mul(i); return *this; }

  constexpr IntSize operator+(const IntSize& other) const noexcept { return IntSize { w() + other.w(), h() + other.h() }; }
  constexpr IntSize operator-(const IntSize& other) const noexcept { return IntSize { w() - other.w(), h() - other.h() }; }
  constexpr IntSize operator*(const IntSize& other) const noexcept { return IntSize { w() * other.w(), h() * other.h() }; }

  constexpr IntSize operator+(int i) const noexcept { return IntSize { w() + i, h() + i }; }
  constexpr IntSize operator-(int i) const noexcept { return IntSize { w() - i, h() - i }; }
  constexpr IntSize operator*(int i) const noexcept { return IntSize { w() * i, h() * i }; }

  constexpr bool operator==(const IntSize& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const IntSize& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  int _w;
  int _h;
};

// ============================================================================
// [b2d::IntBox]
// ============================================================================

//! Box (int).
struct IntBox {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline IntBox() noexcept = default;
  constexpr IntBox(const IntBox& box) noexcept = default;

  constexpr IntBox(int x0, int y0, int x1, int y1) noexcept
    : _x0(x0),
      _y0(y0),
      _x1(x1),
      _y1(y1) {}

  inline explicit IntBox(const IntRect& other) noexcept {
    resetToRect(other);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_x1 > _x0) & (_y1 > _y0); }

  B2D_INLINE void reset() noexcept { reset(0, 0, 0, 0); }
  B2D_INLINE void reset(const IntBox& other) noexcept { *this = other; }
  B2D_INLINE void reset(int x0, int y0, int x1, int y1) noexcept { _x0 = x0; _y0 = y0; _x1 = x1; _y1 = y1; }

  B2D_INLINE void resetToRect(const IntRect& rect) noexcept;
  B2D_INLINE void resetToRect(int x, int y, int w, int h) noexcept { reset(x, y, x + w, y + h); }

  B2D_INLINE void resetToMinMax() noexcept {
    _x0 = std::numeric_limits<int>::lowest();
    _y0 = std::numeric_limits<int>::lowest();
    _x1 = std::numeric_limits<int>::max();
    _y1 = std::numeric_limits<int>::max();
  }

  constexpr int x() const noexcept { return _x0; }
  constexpr int y() const noexcept { return _y0; }
  constexpr int w() const noexcept { return _x1 - _x0; }
  constexpr int h() const noexcept { return _y1 - _y0; }

  constexpr int width() const noexcept { return w(); }
  constexpr int height() const noexcept { return h(); }

  constexpr int x0() const noexcept { return _x0; }
  constexpr int y0() const noexcept { return _y0; }
  constexpr int x1() const noexcept { return _x1; }
  constexpr int y1() const noexcept { return _y1; }

  constexpr IntPoint origin() const noexcept { return IntPoint { _x0, _y0 }; }
  constexpr IntSize size() const noexcept { return IntSize { _x1 - _x0, _y1 - _y0 }; }

  B2D_INLINE void setX(int x) noexcept { _x0 = x; }
  B2D_INLINE void setY(int y) noexcept { _y0 = y; }
  B2D_INLINE void setWidth(int w) noexcept { _x1 = _x0 + w; }
  B2D_INLINE void setHeight(int h) noexcept { _y1 = _y0 + h; }

  B2D_INLINE void setX0(int x0) noexcept { _x0 = x0; }
  B2D_INLINE void setY0(int y0) noexcept { _y0 = y0; }
  B2D_INLINE void setX1(int x1) noexcept { _x1 = x1; }
  B2D_INLINE void setY1(int y1) noexcept { _y1 = y1; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const IntPoint& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translate(int tx, int ty) noexcept {
    _x0 += tx;
    _y0 += ty;
    _x1 += tx;
    _y1 += ty;
  }

  // --------------------------------------------------------------------------
  // [Algebra]
  // --------------------------------------------------------------------------

  //! Returns `true` if both rectangles overlap.
  constexpr bool overlaps(const IntBox& r) const noexcept { return (((_y0 - r._y1) ^ (_y1 - r._y0)) & ((_x0 - r._x1) ^ (_x1 - r._x0))) < 0; }
  //! Returns `true` if the rectangle completely subsumes `r`.
  constexpr bool subsumes(const IntBox& r) const noexcept { return (r._x0 >= _x0) & (r._x1 <= _x1) & (r._y0 >= _y0) & (r._y1 <= _y1); }

  // --------------------------------------------------------------------------
  // [Contains]
  // --------------------------------------------------------------------------

  B2D_INLINE bool contains(const IntPoint& p) const noexcept { return contains(p.x(), p.y()); }
  B2D_INLINE bool contains(int px, int py) const noexcept { return (px >= _x0) & (py >= _y0) & (px < _x1) & (py < _y1); }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const IntBox& other) const noexcept {
    return eq(other.x0(), other.y0(), other.x1(), other.y1());
  }

  constexpr bool eq(int x0, int y0, int x1, int y1) const noexcept {
    return (_x0 == x0) & (_y0 == y0) & (_x1 == x1) & (_y1 == y1);
  }

  // --------------------------------------------------------------------------
  // [Shrink / Expand]
  // --------------------------------------------------------------------------

  //! Shrinks rectangle by `n`.
  B2D_INLINE void shrink(int n) noexcept {
    _x0 += n;
    _y0 += n;
    _x1 -= n;
    _y1 -= n;
  }

  //! Expands rectangle by `n`.
  B2D_INLINE void expand(int n) noexcept {
    _x0 -= n;
    _y0 -= n;
    _x1 += n;
    _y1 += n;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE IntBox& operator=(const IntBox& other) noexcept = default;
  B2D_INLINE IntBox& operator=(const IntRect& rect) noexcept { resetToRect(rect); return *this; }

  constexpr IntBox operator+(const IntPoint& p) const noexcept { return IntBox { _x0 + p._x, _y0 + p._y, _x1 + p._x, _y1 + p._y }; }
  constexpr IntBox operator-(const IntPoint& p) const noexcept { return IntBox { _x0 - p._x, _y0 - p._y, _x1 - p._x, _y1 - p._y }; }

  B2D_INLINE IntBox& operator+=(const IntPoint& p) noexcept { translate(p); return *this; }
  B2D_INLINE IntBox& operator-=(const IntPoint& p) noexcept { translate(-p.x(), -p.y()); return *this; }

  constexpr bool operator==(const IntBox& box) const noexcept { return  eq(box); }
  constexpr bool operator!=(const IntBox& box) const noexcept { return !eq(box); }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static B2D_INLINE bool intersect(IntBox& dst, const IntBox& a, const IntBox& b) noexcept {
    dst.reset(Math::max(a._x0, b._x0), Math::max(a._y0, b._y0),
              Math::min(a._x1, b._x1), Math::min(a._y1, b._y1));
    return dst.isValid();
  }

  static B2D_INLINE void bound(IntBox& dst, const IntBox& a, const IntBox& b) noexcept {
    dst.reset(Math::min(a._x0, b._x0), Math::min(a._y0, b._y0),
              Math::max(a._x1, b._x1), Math::max(a._y1, b._y1));
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  int _x0;
  int _y0;
  int _x1;
  int _y1;
};

// ============================================================================
// [b2d::IntRect]
// ============================================================================

//! Rectangle (int).
struct IntRect {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline IntRect() noexcept = default;
  constexpr IntRect(const IntRect& other) noexcept = default;

  constexpr IntRect(const IntPoint& p0, const IntSize& sz) noexcept
    : _x(p0.x()),
      _y(p0.y()),
      _w(sz.w()),
      _h(sz.h()) {}

  constexpr IntRect(const IntPoint& p0, const IntPoint& p1) noexcept
    : _x(p0.x()),
      _y(p0.y()),
      _w(p1.x() - p0.x()),
      _h(p1.y() - p0.y()) {}

  constexpr IntRect(int x, int y, int w, int h) noexcept
    : _x(x),
      _y(y),
      _w(w),
      _h(h) {}

  inline explicit IntRect(const IntBox& box) noexcept { resetToBox(box); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_w > 0) & (_h > 0); }

  B2D_INLINE void reset() noexcept { reset(0, 0, 0, 0); }
  B2D_INLINE void reset(const IntRect& r) noexcept { *this = r; }
  B2D_INLINE void reset(const IntPoint& p, const IntSize& size) noexcept { reset(p.x(), p.y(), size.w(), size.h()); }
  B2D_INLINE void reset(int x, int y, int w, int h) noexcept { _x = x; _y = y; _w = w; _h = h; }

  B2D_INLINE void resetToBox(const IntBox& box) noexcept { resetToBox(box.x0(), box.y0(), box.x1(), box.y1()); }
  B2D_INLINE void resetToBox(const IntPoint& p0, const IntPoint& p1) noexcept { resetToBox(p0.x(), p0.y(), p1.x(), p1.y()); }
  B2D_INLINE void resetToBox(int x0, int y0, int x1, int y1) noexcept { reset(x0, y0, x1 - x0, y1 - y0); }

  constexpr int x() const noexcept { return _x; }
  constexpr int y() const noexcept { return _y; }
  constexpr int w() const noexcept { return _w; }
  constexpr int h() const noexcept { return _h; }

  constexpr int width() const noexcept { return _w; }
  constexpr int height() const noexcept { return _h; }

  constexpr int x0() const noexcept { return _x; }
  constexpr int y0() const noexcept { return _y; }
  constexpr int x1() const noexcept { return _x + _w; }
  constexpr int y1() const noexcept { return _y + _h; }

  constexpr int left() const noexcept { return _x; }
  constexpr int top() const noexcept { return _y; }
  constexpr int right() const noexcept { return _x + _w; }
  constexpr int bottom() const noexcept { return _y + _h; }

  constexpr IntPoint origin() const noexcept { return IntPoint { _x, _y }; }
  constexpr IntSize size() const noexcept { return IntSize { _w, _h }; }

  B2D_INLINE void setX(int x) noexcept { _x = x; }
  B2D_INLINE void setY(int y) noexcept { _y = y; }
  B2D_INLINE void setWidth(int w) noexcept { _w = w; }
  B2D_INLINE void setHeight(int h) noexcept { _h = h; }

  B2D_INLINE void setX0(int x0) noexcept { _x = x0; }
  B2D_INLINE void setY0(int y0) noexcept { _y = y0; }
  B2D_INLINE void setX1(int x1) noexcept { _w = x1 - _x; }
  B2D_INLINE void setY1(int y1) noexcept { _h = y1 - _y; }

  B2D_INLINE void setLeft(int x0) noexcept { _x = x0; }
  B2D_INLINE void setTop(int y0) noexcept { _y = y0; }
  B2D_INLINE void setRight(int x1) noexcept { _w = x1 - _x; }
  B2D_INLINE void setBottom(int y1) noexcept { _h = y1 - _y; }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const IntPoint& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translate(int tx, int ty) noexcept { _x += tx; _y += ty; }

  constexpr IntRect translated(const IntPoint& p) const noexcept { return translated(p.x(), p.y()); }
  constexpr IntRect translated(int tx, int ty) const noexcept { return IntRect { _x + tx, _y + ty, _w, _h }; }

  // --------------------------------------------------------------------------
  // [Algebra]
  // --------------------------------------------------------------------------

  //! Checks if two rectangles overlap.
  constexpr bool overlaps(const IntRect& r) const noexcept {
    return ( ((y0() - r.y1()) ^ (y1() - r.y0())) &
             ((x0() - r.x1()) ^ (x1() - r.x0())) ) < 0;
  }

  //! Get whether rectangle completely subsumes @a r.
  constexpr bool subsumes(const IntRect& r) const noexcept {
    return r.x0() >= x0() && r.x1() <= x1() &&
           r.y0() >= y0() && r.y1() <= y1() ;
  }

  static B2D_INLINE bool intersect(IntRect& dst, const IntRect& a, const IntRect& b) noexcept {
    int xx = Math::max(a.x0(), b.x0());
    int yy = Math::max(a.y0(), b.y0());

    dst.reset(xx, yy, Math::min(a.x1(), b.x1()) - xx,
                      Math::min(a.y1(), b.y1()) - yy);
    return dst.isValid();
  }

  static B2D_INLINE void unite(IntRect& dst, const IntRect& a, const IntRect& b) noexcept {
    int x0 = Math::min(a._x, b._x);
    int y0 = Math::min(a._y, b._y);
    int x1 = Math::max(a._x + a._w, b._x + b._w);
    int y1 = Math::max(a._y + a._h, b._y + b._h);

    dst.reset(x0, y0, x1 - x0, y1 - y0);
  }

  // --------------------------------------------------------------------------
  // [Contains]
  // --------------------------------------------------------------------------

  constexpr bool contains(const IntPoint& p) const noexcept { return contains(p.x(), p.y()); }
  constexpr bool contains(int px, int py) const noexcept { return (unsigned(px - _x) < unsigned(_w)) & (unsigned(py - _y) < unsigned(_h)); }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const IntRect& other) const noexcept {
    return eq(other.x(), other.y(), other.w(), other.h());
  }

  constexpr bool eq(int x, int y, int w, int h) const noexcept {
    return (_x == x) & (_y == y) & (_w == w) & (_h == h);
  }

  // --------------------------------------------------------------------------
  // [Shrink / Expand]
  // --------------------------------------------------------------------------

  //! Shrink rectangle by `n`.
  B2D_INLINE IntRect& shrink(int n) noexcept {
    _x += n;
    _y += n;

    n <<= 1;
    _w -= n;
    _h -= n;

    return *this;
  }

  //! Expand rectangle by `n`.
  B2D_INLINE IntRect& expand(int n) noexcept {
    _x -= n;
    _y -= n;

    n <<= 1;
    _w += n;
    _h += n;

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Adjust]
  // --------------------------------------------------------------------------

  B2D_INLINE IntRect& adjust(int x, int y) noexcept {
    _x += x; _w -= x;
    _y += y; _h -= y;

    _w -= x;
    _h -= y;

    return *this;
  }

  B2D_INLINE IntRect& adjust(int x0, int y0, int x1, int y1) noexcept {
    _x += x0;
    _y += y0;

    _w -= x0;
    _h -= y0;

    _w += x1;
    _h += y1;

    return *this;
  }

  B2D_INLINE IntRect adjusted(int x, int y) const noexcept {
    return IntRect(_x + x, _y + y, _w - x - x, _h - y - y);
  }

  B2D_INLINE IntRect adjusted(int x0, int y0, int x1, int y1) const noexcept {
    return IntRect(_x + x0, _y + y0, _w - x0 + x1, _h - y0 + y1);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE IntRect& operator=(const IntRect& other) noexcept = default;
  B2D_INLINE IntRect& operator=(const IntBox& other) noexcept { resetToBox(other); return *this; }

  B2D_INLINE IntRect& operator+=(const IntPoint& p) noexcept { translate( p.x(),  p.y()); return *this; }
  B2D_INLINE IntRect& operator-=(const IntPoint& p) noexcept { translate(-p.x(), -p.y()); return *this; }

  constexpr IntRect operator+(const IntPoint& p) const noexcept { return IntRect { _x + p._x, _y + p._y, _w, _h }; }
  constexpr IntRect operator-(const IntPoint& p) const noexcept { return IntRect { _x - p._x, _y - p._y, _w, _h }; }

  constexpr bool operator==(const IntRect& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const IntRect& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  int _x;
  int _y;
  int _w;
  int _h;
};

// ============================================================================
// [b2d::Point]
// ============================================================================

//! Point.
struct Point {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Point() noexcept = default;
  constexpr Point(const Point& p) noexcept = default;

  constexpr Point(double x, double y) noexcept
    : _x(x),
      _y(y) {}

  constexpr explicit Point(const IntPoint& p) noexcept
    : _x(double(p._x)),
      _y(double(p._y)) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0); }
  B2D_INLINE void reset(const Point& p) noexcept { *this = p; }
  B2D_INLINE void reset(const IntPoint& p) noexcept { reset(double(p.x()), double(p.y())); }
  B2D_INLINE void reset(double x, double y) noexcept { _x = x; _y = y; }

  B2D_INLINE void resetToNaN() noexcept {
    _x = std::numeric_limits<double>::quiet_NaN();
    _y = std::numeric_limits<double>::quiet_NaN();
  }

  constexpr double x() const noexcept { return _x; }
  constexpr double y() const noexcept { return _y; }

  B2D_INLINE void setX(double x) noexcept { _x = x; }
  B2D_INLINE void setY(double y) noexcept { _y = y; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void negate() noexcept { _x = -_x; _y = -_y; }
  constexpr Point negated() const noexcept { return Point { -_x, -_y }; }

  B2D_INLINE void add(const Point& p) noexcept { add(p.x(), p.y()); }
  B2D_INLINE void sub(const Point& p) noexcept { sub(p.x(), p.y()); }
  B2D_INLINE void mul(const Point& p) noexcept { mul(p.x(), p.y()); }
  B2D_INLINE void div(const Point& p) noexcept { div(p.x(), p.y()); }

  B2D_INLINE void add(const IntPoint& p) noexcept { add(double(p.x()), double(p.y())); }
  B2D_INLINE void sub(const IntPoint& p) noexcept { sub(double(p.x()), double(p.y())); }
  B2D_INLINE void mul(const IntPoint& p) noexcept { mul(double(p.x()), double(p.y())); }
  B2D_INLINE void div(const IntPoint& p) noexcept { div(double(p.x()), double(p.y())); }

  B2D_INLINE void add(double i) noexcept { add(i, i); }
  B2D_INLINE void sub(double i) noexcept { sub(i, i); }
  B2D_INLINE void mul(double i) noexcept { mul(i, i); }
  B2D_INLINE void div(double i) noexcept { div(i, i); }

  B2D_INLINE void add(double x, double y) noexcept { _x += x; _y += y; }
  B2D_INLINE void sub(double x, double y) noexcept { _x -= x; _y -= y; }
  B2D_INLINE void mul(double x, double y) noexcept { _x *= x; _y *= y; }
  B2D_INLINE void div(double x, double y) noexcept { _x /= x; _y /= y; }

  B2D_INLINE void translate(const Point& p) noexcept { add(p); }
  B2D_INLINE void translate(const IntPoint& p) noexcept { add(p); }
  B2D_INLINE void translate(double x, double y) noexcept { add(x, y); }

  constexpr Point translated(const Point& p) const noexcept { return translated(p.x(), p.y()); }
  constexpr Point translated(const IntPoint& p) const noexcept { return translated(double(p.x()), double(p.y())); }
  constexpr Point translated(double tx, double ty) const noexcept { return Point { _x + tx, _y + ty }; }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const Point& p) const noexcept { return eq(p.x(), p.y()); }
  constexpr bool eq(double x, double y) const noexcept { return (_x == x) & (_y == y); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Point& operator=(const Point& p) noexcept = default;
  B2D_INLINE Point& operator+=(const Point& p) noexcept { add(p); return *this; }
  B2D_INLINE Point& operator-=(const Point& p) noexcept { sub(p); return *this; }
  B2D_INLINE Point& operator*=(const Point& p) noexcept { mul(p); return *this; }
  B2D_INLINE Point& operator/=(const Point& p) noexcept { div(p); return *this; }

  B2D_INLINE Point& operator=(const IntPoint& p) noexcept { reset(p); return *this; }
  B2D_INLINE Point& operator+=(const IntPoint& p) noexcept { add(p); return *this; }
  B2D_INLINE Point& operator-=(const IntPoint& p) noexcept { sub(p); return *this; }
  B2D_INLINE Point& operator*=(const IntPoint& p) noexcept { mul(p); return *this; }
  B2D_INLINE Point& operator/=(const IntPoint& p) noexcept { div(p); return *this; }

  B2D_INLINE Point& operator+=(double i) noexcept { add(i); return *this; }
  B2D_INLINE Point& operator-=(double i) noexcept { sub(i); return *this; }
  B2D_INLINE Point& operator*=(double i) noexcept { mul(i); return *this; }
  B2D_INLINE Point& operator/=(double i) noexcept { div(i); return *this; }

  constexpr Point operator+(const Point& p) const noexcept { return Point { x() + p.x(), y() + p.y() }; }
  constexpr Point operator-(const Point& p) const noexcept { return Point { x() - p.x(), y() - p.y() }; }
  constexpr Point operator*(const Point& p) const noexcept { return Point { x() * p.x(), y() * p.y() }; }
  constexpr Point operator/(const Point& p) const noexcept { return Point { x() / p.x(), y() / p.y() }; }

  constexpr Point operator+(const IntPoint& p) const noexcept { return Point { x() + double(p.x()), y() + double(p.y()) }; }
  constexpr Point operator-(const IntPoint& p) const noexcept { return Point { x() - double(p.x()), y() - double(p.y()) }; }
  constexpr Point operator*(const IntPoint& p) const noexcept { return Point { x() * double(p.x()), y() * double(p.y()) }; }
  constexpr Point operator/(const IntPoint& p) const noexcept { return Point { x() / double(p.x()), y() / double(p.y()) }; }

  constexpr Point operator+(double i) const noexcept { return Point { x() + i, y() + i }; }
  constexpr Point operator-(double i) const noexcept { return Point { x() - i, y() - i }; }
  constexpr Point operator*(double i) const noexcept { return Point { x() * i, y() * i }; }
  constexpr Point operator/(double i) const noexcept { return Point { x() / i, y() / i }; }

  constexpr bool operator==(const Point& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const Point& other) const noexcept { return !eq(other); }

  constexpr bool operator< (const Point& other) const noexcept { return (y() < other.y()) | ((y() <= other.y()) & (x() <  other.x())); }
  constexpr bool operator> (const Point& other) const noexcept { return (y() > other.y()) | ((y() <= other.y()) & (x() >  other.x())); }
  constexpr bool operator<=(const Point& other) const noexcept { return (y() < other.y()) | ((y() == other.y()) & (x() <= other.x())); }
  constexpr bool operator>=(const Point& other) const noexcept { return (y() > other.y()) | ((y() == other.y()) & (x() >= other.x())); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _x;
  double _y;
};

// ============================================================================
// [b2d::Size]
// ============================================================================

struct Size {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Size() noexcept = default;
  constexpr Size(const Size& size) noexcept = default;

  inline explicit Size(const IntSize& size) noexcept
    : _w(size._w),
      _h(size._h) {}

  constexpr Size(double w, double h) noexcept
    : _w(w),
      _h(h) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_w > 0.0) & (_h > 0.0); }

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0); }
  B2D_INLINE void reset(const Size& size) noexcept { *this = size; }
  B2D_INLINE void reset(const IntSize& size) noexcept { reset(double(size.w()), double(size.h())); }
  B2D_INLINE void reset(double w, double h) noexcept { _w = w; _h = h; }

  B2D_INLINE void resetToNaN() noexcept {
    _w = std::numeric_limits<double>::quiet_NaN();
    _h = std::numeric_limits<double>::quiet_NaN();
  }

  B2D_INLINE void resetToMax() noexcept {
    _w = std::numeric_limits<double>::max();
    _h = std::numeric_limits<double>::max();
  }

  constexpr double w() const noexcept { return _w; }
  constexpr double h() const noexcept { return _h; }

  constexpr double width() const noexcept { return _w; }
  constexpr double height() const noexcept { return _h; }

  B2D_INLINE void setWidth(double w) noexcept { _w = w; }
  B2D_INLINE void setHeight(double h) noexcept { _h = h; }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const Size& other) const noexcept { return eq(other.w(), other.h()); }
  constexpr bool eq(double w, double h) const noexcept { return (_w == w) & (_h == h); }

  // --------------------------------------------------------------------------
  // [Adjust]
  // --------------------------------------------------------------------------

  B2D_INLINE void add(const Size& size) noexcept { add(size.w(), size.h()); }
  B2D_INLINE void sub(const Size& size) noexcept { sub(size.w(), size.h()); }
  B2D_INLINE void mul(const Size& size) noexcept { mul(size.w(), size.h()); }
  B2D_INLINE void div(const Size& size) noexcept { div(size.w(), size.h()); }

  B2D_INLINE void add(const IntSize& size) noexcept { add(double(size.w()), double(size.h())); }
  B2D_INLINE void sub(const IntSize& size) noexcept { sub(double(size.w()), double(size.h())); }
  B2D_INLINE void mul(const IntSize& size) noexcept { mul(double(size.w()), double(size.h())); }
  B2D_INLINE void div(const IntSize& size) noexcept { div(double(size.w()), double(size.h())); }

  B2D_INLINE void add(double i) noexcept { add(i, i); }
  B2D_INLINE void sub(double i) noexcept { sub(i, i); }
  B2D_INLINE void mul(double i) noexcept { mul(i, i); }
  B2D_INLINE void div(double i) noexcept { div(i, i); }

  B2D_INLINE void add(double w, double h) noexcept { _w += w; _h += h; }
  B2D_INLINE void sub(double w, double h) noexcept { _w -= w; _h -= h; }
  B2D_INLINE void mul(double w, double h) noexcept { _w *= w; _h *= h; }
  B2D_INLINE void div(double w, double h) noexcept { _w /= w; _h /= h; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Size& operator=(const Size& other) noexcept = default;
  B2D_INLINE Size& operator=(const IntSize& other) noexcept { reset(other); return *this; }

  B2D_INLINE Size& operator+=(const Size& other) noexcept { add(other.w(), other.h()); return *this; }
  B2D_INLINE Size& operator-=(const Size& other) noexcept { sub(other.w(), other.h()); return *this; }
  B2D_INLINE Size& operator*=(const Size& other) noexcept { mul(other.w(), other.h()); return *this; }
  B2D_INLINE Size& operator/=(const Size& other) noexcept { div(other.w(), other.h()); return *this; }

  B2D_INLINE Size& operator+=(const IntSize& other) noexcept { add(double(other.w()), double(other.h())); return *this; }
  B2D_INLINE Size& operator-=(const IntSize& other) noexcept { sub(double(other.w()), double(other.h())); return *this; }
  B2D_INLINE Size& operator*=(const IntSize& other) noexcept { mul(double(other.w()), double(other.h())); return *this; }
  B2D_INLINE Size& operator/=(const IntSize& other) noexcept { div(double(other.w()), double(other.h())); return *this; }

  B2D_INLINE Size& operator+=(double i) noexcept { add(i, i); return *this; }
  B2D_INLINE Size& operator-=(double i) noexcept { sub(i, i); return *this; }
  B2D_INLINE Size& operator*=(double i) noexcept { mul(i, i); return *this; }
  B2D_INLINE Size& operator/=(double i) noexcept { div(i, i); return *this; }

  constexpr Size operator+(const Size& other) const noexcept { return Size { w() + other.w(), h() + other.h() }; }
  constexpr Size operator-(const Size& other) const noexcept { return Size { w() - other.w(), h() - other.h() }; }
  constexpr Size operator*(const Size& other) const noexcept { return Size { w() * other.w(), h() * other.h() }; }
  constexpr Size operator/(const Size& other) const noexcept { return Size { w() / other.w(), h() / other.h() }; }

  constexpr Size operator+(const IntSize& other) const noexcept { return Size { w() + other.w(), h() + other.h() }; }
  constexpr Size operator-(const IntSize& other) const noexcept { return Size { w() - other.w(), h() - other.h() }; }
  constexpr Size operator*(const IntSize& other) const noexcept { return Size { w() * other.w(), h() * other.h() }; }
  constexpr Size operator/(const IntSize& other) const noexcept { return Size { w() / other.w(), h() / other.h() }; }

  constexpr bool operator==(const Size& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const Size& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _w;
  double _h;
};

// ============================================================================
// [b2d::Box]
// ============================================================================

//! Box.
struct Box {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Box() noexcept = default;
  constexpr Box(const Box& box) noexcept = default;

  constexpr Box(double x0, double y0, double x1, double y1) noexcept
    : _x0(x0),
      _y0(y0),
      _x1(x1),
      _y1(y1) {}

  constexpr explicit Box(const IntBox& box) noexcept
    : _x0(box._x0),
      _y0(box._y0),
      _x1(box._x1),
      _y1(box._y1) {}

  inline explicit Box(const Rect& rect) noexcept { resetToRect(rect); }
  inline explicit Box(const IntRect& rect) noexcept { resetToRect(rect); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_x1 > _x0) & (_y1 > _y0); }

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0, 0.0, 0.0); }
  B2D_INLINE void reset(const Box& box) noexcept { *this = box; }
  B2D_INLINE void reset(const IntBox& box) noexcept { reset(double(box.x0()), double(box.y0()), double(box.x1()), double(box.y1())); }
  B2D_INLINE void reset(double x0, double y0, double x1, double y1) noexcept { _x0 = x0; _y0 = y0; _x1 = x1; _y1 = y1; }

  B2D_INLINE void resetToRect(const Rect& rect) noexcept;
  B2D_INLINE void resetToRect(const IntRect& rect) noexcept;
  B2D_INLINE void resetToRect(double x, double y, double w, double h) noexcept { reset(x, y, x + w, y + h); }

  B2D_INLINE void resetToMinMax() noexcept {
    _x0 = std::numeric_limits<double>::lowest();
    _y0 = std::numeric_limits<double>::lowest();
    _x1 = std::numeric_limits<double>::max();
    _y1 = std::numeric_limits<double>::max();
  }

  B2D_INLINE void resetToNaN() noexcept {
    _x0 = std::numeric_limits<double>::quiet_NaN();
    _y0 = std::numeric_limits<double>::quiet_NaN();
    _x1 = std::numeric_limits<double>::quiet_NaN();
    _y1 = std::numeric_limits<double>::quiet_NaN();
  }

  constexpr double x() const noexcept { return _x0; }
  constexpr double y() const noexcept { return _y0; }
  constexpr double w() const noexcept { return _x1 - _x0; }
  constexpr double h() const noexcept { return _y1 - _y0; }

  constexpr double width() const noexcept { return w(); }
  constexpr double height() const noexcept { return h(); }

  constexpr double x0() const noexcept { return _x0; }
  constexpr double y0() const noexcept { return _y0; }
  constexpr double x1() const noexcept { return _x1; }
  constexpr double y1() const noexcept { return _y1; }

  constexpr Point origin() const noexcept { return Point { _x0, _y0 }; }
  constexpr Size size() const noexcept { return Size { _x1 - _x0, _y1 - _y0 }; }

  B2D_INLINE void setX(double x) noexcept { _x0 = x; }
  B2D_INLINE void setY(double y) noexcept { _y0 = y; }
  B2D_INLINE void setWidth(double w) noexcept { _x1 = _x0 + w; }
  B2D_INLINE void setHeight(double h) noexcept { _y1 = _y0 + h; }

  B2D_INLINE void setX0(double x0) noexcept { _x0 = x0; }
  B2D_INLINE void setY0(double y0) noexcept { _y0 = y0; }
  B2D_INLINE void setX1(double x1) noexcept { _x1 = x1; }
  B2D_INLINE void setY1(double y1) noexcept { _y1 = y1; }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translate(const IntPoint& p) noexcept { translate(double(p.x()), double(p.y())); }

  B2D_INLINE void translate(double tx, double ty) noexcept {
    _x0 += tx;
    _y0 += ty;
    _x1 += tx;
    _y1 += ty;
  }

  constexpr Box translated(const Point& p) const noexcept { return translated(p.x(), p.y()); }
  constexpr Box translated(const IntPoint& p) const noexcept { return translated(double(p.x()), double(p.y())); }
  constexpr Box translated(double tx, double ty) const noexcept { return Box { _x0 + tx, _y0 + ty, _x1 + tx, _y1 + ty }; }

  // --------------------------------------------------------------------------
  // [Bound]
  // --------------------------------------------------------------------------

  B2D_INLINE Box& bound(double x, double y) noexcept {
    boundX(x);
    boundY(y);
    return *this;
  }

  B2D_INLINE Box& bound(const Point& p) noexcept {
    return bound(p._x, p._y);
  }

  B2D_INLINE Box& bound(const Box& box) noexcept {
    if (_x0 > box._x0) _x0 = box._x0;
    if (_y0 > box._y0) _y0 = box._y0;
    if (_x1 < box._x1) _x1 = box._x1;
    if (_y1 < box._y1) _y1 = box._y1;
    return *this;
  }

  B2D_INLINE Box& boundX(double x) noexcept {
    if (_x0 > x)
      _x0 = x;
    else if (_x1 < x)
      _x1 = x;
    return *this;
  }

  B2D_INLINE Box& boundY(double y) noexcept {
    if (_y0 > y)
      _y0 = y;
    else if (_y1 < y)
      _y1 = y;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Algebra]
  // --------------------------------------------------------------------------

  //! Returns `true` if both rectangles overlap.
  B2D_INLINE bool overlaps(const Box& box) const noexcept { return box._x0 >= _x0 || box._x1 <= _x1 || box._y0 >= _y0 || box._y1 <= _y1 ; }
  //! Returns `true` if the rectangle completely subsumes `r`.
  B2D_INLINE bool subsumes(const Box& box) const noexcept { return box._x0 >= _x0 && box._x1 <= _x1 && box._y0 >= _y0 && box._y1 <= _y1 ; }

  // --------------------------------------------------------------------------
  // [Contains]
  // --------------------------------------------------------------------------

  constexpr bool contains(const Point& p) const noexcept { return contains(p._x, p._y); }
  constexpr bool contains(const IntPoint& p) const noexcept { return contains(double(p._x), double(p._y)); }
  constexpr bool contains(double x, double y) const noexcept { return x >= _x0 && y >= _y0 && x < _x1 && y < _y1; }

  // --------------------------------------------------------------------------
  // [Normalize]
  // --------------------------------------------------------------------------

  B2D_INLINE Box& normalize() noexcept {
    if (_x0 > _x1) std::swap(_x0, _x1);
    if (_y0 > _y1) std::swap(_y0, _y1);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const Box& other) const noexcept {
    return eq(other.x0(), other.y0(), other.x1(), other.y1());
  }

  constexpr bool eq(double x0, double y0, double x1, double y1) const noexcept {
    return (_x0 == x0) & (_y0 == y0) & (_x1 == x1) & (_y1 == y1) ;
  }

  // --------------------------------------------------------------------------
  // [Shrink / Expand]
  // --------------------------------------------------------------------------

  //! Shrinks rectangle by `n`.
  B2D_INLINE void shrink(double n) noexcept {
    _x0 += n;
    _y0 += n;
    _x1 -= n;
    _y1 -= n;
  }

  //! Expands rectangle by `n`.
  B2D_INLINE void expand(double n) noexcept {
    _x0 -= n;
    _y0 -= n;
    _x1 += n;
    _y1 += n;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Box& operator=(const Box& other) noexcept = default;
  B2D_INLINE Box& operator=(const IntBox& other) noexcept { reset(other); return *this; }
  B2D_INLINE Box& operator=(const Rect& other) noexcept { resetToRect(other); return *this; }
  B2D_INLINE Box& operator=(const IntRect& other) noexcept { resetToRect(other); return *this; }

  constexpr Box operator+(const Point& p) const noexcept { return Box { x0() + p.x(), y0() + p.y(), x1() + p.x(), y1() + p.y() }; }
  constexpr Box operator-(const Point& p) const noexcept { return Box { x0() - p.x(), y0() - p.y(), x1() - p.x(), y1() - p.y() }; }

  B2D_INLINE Box operator+(const IntPoint& p) const noexcept {
    double x = double(p._x);
    double y = double(p._y);
    return Box(_x0 + x, _y0 + y, _x1 + x, _y1 + y);
  }

  B2D_INLINE Box operator-(const IntPoint& p) const noexcept {
    double x = double(p._x);
    double y = double(p._y);
    return Box(_x0 - x, _y0 - y, _x1 - x, _y1 - y);
  }

  B2D_INLINE Box& operator+=(const Point& p) noexcept { translate(p); return *this; }
  B2D_INLINE Box& operator+=(const IntPoint& p) noexcept { translate(p); return *this; }

  B2D_INLINE Box& operator-=(const Point& p) noexcept {
    _x0 -= p._x;
    _y0 -= p._y;
    _x1 -= p._x;
    _y1 -= p._y;
    return *this;
  }

  B2D_INLINE Box& operator-=(const IntPoint& p) noexcept {
    double tx = double(p._x);
    double ty = double(p._y);

    _x0 -= tx;
    _y0 -= ty;
    _x1 -= tx;
    _y1 -= ty;

    return *this;
  }

  constexpr bool operator==(const Box& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const Box& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static B2D_INLINE bool intersect(Box& dst, const Box& a, const Box& b) noexcept {
    dst.reset(Math::max(a._x0, b._x0),
              Math::max(a._y0, b._y0),
              Math::min(a._x1, b._x1),
              Math::min(a._y1, b._y1));
    return dst.isValid();
  }

  static B2D_INLINE void bound(Box& dst, const Box& a, const Box& b) noexcept {
    dst.reset(Math::min(a._x0, b._x0),
              Math::min(a._y0, b._y0),
              Math::max(a._x1, b._x1),
              Math::max(a._y1, b._y1));
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _x0;
  double _y0;
  double _x1;
  double _y1;
};

// ============================================================================
// [b2d::Rect]
// ============================================================================

//! Rectangle.
struct Rect {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Rect() noexcept = default;
  constexpr Rect(const Rect& other) noexcept = default;

  constexpr explicit Rect(const IntRect& other) noexcept
    : _x(double(other.x())),
      _y(double(other.y())),
      _w(double(other.w())),
      _h(double(other.h())) {}

  constexpr Rect(const Point& origin, const Size& size) noexcept
    : _x(origin.x()),
      _y(origin.y()),
      _w(size.w()),
      _h(size.h()) {}

  constexpr Rect(const IntPoint& origin, const IntSize& size) noexcept
    : _x(double(origin.x())),
      _y(double(origin.y())),
      _w(double(size.w())),
      _h(double(size.h())) {}

  constexpr Rect(double x, double y, double w, double h) noexcept
    : _x(x),
      _y(y),
      _w(w),
      _h(h) {}

  inline explicit Rect(const Box& box) noexcept { resetToBox(box); }
  inline explicit Rect(const IntBox& box) noexcept { resetToBox(box); }

  B2D_INLINE Rect(const Point& pt0, const Point& pt1) noexcept { resetToBox(pt0, pt1); }
  B2D_INLINE Rect(const IntPoint& pt0, const IntPoint& pt1) noexcept { resetToBox(pt0, pt1); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  constexpr bool isValid() const noexcept { return (_w > 0.0) & (_h > 0.0); }

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0, 0.0, 0.0); }
  B2D_INLINE void reset(const Rect& r) noexcept { reset(r.x(), r.y(), r.w(), r.h()); }
  B2D_INLINE void reset(const IntRect& r) noexcept { reset(double(r.x()), double(r.y()), double(r.w()), double(r.h())); }
  B2D_INLINE void reset(const Point& p, const Size& size) noexcept { reset(p.x(), p.y(), size.w(), size.h()); }
  B2D_INLINE void reset(const IntPoint& pt, const IntSize& size) noexcept { reset(double(pt.x()), double(pt.y()), double(size.w()), double(size.h())); }
  B2D_INLINE void reset(double x, double y, double w, double h) noexcept { _x = x; _y = y; _w = w; _h = h; }

  B2D_INLINE void resetToBox(const Box& box) noexcept { resetToBox(box.x0(), box.y0(), box.x1(), box.y1()); }
  B2D_INLINE void resetToBox(const IntBox& box) noexcept { resetToBox(double(box.x0()), double(box.y0()), double(box.x1()), double(box.y1())); }
  B2D_INLINE void resetToBox(const Point& p0, const Point& p1) noexcept { resetToBox(p0.x(), p0.y(), p1.x(), p1.y()); }
  B2D_INLINE void resetToBox(const IntPoint& p0, const IntPoint& p1) noexcept { resetToBox(double(p0.x()), double(p0.y()), double(p1.x()), double(p1.y())); }
  B2D_INLINE void resetToBox(double x0, double y0, double x1, double y1) noexcept { reset(x0, y0, x1 - x0, y1 - y0); }

  B2D_INLINE void resetToNaN() noexcept {
    _x = std::numeric_limits<double>::quiet_NaN();
    _y = std::numeric_limits<double>::quiet_NaN();
    _w = std::numeric_limits<double>::quiet_NaN();
    _h = std::numeric_limits<double>::quiet_NaN();
  }

  constexpr double x() const noexcept { return _x; }
  constexpr double y() const noexcept { return _y; }
  constexpr double w() const noexcept { return _w; }
  constexpr double h() const noexcept { return _h; }

  constexpr double width() const noexcept { return _w; }
  constexpr double height() const noexcept { return _h; }

  constexpr double x0() const noexcept { return _x; }
  constexpr double y0() const noexcept { return _y; }
  constexpr double x1() const noexcept { return _x + _w; }
  constexpr double y1() const noexcept { return _y + _h; }

  constexpr double left() const noexcept { return _x; }
  constexpr double top() const noexcept { return _y; }
  constexpr double right() const noexcept { return _x + _w; }
  constexpr double bottom() const noexcept { return _y + _h; }

  constexpr Point origin() const noexcept { return Point { _x, _y }; }
  constexpr Size size() const noexcept { return Size { _w, _h }; }

  B2D_INLINE void setX(double x) noexcept { _x = x; }
  B2D_INLINE void setY(double y) noexcept { _y = y; }
  B2D_INLINE void setWidth(double w) noexcept { _w = w; }
  B2D_INLINE void setHeight(double h) noexcept { _h = h; }

  B2D_INLINE void setX0(double x0) noexcept { _x = x0; }
  B2D_INLINE void setY0(double y0) noexcept { _y = y0; }
  B2D_INLINE void setX1(double x1) noexcept { _w = x1 - _x; }
  B2D_INLINE void setY1(double y1) noexcept { _h = y1 - _y; }

  B2D_INLINE void setLeft(double x0) noexcept { _x = x0; }
  B2D_INLINE void setTop(double y0) noexcept { _y = y0; }
  B2D_INLINE void setRight(double x1) noexcept { _w = x1 - _x; }
  B2D_INLINE void setBottom(double y1) noexcept { _h = y1 - _y; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translate(const IntPoint& p) noexcept { translate(double(p.x()), double(p.y())); }
  B2D_INLINE void translate(double tx, double ty) noexcept { _x += tx; _y += ty; }

  constexpr Rect translated(const Point& p) const noexcept { return translated(p.x(), p.y()); }
  constexpr Rect translated(const IntPoint& p) const noexcept { return translated(double(p.x()), double(p.y())); }
  constexpr Rect translated(double tx, double ty) const noexcept { return Rect { _x + tx, _y + ty, _w, _h }; }

  B2D_INLINE void adjust(double x, double y) noexcept {
    _x += x; _y += y;
    _w -= x; _h -= y;
    _w -= x; _h -= y;
  }

  B2D_INLINE void adjust(const Point& p) noexcept { adjust(p._x, p._y); }

  B2D_INLINE void adjust(const IntPoint& p) noexcept {
    double x = double(p._x);
    double y = double(p._y);
    adjust(x, y);
  }

  B2D_INLINE Rect adjusted(const Point& p) const noexcept {
    double x = p._x;
    double y = p._y;
    return Rect(_x + x, _y + y, _w - x - x, _h - y - y);
  }

  B2D_INLINE Rect adjusted(const IntPoint& p) const noexcept {
    double x = double(p._x);
    double y = double(p._y);
    return Rect(_x + x, _y + y, _w - x - x, _h - y - y);
  }

  B2D_INLINE Rect adjusted(double x, double y) const noexcept {
    return Rect(_x + x, _y + y, _w - x - x, _h - y - y);
  }

  // --------------------------------------------------------------------------
  // [Algebra]
  // --------------------------------------------------------------------------

  //! Checks if two rectangles overlap.
  //!
  //! Returns `true` if two rectangles overlap, `false` otherwise.
  B2D_INLINE bool overlaps(const Rect& r) const noexcept {
    return (Math::max(x0(), r.x0()) < Math::min(x1(), r.x1())) &
           (Math::max(y0(), r.y0()) < Math::min(y1(), r.y1())) ;
  }

  //! Returns `true` if rectangle completely subsumes `r`.
  B2D_INLINE bool subsumes(const Rect& r) const noexcept {
    return (r.x0() >= x0()) & (r.x1() <= x1()) &
           (r.y0() >= y0()) & (r.y1() <= y1()) ;
  }

  static B2D_INLINE bool intersect(Rect& dst, const Rect& a, const Rect& b) noexcept {
    dst._x = Math::max(a.x0(), b.x0());
    dst._y = Math::max(a.y0(), b.y0());
    dst._w = Math::min(a.x1(), b.x1()) - dst._x;
    dst._h = Math::min(a.y1(), b.y1()) - dst._y;

    return dst.isValid();
  }

  static B2D_INLINE void unite(Rect& dst, const Rect& a, const Rect& b) noexcept {
    dst._x = Math::min(a.x0(), b.x0());
    dst._y = Math::min(a.y0(), b.y0());
    dst._w = Math::max(a.x1(), b.x1()) - dst._x;
    dst._h = Math::max(a.y1(), b.y1()) - dst._y;
  }

  // --------------------------------------------------------------------------
  // [Contains]
  // --------------------------------------------------------------------------

  B2D_INLINE bool contains(const Point& p) const noexcept {
    return contains(p._x, p._y);
  }

  B2D_INLINE bool contains(double px, double py) const noexcept {
    double x0 = _x;
    double y0 = _y;
    double x1 = x0 + _w;
    double y1 = y0 + _h;

    return px >= x0 && py >= y0 && px < x1 && py < y1;
  }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  constexpr bool eq(const Rect& other) const noexcept {
    return eq(other.x(), other.y(), other.w(), other.h());
  }

  constexpr bool eq(double x, double y, double w, double h) const noexcept {
    return (_x == x) & (_y == y) & (_w == w) & (_h == h);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Rect& operator=(const Rect& rect) noexcept = default;
  B2D_INLINE Rect& operator=(const IntRect& rect) noexcept { reset(rect); return *this; }
  B2D_INLINE Rect& operator=(const Box& box) noexcept { resetToBox(box); return *this; }
  B2D_INLINE Rect& operator=(const IntBox& box) noexcept { resetToBox(box); return *this; }

  constexpr Rect operator+(const Point& p) const noexcept { return Rect { _x + p._x, _y + p._y, _w, _h }; }
  constexpr Rect operator-(const Point& p) const noexcept { return Rect { _x - p._x, _y - p._y, _w, _h }; }
  constexpr Rect operator+(const IntPoint& p) const noexcept { return Rect { _x + double(p._x), _y + double(p._y), _w, _h }; }
  constexpr Rect operator-(const IntPoint& p) const noexcept { return Rect { _x - double(p._x), _y - double(p._y), _w, _h }; }

  B2D_INLINE Rect& operator+=(const Point& p) noexcept { translate(p); return *this; }
  B2D_INLINE Rect& operator+=(const IntPoint& p) noexcept { translate(p); return *this; }

  B2D_INLINE Rect& operator-=(const Point& p) noexcept {
    _x -= p._x;
    _y -= p._y;
    return *this;
  }
  B2D_INLINE Rect& operator-=(const IntPoint& p) noexcept {
    _x -= double(p._x);
    _y -= double(p._y);
    return *this;
  }

  constexpr bool operator==(const Rect& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const Rect& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  double _x;
  double _y;
  double _w;
  double _h;
};

B2D_INLINE void IntBox::resetToRect(const IntRect& rect) noexcept { reset(rect._x, rect._y, rect._x + rect._w, rect._y + rect._h); }
B2D_INLINE void Box::resetToRect(const Rect& rect) noexcept { reset(rect.x0(), rect.y0(), rect.x1(), rect.y1()); }
B2D_INLINE void Box::resetToRect(const IntRect& rect) noexcept { reset(double(rect.x()), double(rect.y()), double(rect.x()) + double(rect.w()), double(rect.y()) + double(rect.h())); }

// ============================================================================
// [b2d::Line]
// ============================================================================

//! Line.
struct Line {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Line() noexcept = default;
  constexpr Line(const Line& line) noexcept = default;

  inline explicit Line(const Point p[2]) noexcept
    : _p { p[0], p[1] } {}

  constexpr Line(const Point& p0, const Point& p1) noexcept
    : _p { p0, p1 } {}

  constexpr Line(double x0, double y0, double x1, double y1) noexcept
    : _p { { x0, y0 }, { x1, y1 } } {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0, 0.0, 0.0); }
  B2D_INLINE void reset(const Line& other) noexcept { *this = other; }

  B2D_INLINE void reset(const Point p[2]) noexcept { reset(p[0].x(), p[0].y(), p[1].x(), p[1].y()); }
  B2D_INLINE void reset(const Point& p0, const Point& p1) noexcept { reset(p0.x(), p0.y(), p1.x(), p1.y()); }

  B2D_INLINE void reset(double x0, double y0, double x1, double y1) noexcept {
    _p[0].reset(x0, y0);
    _p[1].reset(x1, y1);
  }

  constexpr double x0() const noexcept { return _p[0].x(); }
  constexpr double y0() const noexcept { return _p[0].y(); }
  constexpr double x1() const noexcept { return _p[1].x(); }
  constexpr double y1() const noexcept { return _p[1].y(); }

  constexpr Point p0() const noexcept { return _p[0]; }
  constexpr Point p1() const noexcept { return _p[1]; }
  constexpr const Point* points() const noexcept { return _p; }

  constexpr double dx() const noexcept { return x1() - x0(); }
  constexpr double dy() const noexcept { return y1() - y0(); }
  constexpr const Point vector() const noexcept { return Point { dx(), dy() }; }

  B2D_INLINE double angle() const noexcept { return std::atan2(y0() - y1(), x1() - x0()); }
  B2D_INLINE double length() const noexcept { return std::hypot(x1() - x0(), y1() - y0()); }

  B2D_INLINE void setP0(const Point& p0) noexcept { _p[0] = p0; }
  B2D_INLINE void setP1(const Point& p1) noexcept { _p[1] = p1; }

  B2D_INLINE void setAngle(double angle) noexcept {
    double aSin = std::sin(angle);
    double aCos = std::cos(angle);
    double len = length();
    _p[1].reset(_p[0]._x + aCos * len, _p[0]._y - aSin * len);
  }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { translate(p.x(), p.y()); }
  B2D_INLINE void translate(double tx, double ty) noexcept {
    _p[0].translate(tx, ty);
    _p[1].translate(tx, ty);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Line& operator=(const Line& other) noexcept = default;

  B2D_INLINE bool operator==(const Line& other) const noexcept { return  Support::inlinedEqT<Line>(this, &other); }
  B2D_INLINE bool operator!=(const Line& other) const noexcept { return !Support::inlinedEqT<Line>(this, &other); }

  // --------------------------------------------------------------------------
  // [Algebra]
  // --------------------------------------------------------------------------

  static B2D_API uint32_t intersect(Point& dst, const Line& a, const Line& b) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _p[2];
};

// ============================================================================
// [b2d::Triangle]
// ============================================================================

//! Triangle.
struct Triangle {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Triangle() noexcept = default;
  constexpr Triangle(const Triangle& triangle) noexcept = default;

  constexpr Triangle(const Point& p0, const Point& p1, const Point& p2) noexcept
    : _p { p0, p1, p2 } {}

  constexpr explicit Triangle(const Point p[3]) noexcept
    : _p { p[0], p[1], p[2] } {}

  constexpr Triangle(double x0, double y0, double x1, double y1, double x2, double y2) noexcept
    : _p { { x0, y0 }, { x1, y1 }, { x2, y2 } } {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _p[0].reset();
    _p[1].reset();
    _p[2].reset();
  }

  B2D_INLINE void reset(const Triangle& other) noexcept { operator=(other); }

  B2D_INLINE void reset(double x0, double y0, double x1, double y1, double x2, double y2) noexcept {
    _p[0].reset(x0, y0);
    _p[1].reset(x1, y1);
    _p[2].reset(x2, y2);
  }

  B2D_INLINE void reset(const Point& p0, const Point& p1, const Point& p2) noexcept {
    _p[0] = p0;
    _p[1] = p1;
    _p[2] = p2;
  }

  constexpr double x0() const noexcept { return _p[0].x(); }
  constexpr double y0() const noexcept { return _p[0].y(); }
  constexpr double x1() const noexcept { return _p[1].x(); }
  constexpr double y1() const noexcept { return _p[1].y(); }
  constexpr double x2() const noexcept { return _p[2].x(); }
  constexpr double y2() const noexcept { return _p[2].y(); }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { translate(p.x(), p.y()); }

  B2D_INLINE void translate(double x, double y) noexcept {
    _p[0].translate(x, y);
    _p[1].translate(x, y);
    _p[2].translate(x, y);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Triangle& operator=(const Triangle& other) noexcept = default;

  B2D_INLINE bool operator==(const Triangle& other) const noexcept { return  Support::inlinedEqT<Triangle>(this, &other); }
  B2D_INLINE bool operator!=(const Triangle& other) const noexcept { return !Support::inlinedEqT<Triangle>(this, &other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _p[3];
};

// ============================================================================
// [b2d::Round]
// ============================================================================

//! Rounded rectangle.
struct Round {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Round() noexcept = default;
  constexpr Round(const Round& round) noexcept = default;

  constexpr Round(const Rect& rect, const Point& r) noexcept
    : _rect(rect),
      _radius(r) {}

  constexpr Round(const Rect& rect, double r) noexcept
    : _rect(rect),
      _radius(r, r) {}

  constexpr Round(const Rect& rect, double rx, double ry) noexcept
    : _rect(rect),
      _radius(rx, ry) {}

  constexpr Round(double x, double y, double w, double h, double r) noexcept
    : _rect(x, y, w, h),
      _radius(r, r) {}

  constexpr Round(double x, double y, double w, double h, double rx, double ry) noexcept
    : _rect(x, y, w, h),
      _radius(rx, ry) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _rect.reset();
    _radius.reset();
  }

  B2D_INLINE void reset(const Round& other) noexcept { operator=(other); }
  B2D_INLINE void reset(const Rect& rect, const Point& r) noexcept { reset(rect.x(), rect.y(), rect.w(), rect.h(), r.x(), r.y()); }
  B2D_INLINE void reset(const Rect& rect, double r) noexcept { reset(rect.x(), rect.y(), rect.w(), rect.h(), r, r); }
  B2D_INLINE void reset(const Rect& rect, double rx, double ry) noexcept { reset(rect.x(), rect.y(), rect.w(), rect.h(), rx, ry); }
  B2D_INLINE void reset(double x, double y, double w, double h, double r) noexcept { reset(x, y, w, h, r, r); }
  B2D_INLINE void reset(double x, double y, double w, double h, double rx, double ry) noexcept { _rect.reset(x, y, w, h); _radius.reset(rx, ry); }

  constexpr Rect rect() const noexcept { return _rect; }
  constexpr double x() const noexcept { return _rect.x(); }
  constexpr double y() const noexcept { return _rect.y(); }
  constexpr double w() const noexcept { return _rect.w(); }
  constexpr double h() const noexcept { return _rect.h(); }

  constexpr Point radius() const noexcept { return _radius; }
  constexpr double rx() const noexcept { return _radius.x(); }
  constexpr double ry() const noexcept { return _radius.y(); }

  B2D_INLINE void setRect(const Rect& rect) noexcept { _rect.reset(rect); }
  B2D_INLINE void setRect(double x, double y, double w, double h) noexcept { _rect.reset(x, y, w, h); }

  B2D_INLINE void setRadius(const Point& r) noexcept { _radius.reset(r.x(), r.y()); }
  B2D_INLINE void setRadius(double r) noexcept { _radius.reset(r, r); }
  B2D_INLINE void setRadius(double rx, double ry) noexcept { _radius.reset(rx, ry); }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& pt) noexcept { _rect.translate(pt); }
  B2D_INLINE void translate(double tx, double ty) noexcept { _rect.translate(tx, ty); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Round& operator=(const Round& other) noexcept = default;

  B2D_INLINE bool operator==(const Round& other) const noexcept { return  Support::inlinedEqT<Round>(this, &other); }
  B2D_INLINE bool operator!=(const Round& other) const noexcept { return !Support::inlinedEqT<Round>(this, &other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Rect _rect;
  Point _radius;
};

// ============================================================================
// [b2d::Circle]
// ============================================================================

//! Circle.
struct Circle {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Circle() noexcept = default;
  constexpr Circle(const Circle& circle) noexcept = default;

  constexpr Circle(const Point& center, double r) noexcept
    : _center(center),
      _radius(r) {}

  constexpr Circle(double x0, double y0, double r) noexcept
    : _center(x0, y0),
      _radius(r) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _center.reset();
    _radius = 0.0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Circle& setCircle(const Circle& circle) noexcept {
    _center = circle._center;
    _radius = circle._radius;
    return *this;
  }

  B2D_INLINE Circle& setCircle(const Point& center, double r) noexcept {
    _center = center;
    _radius = r;
    return *this;
  }

  B2D_INLINE Circle& setCircle(double x0, double y0, double r) noexcept {
    _center.reset(x0, y0);
    _radius = r;
    return *this;
  }

  constexpr const Point& center() const noexcept { return _center; }
  constexpr double cx() const noexcept { return _center.x(); }
  constexpr double cy() const noexcept { return _center.y(); }
  constexpr double radius() const noexcept { return _radius; }

  B2D_INLINE void setCenter(const Point& p) noexcept { _center = p; }
  B2D_INLINE void setCenter(double x, double y) noexcept { _center.reset(x, y); }
  B2D_INLINE void setRadius(double r) noexcept { _radius = r; }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { _center.translate(p); }
  B2D_INLINE void translate(double tx, double ty) noexcept { _center.translate(tx, ty); }

  // --------------------------------------------------------------------------
  // [ToCSpline]
  // --------------------------------------------------------------------------

  B2D_API uint32_t toCSpline(Point* dst) const noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Circle& operator=(const Circle& other) noexcept = default;

  B2D_INLINE bool operator==(const Circle& other) const noexcept { return  Support::inlinedEqT<Circle>(this, &other); }
  B2D_INLINE bool operator!=(const Circle& other) const noexcept { return !Support::inlinedEqT<Circle>(this, &other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _center;
  double _radius;
};

// ============================================================================
// [b2d::Ellipse]
// ============================================================================

//! Ellipse.
struct Ellipse {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Ellipse() noexcept = default;
  constexpr Ellipse(const Ellipse& ellipse) noexcept = default;

  constexpr Ellipse(const Point& center, const Point& r) noexcept
    : _center(center),
      _radius(r) {}

  constexpr Ellipse(const Point& center, double r) noexcept
    : _center(center),
      _radius(r, r) {}

  constexpr Ellipse(double x0, double y0, double r) noexcept
    : _center(x0, y0),
      _radius(r, r) {}

  constexpr Ellipse(double x0, double y0, double rx, double ry) noexcept
    : _center(x0, y0),
      _radius(rx, ry) {}

  static B2D_INLINE Ellipse fromCircle(const Circle& circle) noexcept {
    return Ellipse(circle._center, circle._radius);
  }

  static B2D_INLINE Ellipse fromRect(const Rect& rect) noexcept {
    double rx = rect._w * 0.5;
    double ry = rect._h * 0.5;
    return Ellipse(rect._x + rx, rect._y + ry, rx, ry);
  }

  static B2D_INLINE Ellipse fromBox(const Box& box) noexcept {
    double rx = (box._x1 - box._x0) * 0.5;
    double ry = (box._y1 - box._y0) * 0.5;
    return Ellipse(box._x0 + rx, box._y0 + ry, rx, ry);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _center.reset();
    _radius.reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE Ellipse& setEllipse(const Ellipse& ellipse) noexcept {
    _center = ellipse._center;
    _radius = ellipse._radius;
    return *this;
  }

  B2D_INLINE Ellipse& setEllipse(const Point& center, double r) noexcept {
    _center = center;
    _radius.reset(r, r);
    return *this;
  }

  B2D_INLINE Ellipse& setEllipse(const Point& center, const Point& r) noexcept {
    _center = center;
    _radius = r;
    return *this;
  }

  B2D_INLINE Ellipse& setEllipse(double x0, double y0, double r) noexcept {
    _center.reset(x0, y0);
    _radius.reset(r, r);
    return *this;
  }

  B2D_INLINE Ellipse& setEllipse(double x0, double y0, double rx, double ry) noexcept {
    _center.reset(x0, y0);
    _radius.reset(rx, ry);
    return *this;
  }

  B2D_INLINE Ellipse& setRect(const Rect& rect) noexcept {
    double rx = rect._w * 0.5;
    double ry = rect._h * 0.5;

    _center.reset(rect._x + rx, rect._y + ry);
    _radius.reset(rx, ry);
    return *this;
  }

  B2D_INLINE Ellipse& setBox(const Box& box) noexcept {
    double rx = (box._x1 - box._x0) * 0.5;
    double ry = (box._y1 - box._y0) * 0.5;

    _center.reset(box._x0 + rx, box._y0 + ry);
    _radius.reset(rx, ry);
    return *this;
  }

  B2D_INLINE Ellipse& setCircle(const Circle& circle) noexcept {
    _center = circle._center;
    _radius.reset(circle._radius, circle._radius);
    return *this;
  }

  constexpr const Point& center() const noexcept { return _center; }
  constexpr const Point& radius() const noexcept { return _radius; }

  B2D_INLINE Ellipse& setCenter(const Point& center) noexcept { _center = center; return *this; }
  B2D_INLINE Ellipse& setRadius(const Point& r) noexcept { _radius = r; return *this; }
  B2D_INLINE Ellipse& setRadius(double r) noexcept { _radius.reset(r, r); return *this; }

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { _center.translate(p); }
  B2D_INLINE void translate(double tx, double ty) noexcept { _center.translate(tx, ty); }

  // --------------------------------------------------------------------------
  // [ToCSpline]
  // --------------------------------------------------------------------------

  B2D_API uint32_t toCSpline(Point* dst) const noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Ellipse& operator=(const Ellipse& other) noexcept {
    setEllipse(other);
    return *this;
  }

  B2D_INLINE bool operator==(const Ellipse& other) const noexcept { return  Support::inlinedEqT<Ellipse>(this, &other); }
  B2D_INLINE bool operator!=(const Ellipse& other) const noexcept { return !Support::inlinedEqT<Ellipse>(this, &other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _center;
  Point _radius;
};

// ============================================================================
// [b2d::Arc]
// ============================================================================

//! Arc base.
struct Arc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Arc() noexcept = default;
  constexpr Arc(const Arc& other) noexcept = default;

  constexpr Arc(const Point& center, double r, double start, double sweep) noexcept
    : _center(center),
      _radius(r, r),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(double cx, double cy, double r, double start, double sweep) noexcept
    : _center(cx, cy),
      _radius(r, r),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(const Point& cp, const Point& rp, double start, double sweep) noexcept
    : _center(cp),
      _radius(rp),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(double cx, double cy, const Point& rp, double start, double sweep) noexcept
    : _center(cx, cy),
      _radius(rp),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(double cx, double cy, double rx, double ry, double start, double sweep) noexcept
    : _center(cx, cy),
      _radius(rx, ry),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(const Rect& rect, double start, double sweep) noexcept
    : _center(rect.x() + rect.w() * 0.5,
              rect.y() + rect.h() * 0.5),
      _radius(rect.w()  * 0.5,
              rect.h() * 0.5),
      _start(start),
      _sweep(sweep) {}

  constexpr Arc(const Box& box, double start, double sweep) noexcept
    : _center(box.x1() * 0.5 + box.x0() * 0.5,
              box.y1() * 0.5 + box.y0() * 0.5),
      _radius(box.x1() * 0.5 - box.x0() * 0.5,
              box.y1() * 0.5 - box.y0() * 0.5),
      _start(start),
      _sweep(sweep) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { reset(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); }
  B2D_INLINE void reset(const Arc& other) noexcept { *this = other; }

  B2D_INLINE void reset(const Point& c, double r, double start, double sweep) noexcept { reset(c.x(), c.y(), r, r, start, sweep); }
  B2D_INLINE void reset(const Point& c, const Point& r, double start, double sweep) noexcept { reset(c.x(), c.y(), r.x(), r.y(), start, sweep); }
  B2D_INLINE void reset(double cx, double cy, double r, double start, double sweep) noexcept { reset(cx, cy, r, r, start, sweep); }

  B2D_INLINE void reset(double cx, double cy, double rx, double ry, double start, double sweep) noexcept {
    _center.reset(cx, cy);
    _radius.reset(rx, ry);
    _start = start;
    _sweep = sweep;
  }

  // TODO: Verify these and remove if not needed.
  B2D_INLINE Arc& setEllipse(const Circle& circle) noexcept {
    _center = circle._center;
    _radius.reset(circle._radius, circle._radius);
    return *this;
  }

  B2D_INLINE Arc& setEllipse(const Ellipse& ellipse) noexcept {
    _center = ellipse._center;
    _radius = ellipse._radius;
    return *this;
  }

  B2D_INLINE Arc& setEllipse(const Point& center, double r) noexcept {
    _center = center;
    _radius.reset(r, r);
    return *this;
  }

  B2D_INLINE Arc& setEllipse(const Point& center, const Point& r) noexcept {
    _center = center;
    _radius = r;
    return *this;
  }

  B2D_INLINE Arc& setEllipse(const Rect& rect) noexcept {
    double rx = rect._w * 0.5;
    double ry = rect._h * 0.5;

    _radius.reset(rx, ry);
    _center.reset(rect._x + _radius._x, rect._y + _radius._y);
    return *this;
  }

  B2D_INLINE Arc& setEllipse(const Box& box) noexcept {
    double rx = (box._x1 - box._x0) * 0.5;
    double ry = (box._y1 - box._y0) * 0.5;

    _radius.reset(rx, ry);
    _center.reset(box._x0 + rx, box._y0 + ry);
    return *this;
  }

  constexpr const Point& center() const noexcept { return _center; }
  constexpr const Point& radius() const noexcept { return _radius; }
  constexpr double start() const noexcept { return _start; }
  constexpr double sweep() const noexcept { return _sweep; }

  B2D_INLINE void setCenter(const Point& cp) noexcept { _center = cp; }
  B2D_INLINE void setRadius(const Point& rp) noexcept { _radius = rp; }
  B2D_INLINE void setRadius(double r) noexcept { _radius.reset(r, r); }
  B2D_INLINE void setRadius(double rx, double ry) noexcept { _radius.reset(rx, ry); }

  B2D_INLINE void setStart(double start) noexcept { _start = start; }
  B2D_INLINE void setSweep(double sweep) noexcept { _sweep = sweep; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  B2D_INLINE void translate(const Point& p) noexcept { _center.translate(p); }
  B2D_INLINE void translate(double tx, double ty) noexcept { _center.translate(tx, ty); }

  constexpr bool eq(const Arc& other) const noexcept {
    return _center == other._center &&
           _radius == other._radius &&
           _start  == other._start  &&
           _sweep  == other._sweep  ;
  }

  // --------------------------------------------------------------------------
  // [ToCSpline]
  // --------------------------------------------------------------------------

  B2D_API uint32_t toCSpline(Point* dst) const noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Arc& operator=(const Arc& other) noexcept = default;

  constexpr bool operator==(const Arc& other) const noexcept { return  eq(other); }
  constexpr bool operator!=(const Arc& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Point _center;
  Point _radius;
  double _start;
  double _sweep;
};

// ============================================================================
// [b2d::Chord]
// ============================================================================

//! Chord.
struct Chord : public Arc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Chord() noexcept = default;
  constexpr Chord(const Chord& other) noexcept = default;

  constexpr explicit Chord(const Arc& arc) noexcept : Arc(arc) {}

  constexpr Chord(const Point& cp, double r, double start, double sweep) noexcept : Arc(cp, r, start, sweep) {}
  constexpr Chord(double cx, double cy, double r, double start, double sweep) noexcept : Arc(cx, cy, r, start, sweep) {}
  constexpr Chord(const Point& cp, const Point& rp, double start, double sweep) noexcept : Arc(cp, rp, start, sweep) {}
  constexpr Chord(double cx, double cy, const Point& rp, double start, double sweep) noexcept : Arc(cx, cy, rp, start, sweep) {}
  constexpr Chord(double cx, double cy, double rx, double ry, double start, double sweep) noexcept : Arc(cx, cy, rx, ry, start, sweep) {}
  constexpr Chord(const Rect& rect, double start, double sweep) noexcept : Arc(rect, start, sweep) {}
  constexpr Chord(const Box& box, double start, double sweep) noexcept : Arc(box, start, sweep) {}
};

// ============================================================================
// [b2d::Pie]
// ============================================================================

//! Pie.
struct Pie : public Arc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Pie() noexcept = default;
  constexpr Pie(const Pie& other) noexcept = default;

  constexpr explicit Pie(const Arc& arc) noexcept : Arc(arc) {}

  constexpr Pie(const Point& cp, double r, double start, double sweep) noexcept : Arc(cp, r, start, sweep) {}
  constexpr Pie(double cx, double cy, double r, double start, double sweep) noexcept : Arc(cx, cy, r, start, sweep) {}
  constexpr Pie(const Point& cp, const Point& rp, double start, double sweep) noexcept : Arc(cp, rp, start, sweep) {}
  constexpr Pie(double cx, double cy, const Point& rp, double start, double sweep) noexcept : Arc(cx, cy, rp, start, sweep) {}
  constexpr Pie(double cx, double cy, double rx, double ry, double start, double sweep) noexcept : Arc(cx, cy, rx, ry, start, sweep) {}
  constexpr Pie(const Rect& rect, double start, double sweep) noexcept : Arc(rect, start, sweep) {}
  constexpr Pie(const Box& box, double start, double sweep) noexcept : Arc(box, start, sweep) {}
};

// ============================================================================
// [b2d::Math [Specialization]]
// ============================================================================

namespace Math {

static B2D_INLINE double lengthSq(const Point& vec) noexcept { return lengthSq(vec.x(), vec.y()); }
static B2D_INLINE double lengthSq(const Point& p0, const Point& p1) noexcept { return lengthSq(p0.x(), p0.y(), p1.x(), p1.y()); }

static B2D_INLINE double length(const Point& vec) noexcept { return length(vec.x(), vec.y()); }
static B2D_INLINE double length(const Point& p0, const Point& p1) noexcept { return length(p0.x(), p0.y(), p1.x(), p1.y()); }

template<> constexpr double dot<Point>(const Point& a, const Point& b) noexcept { return a.x() * b.x() + a.y() * b.y(); }
template<> constexpr double cross<Point>(const Point& a, const Point& b) noexcept { return a.x() * b.y() - a.y() * b.x(); }

template<> constexpr IntPoint abs<IntPoint>(const IntPoint& x) noexcept { return IntPoint { abs(x.x()), abs(x.y()) }; }
template<> constexpr IntPoint min<IntPoint>(const IntPoint& a, const IntPoint& b) noexcept { return IntPoint { min(a.x(), b.x()), min(a.y(), b.y()) }; }
template<> constexpr IntPoint max<IntPoint>(const IntPoint& a, const IntPoint& b) noexcept { return IntPoint { max(a.x(), b.x()), max(a.y(), b.y()) }; }

template<> constexpr IntSize abs<IntSize>(const IntSize& x) noexcept { return IntSize { abs(x.w()), abs(x.h()) }; }
template<> constexpr IntSize min<IntSize>(const IntSize& a, const IntSize& b) noexcept { return IntSize { min(a.w(), b.w()), min(a.h(), b.h()) }; }
template<> constexpr IntSize max<IntSize>(const IntSize& a, const IntSize& b) noexcept { return IntSize { max(a.w(), b.w()), max(a.h(), b.h()) }; }

template<> constexpr IntRect min<IntRect>(const IntRect& a, const IntRect& b) noexcept { return IntRect { min(a.x(), b.x()), min(a.y(), b.y()), min(a.w(), b.w()), min(a.h(), b.h()) }; }
template<> constexpr IntRect max<IntRect>(const IntRect& a, const IntRect& b) noexcept { return IntRect { max(a.x(), b.x()), max(a.y(), b.y()), max(a.w(), b.w()), max(a.h(), b.h()) }; }

template<> constexpr IntBox min<IntBox>(const IntBox& a, const IntBox& b) noexcept { return IntBox { min(a.x0(), b.x0()), min(a.y0(), b.y0()), min(a.x1(), b.x1()), min(a.y1(), b.y1()) }; }
template<> constexpr IntBox max<IntBox>(const IntBox& a, const IntBox& b) noexcept { return IntBox { max(a.x0(), b.x0()), max(a.y0(), b.y0()), max(a.x1(), b.x1()), max(a.y1(), b.y1()) }; }

template<> constexpr Point abs<Point>(const Point& x) noexcept { return Point { abs(x.x()), abs(x.y()) }; }
template<> constexpr Point min<Point>(const Point& a, const Point& b) noexcept { return Point { min(a.x(), b.x()), min(a.y(), b.y()) }; }
template<> constexpr Point max<Point>(const Point& a, const Point& b) noexcept { return Point { max(a.x(), b.x()), max(a.y(), b.y()) }; }
template<> constexpr Point lerp<Point>(const Point& a, const Point& b, double t) noexcept { return Point { lerp(a.x(), b.x(), t), lerp(a.y(), b.y(), t) }; }

template<> constexpr Size abs<Size>(const Size& x) noexcept { return Size { abs(x.w()), abs(x.h()) }; }
template<> constexpr Size min<Size>(const Size& a, const Size& b) noexcept { return Size { min(a.w(), b.w()), min(a.h(), b.h()) }; }
template<> constexpr Size max<Size>(const Size& a, const Size& b) noexcept { return Size { max(a.w(), b.w()), max(a.h(), b.h()) }; }
template<> constexpr Size lerp<Size>(const Size& a, const Size& b, double t) noexcept { return Size { lerp(a.w(), b.w(), t), lerp(a.h(), b.h(), t) }; }

template<> constexpr Rect min<Rect>(const Rect& a, const Rect& b) noexcept { return Rect { min(a.x(), b.x()), min(a.y(), b.y()), min(a.w(), b.w()), min(a.h(), b.h()) }; }
template<> constexpr Rect max<Rect>(const Rect& a, const Rect& b) noexcept { return Rect { max(a.x(), b.x()), max(a.y(), b.y()), max(a.w(), b.w()), max(a.h(), b.h()) }; }
template<> constexpr Rect lerp<Rect>(const Rect& a, const Rect& b, double t) noexcept { return Rect { lerp(a.x(), b.x(), t), lerp(a.y(), b.y(), t), lerp(a.w(), b.w(), t), lerp(a.h(), b.h(), t) }; }

template<> constexpr Box min<Box>(const Box& a, const Box& b) noexcept { return Box { min(a.x0(), b.x0()), min(a.y0(), b.y0()), min(a.x1(), b.x1()), min(a.y1(), b.y1()) }; }
template<> constexpr Box max<Box>(const Box& a, const Box& b) noexcept { return Box { max(a.x0(), b.x0()), max(a.y0(), b.y0()), max(a.x1(), b.x1()), max(a.y1(), b.y1()) }; }
template<> constexpr Box lerp<Box>(const Box& a, const Box& b, double t) noexcept { return Box { lerp(a.x0(), b.x0(), t), lerp(a.y0(), b.y0(), t), lerp(a.x1(), b.x1(), t), lerp(a.y1(), b.y1(), t) }; }

template<> constexpr Round lerp<Round>(const Round& a, const Round& b, double t) noexcept { return Round { lerp(a.rect(), b.rect(), t), lerp(a.radius(), b.radius(), t) }; }
template<> constexpr Circle lerp<Circle>(const Circle& a, const Circle& b, double t) noexcept { return Circle { lerp(a.center(), b.center(), t), lerp(a.radius(), b.radius(), t) }; }
template<> constexpr Ellipse lerp<Ellipse>(const Ellipse& a, const Ellipse& b, double t) noexcept { return Ellipse { lerp(a.center(), b.center(), t), lerp(a.radius(), b.radius(), t) }; }

} // Math namespace

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_GEOMTYPES_H
