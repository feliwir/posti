// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"
#include "../core/unicodeio_p.h"
#include "../text/glyphengine.h"
#include "../text/textbuffer.h"

B2D_BEGIN_NAMESPACE

// TODO: Move to some support namespace.
template<typename T>
static B2D_INLINE size_t strlenT(const T* str) noexcept {
  const T* p = str;
  while (*p)
    p++;
  return (size_t)(p - str);
}

// ============================================================================
// [b2d::TextBuffer - Storage]
// ============================================================================

constexpr size_t kTextBufferItemSize = sizeof(GlyphItem) + sizeof(Point);

Error TextBuffer::reset(uint32_t resetPolicy) noexcept {
  _flags = 0;

  size_t capacity = _capacity;
  if (capacity == 0 || resetPolicy != Globals::kResetHard)
    return kErrorOk;

  _allocator->release(_offsets, size_t(capacity) * kTextBufferItemSize);
  _items = nullptr;
  _offsets = nullptr;
  _capacity = 0;
  _glyphRun.reset();

  return kErrorOk;
}

Error TextBuffer::prepare(uint32_t capacity) noexcept {
  if (_capacity >= capacity)
    return kErrorOk;

  uint32_t kMaxCapacity = uint32_t(std::min<size_t>(
    (std::numeric_limits<uint32_t>::max() - 64),
    (std::numeric_limits<size_t>::max() - 1024) / kTextBufferItemSize));

  if (B2D_UNLIKELY(size_t(capacity) > kMaxCapacity))
    return DebugUtils::errored(kErrorNoMemory);

  capacity = Support::alignUp(capacity, 64);

  size_t bufferSize = size_t(capacity) * kTextBufferItemSize;
  size_t sizeAllocated;

  void* newData = _allocator->alloc(bufferSize, sizeAllocated);
  if (B2D_UNLIKELY(!newData))
    return DebugUtils::errored(kErrorNoMemory);

  if (_offsets)
    _allocator->release(_offsets, size_t(_capacity) * kTextBufferItemSize);

  _capacity = sizeAllocated / kTextBufferItemSize;
  _items = static_cast<GlyphItem*>(Support::advancePtr(newData, _capacity * sizeof(Point)));
  _offsets = static_cast<Point*>(newData);
  _glyphRun.setGlyphItems(_items);
  _glyphRun.setGlyphOffsets(_offsets);

  return kErrorOk;
}

// ============================================================================
// [b2d::TextBuffer - Text]
// ============================================================================

template<typename Reader, typename CharType>
static B2D_INLINE Error TextBuffer_setText(TextBuffer* self, const CharType* charData, size_t charCount) noexcept {
  Reader reader(charData, charCount);
  GlyphItem* glyphPtr = self->_items;

  while (reader.hasNext()) {
    uint32_t uc;
    Error err = reader.next(uc);

    if (B2D_LIKELY(err == kErrorOk)) {
      glyphPtr->setCodePoint(uc);
      glyphPtr->setCluster(uint32_t(reader.nativeIndex(charData)));
      glyphPtr++;
    }
    else {
      glyphPtr->setCodePoint(Unicode::kCharReplacement);
      self->_flags |= TextBuffer::kFlagInvalidChars;
      reader.skipOneUnit();
    }
  }

  self->_flags = 0;
  self->_glyphRun.setSize((size_t)(glyphPtr - self->_items));

  return kErrorOk;
}

Error TextBuffer::setUtf8Text(const char* data, size_t size) noexcept {
  if (size == Globals::kNullTerminated)
    size = std::strlen(data);

  if (sizeof(size_t) != sizeof(uint32_t) && size > std::numeric_limits<uint32_t>::max())
    return DebugUtils::errored(kErrorStringTooLarge);

  if (_capacity < size)
    B2D_PROPAGATE(prepare(size));

  return TextBuffer_setText<Unicode::Utf8Reader>(this, data, size);
}

Error TextBuffer::setUtf16Text(const uint16_t* data, size_t size) noexcept {
  if (size == Globals::kNullTerminated)
    size = strlenT(data);

  if (sizeof(size_t) != sizeof(uint32_t) && size > std::numeric_limits<uint32_t>::max())
    return DebugUtils::errored(kErrorStringTooLarge);

  if (_capacity < size)
    B2D_PROPAGATE(prepare(size));

  return TextBuffer_setText<Unicode::Utf16Reader>(this, data, size * 2);
}

Error TextBuffer::setUtf32Text(const uint32_t* data, size_t size) noexcept {
  if (size == Globals::kNullTerminated)
    size = strlenT(data);

  if (sizeof(size_t) != sizeof(uint32_t) && size > std::numeric_limits<uint32_t>::max())
    return DebugUtils::errored(kErrorStringTooLarge);

  if (_capacity < size)
    B2D_PROPAGATE(prepare(size));

  return TextBuffer_setText<Unicode::Utf32Reader>(this, data, size * 4);
}

// ============================================================================
// [b2d::TextBuffer - Processing]
// ============================================================================

Error TextBuffer::shape(const GlyphEngine& engine) noexcept {
  mapToGlyphs(engine);
  positionGlyphs(engine);

  return kErrorOk;
}

Error TextBuffer::mapToGlyphs(const GlyphEngine& engine) noexcept {
  if (_flags & kFlagGlyphIds)
    return DebugUtils::errored(kErrorInvalidState);

  b2d::GlyphMappingState state;
  Error err = engine.mapGlyphItems(items(), size(), state);

  if (state.undefinedCount())
    _flags |= kFlagUndefinedChars;

  _flags |= kFlagGlyphIds;
  return err;
}

Error TextBuffer::positionGlyphs(const GlyphEngine& engine, uint32_t positioningFlags) noexcept {
  if (!(_flags & kFlagGlyphIds))
    return DebugUtils::errored(kErrorInvalidState);

  if (!(_flags & kFlagGlyphAdvances)) {
    engine.getGlyphAdvances(_items, _offsets, size());
    _flags |= kFlagGlyphAdvances;
    _glyphRun.addFlags(GlyphRun::kFlagOffsetsAreAdvances | GlyphRun::kFlagDesignSpaceOffsets);
  }

  if (positioningFlags) {
    engine.applyPairAdjustment(_items, _offsets, size());
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::TextBuffer - Measure]
// ============================================================================

Error TextBuffer::measureText(const GlyphEngine& engine, TextMetrics& out) noexcept {
  out.reset();
  if (empty())
    return kErrorOk;

  if (!(_flags & kFlagGlyphAdvances)) {
    B2D_PROPAGATE(shape(engine));
    if (empty())
      return kErrorOk;
  }

  size_t size = this->size();
  double advance = 0.0;

  const GlyphItem* items = this->items();
  const Point* offsets = this->offsets();

  for (size_t i = 0; i < size; i++) {
    advance += offsets[i].x();
  }

  IntBox glyphBounds[2];
  GlyphId borderGlyphs[2] = { GlyphId(items[0].glyphId()), GlyphId(items[size - 1].glyphId()) };

  B2D_PROPAGATE(engine.getGlyphBoundingBoxes(borderGlyphs, glyphBounds, 2));

  out._advance.reset(advance, 0.0);
  out._boundingBox.reset(glyphBounds[0].x0(), 0.0, advance - offsets[size - 1].x() + glyphBounds[1].x1(), 0.0);

  return kErrorOk;
}

B2D_END_NAMESPACE
