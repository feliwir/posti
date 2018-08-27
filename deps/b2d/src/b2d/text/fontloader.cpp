// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/buffer.h"
#include "../core/filesystem.h"
#include "../core/math.h"
#include "../core/runtime.h"
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/fontloader.h"
#include "../text/otsfnt_p.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Font - Globals]
// ============================================================================

static Wrap<FontLoaderImpl> gFontLoaderImplNone;

// ============================================================================
// [b2d::FontLoader - Impl]
// ============================================================================

FontLoaderImpl::FontLoaderImpl(uint32_t type) noexcept
  : ObjectImpl(Any::kTypeIdFontLoader),
    _type(type),
    _flags(0) {}
FontLoaderImpl::~FontLoaderImpl() noexcept {}

FontDataImpl* FontLoaderImpl::fontData(uint32_t faceIndex) noexcept {
  return faceIndex < _fontData.size()
    ?_fontData[faceIndex].impl()->addRef<FontDataImpl>()
    : FontData::none().impl();
}

// ============================================================================
// [b2d::MemFontLoader - Impl]
// ============================================================================

//! \internal
//!
//! FontLoader implementation that uses memory buffer.
class MemFontLoaderImpl final : public FontLoaderImpl {
public:
  B2D_NONCOPYABLE(MemFontLoaderImpl)

  typedef FontLoader::DestroyHandler DestroyHandler;

  MemFontLoaderImpl(const Buffer& buffer) noexcept;
  MemFontLoaderImpl(const void* buffer, size_t size, DestroyHandler destroyHandler, void* destroyData) noexcept;

  virtual ~MemFontLoaderImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Handlers]
  // --------------------------------------------------------------------------

  Error init() noexcept;

  static void B2D_CDECL destroyBufferHandler(FontLoaderImpl* impl, void* destroyData) noexcept {
    B2D_UNUSED(impl);
    static_cast<BufferImpl*>(destroyData)->release();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const uint8_t* _buffer;
  size_t _size;
  DestroyHandler _destroyHandler;
  void* _destroyData;
};

MemFontLoaderImpl::MemFontLoaderImpl(const Buffer& buffer) noexcept
  : FontLoaderImpl(FontLoader::kTypeMemory),
    _buffer(buffer.data()),
    _size(buffer.size()),
    _destroyHandler(destroyBufferHandler),
    _destroyData(buffer.impl()->addRef()) {}

MemFontLoaderImpl::MemFontLoaderImpl(const void* buffer, size_t size, DestroyHandler destroyHandler, void* destroyData) noexcept
  : FontLoaderImpl(FontLoader::kTypeMemory),
    _buffer(static_cast<const uint8_t*>(buffer)),
    _size(size),
    _destroyHandler(destroyHandler),
    _destroyData(destroyData) {}

MemFontLoaderImpl::~MemFontLoaderImpl() noexcept {
  if (_destroyHandler)
    _destroyHandler(this, _destroyData);
}

