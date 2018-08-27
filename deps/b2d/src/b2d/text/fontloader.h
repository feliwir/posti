// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTLOADER_H
#define _B2D_TEXT_FONTLOADER_H

// [Dependencies]
#include "../core/any.h"
#include "../core/array.h"
#include "../core/buffer.h"
#include "../text/fontdata.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontLoader - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontLoaderImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontLoaderImpl)

  B2D_API FontLoaderImpl(uint32_t type) noexcept;
  B2D_API virtual ~FontLoaderImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t type() const noexcept { return _type; }
  B2D_INLINE uint32_t flags() const noexcept { return _flags; }
  B2D_INLINE uint32_t faceCount() const noexcept { return uint32_t(_fontData.size()); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual FontDataImpl* fontData(uint32_t faceIndex) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _type;
  uint32_t _flags;

  Array<FontData> _fontData;
};

// ============================================================================
// [b2d::FontLoader]
// ============================================================================

class FontLoader : public Object {
public:
  enum : uint32_t { kTypeId = Any::kTypeIdFontLoader };

  typedef void (B2D_CDECL* DestroyHandler)(FontLoaderImpl* impl, void* destroyData);

  enum Type : uint32_t {
    kTypeNone   = 0,
    kTypeFile   = 1,
    kTypeMemory = 2,
    kTypeCustom = 3
  };

  enum Flags : uint32_t {
    kFlagCollection       = 0x00000001u, //!< Contains more faces than one.
    kFlagUnloadable       = 0x80000000u
  };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE FontLoader() noexcept
    : Object(none().impl()) {}

  B2D_INLINE FontLoader(const FontLoader& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE FontLoader(FontLoader&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit FontLoader(FontLoaderImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const FontLoader& none() noexcept { return Any::noneOf<FontLoader>(); }

  B2D_API Error createFromFile(
    const char* fileName) noexcept;

  B2D_API Error createFromBuffer(
    const Buffer& buffer) noexcept;

  B2D_API Error createFromMemory(
    const void* buffer,
    size_t size,
    DestroyHandler destroyHandler = nullptr,
    void* destroyData = nullptr) noexcept;

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = FontLoaderImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t type() const noexcept { return impl()->type(); }
  B2D_INLINE uint32_t flags() const noexcept { return impl()->flags(); }

  B2D_INLINE bool hasFlag(uint32_t flag) const noexcept { return (flags() & flag) != 0; }
  B2D_INLINE bool isCollection() const noexcept { return hasFlag(kFlagCollection); }
  B2D_INLINE bool isUnloadable() const noexcept { return hasFlag(kFlagUnloadable); }

  B2D_INLINE uint32_t faceCount() const noexcept { return impl()->faceCount(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE FontDataImpl* _fontDataImpl(uint32_t faceIndex) const noexcept { return impl()->fontData(faceIndex); }
  B2D_INLINE FontData fontData(uint32_t faceIndex) const noexcept { return FontData(_fontDataImpl(faceIndex)); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FontLoader& operator=(const FontLoader& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE FontLoader& operator=(FontLoader&& other) noexcept {
    FontLoaderImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};


//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTLOADER_H
