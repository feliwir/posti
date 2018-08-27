// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONT_H
#define _B2D_TEXT_FONT_H

// [Dependencies]
#include "../core/any.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/fontmatrix.h"
#include "../text/fontmetrics.h"
#include "../text/glyphrun.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::Font - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API explicit FontImpl(const FontFace& face, float size) noexcept;
  B2D_API virtual ~FontImpl() noexcept;

  static B2D_INLINE FontImpl* new_(const FontFace& face, float size) noexcept {
    return AnyInternal::newImplT<FontImpl>(face, size);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const FontFace& face() const noexcept { return _face; }

  B2D_INLINE float size() const noexcept { return _size; }
  B2D_INLINE const FontMatrix& matrix() const noexcept { return _matrix; }
  B2D_INLINE const FontMetrics& metrics() const noexcept { return _metrics; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FontFace _face;
  float _size;
  FontMatrix _matrix;
  FontMetrics _metrics;
};

// ============================================================================
// [b2d::Font]
// ============================================================================

class Font : public Object {
public:
  enum : uint32_t {
    kTypeId = Any::kTypeIdFont
  };

  //! Font weight.
  enum Weight : uint32_t {
    kWeightThin            = 100,
    kWeightExtraLight      = 200,
    kWeightLight           = 300,
    kWeightSemiLight       = 350,
    kWeightNormal          = 400,
    kWeightMedium          = 500,
    kWeightSemiBold        = 600,
    kWeightBold            = 700,
    kWeightExtraBold       = 800,
    kWeightBlack           = 900,
    kWeightExtraBlack      = 950
  };

  //! Font stretch.
  enum Stretch : uint32_t {
    kStretchUltraCondensed = 1,
    kStretchExtraCondensed = 2,
    kStretchCondensed      = 3,
    kStretchSemiCondensed  = 4,
    kStretchNormal         = 5,
    kStretchSemiExpanded   = 6,
    kStretchExpanded       = 7,
    kStretchExtraExpanded  = 8,
    kStretchUltraExpanded  = 9
  };

  //! Font style.
  enum Style : uint32_t {
    kStyleNormal           = 0,
    kStyleOblique          = 1,
    kStyleItalic           = 2
  };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE Font() noexcept
    : Object(none().impl()) {}

  B2D_INLINE Font(const Font& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE Font(Font&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit Font(FontImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const Font& none() noexcept { return Any::noneOf<Font>(); }

  // --------------------------------------------------------------------------
  // [Create]
  // --------------------------------------------------------------------------

  B2D_API Error createFromFace(const FontFace& face, float size) noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE Error reset() noexcept { return AnyInternal::replaceImpl(this, none().impl()); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = FontImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const FontFace& face() const noexcept { return impl()->face(); }
  B2D_INLINE uint32_t unitsPerEm() const noexcept { return face().unitsPerEm(); }

  B2D_INLINE float size() const noexcept { return impl()->size(); }
  B2D_INLINE const FontMatrix& matrix() const noexcept { return impl()->matrix(); }
  B2D_INLINE const FontMetrics& metrics() const noexcept { return impl()->metrics(); }
  B2D_INLINE const FontDesignMetrics& designMetrics() const noexcept { return face().designMetrics(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API Error decodeGlyphRunOutlines(const GlyphRun& glyphRun, const Matrix2D& userMatrix, Path2D& out) noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE Font& operator=(const Font& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE Font& operator=(Font&& other) noexcept {
    FontImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONT_H