// Initializes a single face (.ttf | .otf) or a collection of faces (.ttc | .otc).
Error MemFontLoaderImpl::init() noexcept {
  const uint8_t* rawData = _buffer;
  size_t rawDataSize = _size;

  if (B2D_UNLIKELY(rawDataSize < 4))
    return DebugUtils::errored(kErrorFontInvalidData);

  uint32_t headerTag = reinterpret_cast<const OT::OTTag*>(rawData)->value();
  uint32_t faceCount = 1;

  const OT::OTUInt32* offsetArray = nullptr;
  if (headerTag == OT::TTCHeader::kTag) {
    if (B2D_UNLIKELY(rawDataSize < OT::TTCHeader::kMinSize))
      return DebugUtils::errored(kErrorFontInvalidData);

    const OT::TTCHeader* header = reinterpret_cast<const OT::TTCHeader*>(rawData);

    faceCount = header->numFonts();
    if (B2D_UNLIKELY(!faceCount || faceCount > FontFace::kIndexLimit))
      return DebugUtils::errored(kErrorFontInvalidData);

    size_t ttcHeaderSize = header->calcSize(faceCount);
    if (B2D_UNLIKELY(ttcHeaderSize < rawDataSize))
      return DebugUtils::errored(kErrorFontInvalidData);

    offsetArray = header->offsetArray();
    _flags |= FontLoader::kFlagCollection;
  }
  else {
    if (headerTag != OT::SFNTHeader::kVersionTagOpenType  &&
        headerTag != OT::SFNTHeader::kVersionTagTrueTypeA &&
        headerTag != OT::SFNTHeader::kVersionTagTrueTypeB)
      return DebugUtils::errored(kErrorFontInvalidSignature);
  }

  B2D_PROPAGATE(_fontData.reserve(faceCount));

  for (uint32_t faceIndex = 0; faceIndex < faceCount; faceIndex++) {
    uint32_t faceOffset = 0;
    if (offsetArray)
      faceOffset = offsetArray[faceIndex].value();

    if (B2D_UNLIKELY(faceOffset >= rawDataSize))
      return DebugUtils::errored(kErrorFontInvalidData);

    size_t faceDataSize = rawDataSize - faceOffset;
    if (B2D_UNLIKELY(faceDataSize < OT::SFNTHeader::kMinSize))
      return DebugUtils::errored(kErrorFontInvalidData);

    const OT::SFNTHeader* sfnt = reinterpret_cast<const OT::SFNTHeader*>(rawData + faceOffset);
    uint32_t versionTag = sfnt->versionTag();

    // Version check - accept only TrueType and OpenType fonts.
    if (versionTag != OT::SFNTHeader::kVersionTagOpenType  &&
        versionTag != OT::SFNTHeader::kVersionTagTrueTypeA &&
        versionTag != OT::SFNTHeader::kVersionTagTrueTypeB)
      return DebugUtils::errored(kErrorFontInvalidData);

    // Check table count.
    uint32_t tableCount = sfnt->numTables();
    if (faceDataSize < sizeof(OT::SFNTHeader) + tableCount * sizeof(OT::SFNTHeader::TableRecord))
      return DebugUtils::errored(kErrorFontInvalidData);

    FontDataImpl* fontDataI = FontDataImpl::newFromMemory(_buffer, _size);
    if (B2D_UNLIKELY(!fontDataI))
      return DebugUtils::errored(kErrorNoMemory);

    Error err = fontDataI->init();
    if (B2D_UNLIKELY(err)) {
      fontDataI->release();
      return err;
    }

    // Cannot fail as we reserved enough space for data of all font-faces.
    _fontData.append(FontData(fontDataI));
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::FontLoader - Construction / Destruction]
// ============================================================================

static Error FontLoader_created(FontLoader* self, MemFontLoaderImpl* newI) noexcept {
  if (!newI)
    return DebugUtils::errored(kErrorNoMemory);

  Error err = newI->init();
  if (err) {
    newI->destroy();
    return err;
  }
  else {
    return AnyInternal::replaceImpl(self, newI);
  }
}

B2D_API Error FontLoader::createFromFile(const char* fileName) noexcept {
  Buffer buffer;
  B2D_PROPAGATE(FileUtils::readFile(fileName, buffer));
  return FontLoader_created(this, AnyInternal::newImplT<MemFontLoaderImpl>(buffer));
}

B2D_API Error FontLoader::createFromBuffer(const Buffer& buffer) noexcept {
  return FontLoader_created(this, AnyInternal::newImplT<MemFontLoaderImpl>(buffer));
}

B2D_API Error FontLoader::createFromMemory(const void* buffer, size_t size, DestroyHandler destroyHandler, void* destroyData) noexcept {
  return FontLoader_created(this, AnyInternal::newImplT<MemFontLoaderImpl>(buffer, size, destroyHandler, destroyData));
}

// ============================================================================
// [b2d::FontLoader - Runtime Handlers]
// ============================================================================

void FontLoaderOnInit(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  FontLoaderImpl* fontLoaderI = gFontLoaderImplNone.init(FontLoader::kTypeNone);
  fontLoaderI->_commonData._setToBuiltInNone();
  Any::_initNone(Any::kTypeIdFontLoader, fontLoaderI);
}

B2D_END_NAMESPACE
