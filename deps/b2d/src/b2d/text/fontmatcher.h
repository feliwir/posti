// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTMATCHER_H
#define _B2D_TEXT_FONTMATCHER_H

// [Dependencies]
#include "../core/any.h"
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontMatcher - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontMatcherImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontMatcherImpl)

  B2D_API FontMatcherImpl() noexcept;
  B2D_API virtual ~FontMatcherImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  // TODO:

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _flags;
};

// ============================================================================
// [b2d::FontMatcher]
// ============================================================================

class FontMatcher : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdFontMatcher };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE FontMatcher() noexcept
    : Object(none().impl()) {}

  B2D_INLINE FontMatcher(const FontMatcher& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE FontMatcher(FontMatcher&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit FontMatcher(FontMatcherImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const FontMatcher& none() noexcept { return Any::noneOf<FontMatcher>(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T = FontMatcherImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  // TODO:

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FontMatcher& operator=(const FontMatcher& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE FontMatcher& operator=(FontMatcher&& other) noexcept {
    FontMatcherImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};


//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTMATCHER_H
