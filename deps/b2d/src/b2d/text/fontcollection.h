// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTCOLLECTION_H
#define _B2D_TEXT_FONTCOLLECTION_H

// [Dependencies]
#include "../core/any.h"
#include "../text/font.h"
#include "../text/fontenumerator.h"
#include "../text/fontloader.h"
#include "../text/fontmatcher.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontCollection - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontCollectionImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontCollectionImpl)

  B2D_API FontCollectionImpl() noexcept;
  B2D_API virtual ~FontCollectionImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  // TODO:
};

// ============================================================================
// [b2d::FontCollection]
// ============================================================================

class FontCollection : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdFontCollection };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE FontCollection() noexcept
    : Object(none().impl()) {}

  B2D_INLINE FontCollection(const FontCollection& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE FontCollection(FontCollection&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  static B2D_INLINE const FontCollection& none() noexcept { return Any::noneOf<FontCollection>(); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = FontCollectionImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FontCollection& operator=(const FontCollection& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE FontCollection& operator=(FontCollection&& other) noexcept {
    FontCollectionImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTCOLLECTION_H
