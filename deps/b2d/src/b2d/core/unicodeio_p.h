// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_UNICODEIO_P_H
#define _B2D_CORE_UNICODEIO_P_H

// [Dependencies]
#include "../core/math.h"
#include "../core/support.h"
#include "../core/unicode.h"

B2D_BEGIN_SUB_NAMESPACE(Unicode)

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Unicode - ReaderFlags]
// ============================================================================

//! \internal
//!
//! Flags that can be used to parametrize unicode iterators.
enum ReaderFlags : uint32_t {
  kReadStrict       = 0x00000001u,
  kReadByteSwap     = 0x00000002u,
  kReadUnaligned    = 0x00000004u,
  kReadCalcUtfIndex = 0x00000008u
};

// ============================================================================
// [b2d::Unicode - Utf8Reader]
// ============================================================================

//! \internal
class Utf8Reader {
public:
  enum : uint32_t { kCharSize = 1 };

  B2D_INLINE Utf8Reader(const void* data, size_t byteSize) noexcept {
    reset(data, byteSize);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const void* data, size_t byteSize) noexcept {
    _ptr = static_cast<const char*>(data);
    _end = static_cast<const char*>(data) + byteSize;
    _utf32IndexSubtract = 0;
    _utf16SurrogateCount = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasNext() const noexcept { return _ptr != _end; }
  B2D_INLINE size_t remainingByteSize() const noexcept { return (size_t)(_end - _ptr); }

  B2D_INLINE size_t byteIndex(const void* start) const noexcept { return (size_t)(_ptr - static_cast<const char*>(start)); }
  B2D_INLINE size_t utf8Index(const void* start) const noexcept { return byteIndex(start); }
  B2D_INLINE size_t utf16Index(const void* start) const noexcept { return utf32Index(start) + _utf16SurrogateCount; }
  B2D_INLINE size_t utf32Index(const void* start) const noexcept { return byteIndex(start) - _utf32IndexSubtract; }
  B2D_INLINE size_t nativeIndex(const void* start) const noexcept { return utf8Index(start); }

  // --------------------------------------------------------------------------
  // [Iterator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc) noexcept {
    size_t ucSizeInBytes;
    return next<Flags>(uc, ucSizeInBytes);
  }

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc, size_t& ucSizeInBytes) noexcept {
    B2D_ASSERT(hasNext());

    uc = Support::readU8(_ptr);
    ucSizeInBytes = 1;

    _ptr++;
    if (uc < 0x80u) {
      // 1-Byte UTF-8 Sequence -> [0x00..0x7F].
      // ...nothing to do...
    }
    else {
      // Start of MultiByte.
      const uint32_t kMultiByte = 0xC2u;

      uc -= kMultiByte;
      if (uc < 0xE0u - kMultiByte) {
        // 2-Byte UTF-8 Sequence -> [0x80-0x7FF].
        _ptr++;
        ucSizeInBytes = 2;

        // Truncated input.
        if (B2D_UNLIKELY(_ptr > _end))
          goto TruncatedString;

        // All consecutive bytes must be '10xxxxxx'.
        uint32_t b1 = Support::readU8(_ptr - 1) ^ 0x80u;
        uc = ((uc + kMultiByte - 0xC0u) << 6) + b1;

        if (B2D_UNLIKELY(b1 > 0x3Fu))
          goto InvalidString;

        // 2-Byte UTF-8 maps to one UTF-16 or UTF-32 code-point, so subtract 1.
        if (Flags & kReadCalcUtfIndex) _utf32IndexSubtract += 1;
      }
      else if (uc < 0xF0u - kMultiByte) {
        // 3-Byte UTF-8 Sequence -> [0x800-0xFFFF].
        _ptr += 2;
        ucSizeInBytes = 3;

        // Truncated input.
        if (B2D_UNLIKELY(_ptr > _end))
          goto TruncatedString;

        uint32_t b1 = Support::readU8(_ptr - 2) ^ 0x80u;
        uint32_t b2 = Support::readU8(_ptr - 1) ^ 0x80u;
        uc = ((uc + kMultiByte - 0xE0u) << 12) + (b1 << 6) + b2;

        // 1. All consecutive bytes must be '10xxxxxx'.
        // 2. Refuse overlong UTF-8.
        if (B2D_UNLIKELY((b1 | b2) > 0x3Fu || uc < 0x800u))
          goto InvalidString;

        // 3-Byte UTF-8 maps to one UTF-16 or UTF-32 code-point, so subtract 2.
        if (Flags & kReadCalcUtfIndex) _utf32IndexSubtract += 2;
      }
      else {
        // 4-Byte UTF-8 Sequence -> [0x010000-0x10FFFF].
        _ptr += 3;
        ucSizeInBytes = 4;

        // Truncated input.
        if (B2D_UNLIKELY(_ptr > _end)) {
          // If this happens we want to report a correct error, bytes 0xF5
          // and above are always invalid and normally caught later.
          if (uc >= 0xF5u - kMultiByte)
            goto InvalidString;
          else
            goto TruncatedString;
        }

        uint32_t b1 = Support::readU8(_ptr - 3) ^ 0x80u;
        uint32_t b2 = Support::readU8(_ptr - 2) ^ 0x80u;
        uint32_t b3 = Support::readU8(_ptr - 1) ^ 0x80u;
        uc = ((uc + kMultiByte - 0xF0u) << 18) + (b1 << 12) + (b2 << 6) + b3;

        // 1. All consecutive bytes must be '10xxxxxx'.
        // 2. Refuse overlong UTF-8.
        // 3. Make sure the final character is <= U+10FFFF.
        if (B2D_UNLIKELY((b1 | b2 | b3) > 0x3Fu || uc < 0x010000u || uc > kCharMax))
          goto InvalidString;

        // 4-Byte UTF-8 maps to one UTF-16 or UTF-32 code-point, so subtract 3.
        if (Flags & kReadCalcUtfIndex) _utf32IndexSubtract += 3;
        if (Flags & kReadCalcUtfIndex) _utf16SurrogateCount += 1;
      }
    }
    return kErrorOk;

InvalidString:
    _ptr -= ucSizeInBytes;
    return DebugUtils::errored(kErrorInvalidString);

TruncatedString:
    _ptr -= ucSizeInBytes;
    return DebugUtils::errored(kErrorTruncatedString);
  }

  B2D_INLINE void skipOneUnit() noexcept {
    B2D_ASSERT(hasNext());
    _ptr++;
  }

  // --------------------------------------------------------------------------
  // [Validator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error validate() noexcept {
    Error err = kErrorOk;
    while (hasNext()) {
      uint32_t uc;
      err = next<Flags>(uc);
      if (err)
        break;
    }
    return err;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const char* _ptr;                      //!< Current pointer.
  const char* _end;                      //!< End of input.
  size_t _utf32IndexSubtract;            //!< `index() - _utf32IndexSubtract` yields the current `utf32Index()`.
  size_t _utf16SurrogateCount;           //!< Number of surrogates is required to calculate `utf16Index()`.
};

// ============================================================================
// [b2d::Unicode - Utf16Reader]
// ============================================================================

//! \internal
class Utf16Reader {
public:
  enum : uint32_t { kCharSize = 2 };

  B2D_INLINE Utf16Reader(const void* data, size_t byteSize) noexcept {
    reset(data, byteSize);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const void* data, size_t byteSize) noexcept {
    _ptr = static_cast<const char*>(data);
    _end = static_cast<const char*>(data) + Support::alignDown(byteSize, 2);
    _utf8IndexAdd = 0;
    _utf16SurrogateCount = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasNext() const noexcept { return _ptr != _end; }
  B2D_INLINE size_t remainingByteSize() const noexcept { return (size_t)(_end - _ptr); }

  B2D_INLINE size_t byteIndex(const void* start) const noexcept { return (size_t)(_ptr - static_cast<const char*>(start)); }
  B2D_INLINE size_t utf8Index(const void* start) const noexcept { return utf16Index(start) + _utf8IndexAdd; }
  B2D_INLINE size_t utf16Index(const void* start) const noexcept { return byteIndex(start) / 2u; }
  B2D_INLINE size_t utf32Index(const void* start) const noexcept { return utf16Index(start) - _utf16SurrogateCount; }
  B2D_INLINE size_t nativeIndex(const void* start) const noexcept { return utf16Index(start); }

  // --------------------------------------------------------------------------
  // [Iterator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc) noexcept {
    size_t ucSizeInBytes;
    return next<Flags>(uc, ucSizeInBytes);
  }

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc, size_t& ucSizeInBytes) noexcept {
    B2D_ASSERT(hasNext());

    uc = readU16<Flags>(_ptr);
    _ptr += 2;

    if (isSurrogate(uc)) {
      if (B2D_LIKELY(isHighSurrogate(uc))) {
        if (B2D_LIKELY(_ptr != _end)) {
          uint32_t lo = readU16<Flags>(_ptr);
          if (B2D_LIKELY(isLowSurrogate(lo))) {
            uc = utf32FromSurrogate(uc, lo);
            _ptr += 2;

            // Add two to `_utf8IndexAdd` as two surrogates count as 2, so we
            // have to add 2 more to have UTF-8 length of a valid surrogate.
            if (Flags & kReadCalcUtfIndex) _utf8IndexAdd += 2;
            if (Flags & kReadCalcUtfIndex) _utf16SurrogateCount += 1;

            ucSizeInBytes = 4;
            return kErrorOk;
          }
          else {
            if (Flags & kReadStrict)
              goto InvalidString;
          }
        }
        else {
          if (Flags & kReadStrict)
            goto TruncatedString;
        }
      }
      else {
        if (Flags & kReadStrict)
          goto InvalidString;
      }
    }

    // Either not surrogate or fallback in non-strict mode.
    if (Flags & kReadCalcUtfIndex)
      _utf8IndexAdd += size_t(uc >= 0x0080u) + size_t(uc >= 0x0800u);

    ucSizeInBytes = 2;
    return kErrorOk;

InvalidString:
    _ptr -= 2;
    return DebugUtils::errored(kErrorInvalidString);

TruncatedString:
    _ptr -= 2;
    return DebugUtils::errored(kErrorTruncatedString);
  }

  B2D_INLINE void skipOneUnit() noexcept {
    B2D_ASSERT(hasNext());
    _ptr += 2;
  }

  // --------------------------------------------------------------------------
  // [Validator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error validate() noexcept {
    Error err = kErrorOk;
    while (hasNext()) {
      uint32_t uc;
      err = next<Flags>(uc);
      if (err)
        break;
    }
    return err;
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  static B2D_INLINE uint32_t readU16(const char* ptr) noexcept {
    return Support::readU16x<Flags & kReadByteSwap ? Globals::kByteOrderSwapped : Globals::kByteOrderNative, Flags & kReadUnaligned ? 1 : 2>(ptr);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const char* _ptr;
  const char* _end;

  size_t _utf8IndexAdd;
  size_t _utf16SurrogateCount;
};

// ============================================================================
// [b2d::Unicode - Utf32Reader]
// ============================================================================

//! \internal
class Utf32Reader {
public:
  enum : uint32_t { kCharSize = 4 };

  B2D_INLINE Utf32Reader(const void* data, size_t byteSize) noexcept {
    reset(data, byteSize);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(const void* data, size_t byteSize) noexcept {
    _ptr = static_cast<const char*>(data);
    _end = static_cast<const char*>(data) + Support::alignDown(byteSize, 4);
    _utf8IndexAdd = 0;
    _utf16SurrogateCount = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasNext() const noexcept { return _ptr != _end; }
  B2D_INLINE size_t remainingByteSize() const noexcept { return (size_t)(_end - _ptr); }

  B2D_INLINE size_t byteIndex(const void* start) const noexcept { return (size_t)(_ptr - static_cast<const char*>(start)); }
  B2D_INLINE size_t utf8Index(const void* start) const noexcept { return utf32Index(start) + _utf16SurrogateCount + _utf8IndexAdd; }
  B2D_INLINE size_t utf16Index(const void* start) const noexcept { return utf32Index(start) + _utf16SurrogateCount; }
  B2D_INLINE size_t utf32Index(const void* start) const noexcept { return byteIndex(start) / 4u; }
  B2D_INLINE size_t nativeIndex(const void* start) const noexcept { return utf32Index(start); }

  // --------------------------------------------------------------------------
  // [Iterator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc) noexcept {
    size_t ucSizeInBytes;
    return next<Flags>(uc, ucSizeInBytes);
  }

  template<uint32_t Flags = 0>
  B2D_INLINE Error next(uint32_t& uc, size_t& ucSizeInBytes) noexcept {
    B2D_ASSERT(hasNext());

    uc = readU32<Flags>(_ptr);
    if (B2D_UNLIKELY(uc > kCharMax))
      return DebugUtils::errored(kErrorInvalidString);

    if (Flags & kReadStrict) {
      if (B2D_UNLIKELY(isSurrogate(uc)))
        return DebugUtils::errored(kErrorInvalidString);
    }

    if (Flags & kReadCalcUtfIndex) _utf8IndexAdd += size_t(uc >= 0x800u) + size_t(uc >= 0x80u);
    if (Flags & kReadCalcUtfIndex) _utf16SurrogateCount += size_t(uc >= 0x10000u);

    _ptr += 4;
    ucSizeInBytes = 4;
    return kErrorOk;
  }

  B2D_INLINE void skipOneUnit() noexcept {
    B2D_ASSERT(hasNext());
    _ptr += 4;
  }

  // --------------------------------------------------------------------------
  // [Validator]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  B2D_INLINE Error validate() noexcept {
    Error err = kErrorOk;
    while (hasNext()) {
      uint32_t uc;
      err = next<Flags>(uc);
      if (err)
        break;
    }
    return err;
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  template<uint32_t Flags = 0>
  static B2D_INLINE uint32_t readU32(const char* ptr) noexcept {
    return Support::readU32x<Flags & kReadByteSwap ? Globals::kByteOrderSwapped : Globals::kByteOrderNative, Flags & kReadUnaligned ? 1 : 4>(ptr);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const char* _ptr;
  const char* _end;

  size_t _utf8IndexAdd;
  size_t _utf16SurrogateCount;
};

// ============================================================================
// [b2d::Unicode - Utf8Writer]
// ============================================================================

//! \internal
class Utf8Writer {
public:
  typedef char CharType;

  B2D_INLINE Utf8Writer(char* dst, size_t size) noexcept {
    reset(dst, size);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(char* dst, size_t size) noexcept {
    _ptr = dst;
    _end = dst + size;
  }

  B2D_INLINE size_t index(const char* start) const noexcept {
    return (size_t)(_ptr - start);
  }

  B2D_INLINE bool atEnd() const noexcept { return _ptr == _end; }
  B2D_INLINE size_t remainingSize() const noexcept { return (size_t)(_end - _ptr); }

  // --------------------------------------------------------------------------
  // [Writer]
  // --------------------------------------------------------------------------

  B2D_INLINE Error write(uint32_t uc) noexcept {
    if (uc <= 0x7F)
      return writeByte(uc);
    else if (uc <= 0x7FFu)
      return write2Bytes(uc);
    else if (uc <= 0xFFFFu)
      return write3Bytes(uc);
    else
      return write4Bytes(uc);
  }

  B2D_INLINE Error writeByte(uint32_t uc) noexcept {
    B2D_ASSERT(uc <= 0x7Fu);
    if (B2D_UNLIKELY(atEnd()))
      return DebugUtils::errored(kErrorStringNoSpaceLeft);

    _ptr[0] = char(uint8_t(uc));
    _ptr++;
    return kErrorOk;
  }

  B2D_INLINE Error writeByteUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 1);
    _ptr[0] = char(uint8_t(uc));
    _ptr++;
    return kErrorOk;
  }

  B2D_INLINE Error write2Bytes(uint32_t uc) noexcept {
    B2D_ASSERT(uc >= 0x80u && uc <= 0x7FFu);

    _ptr += 2;
    if (B2D_UNLIKELY(_ptr > _end)) {
      _ptr -= 2;
      return DebugUtils::errored(kErrorStringNoSpaceLeft);
    }

    _ptr[-2] = char(uint8_t(0xC0u | (uc >> 6)));
    _ptr[-1] = char(uint8_t(0x80u | (uc & 63)));
    return kErrorOk;
  }

  B2D_INLINE Error write2BytesUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 2);
    B2D_ASSERT(uc >= 0x80u && uc <= 0x7FFu);

    _ptr[0] = char(uint8_t(0xC0u | (uc >> 6)));
    _ptr[1] = char(uint8_t(0x80u | (uc & 63)));

    _ptr += 2;
    return kErrorOk;
  }

  B2D_INLINE Error write3Bytes(uint32_t uc) noexcept {
    B2D_ASSERT(uc >= 0x800u && uc <= 0xFFFFu);

    _ptr += 3;
    if (B2D_UNLIKELY(_ptr > _end)) {
      _ptr -= 3;
      return DebugUtils::errored(kErrorStringNoSpaceLeft);
    }

    _ptr[-3] = char(uint8_t(0xE0u | ((uc >> 12)     )));
    _ptr[-2] = char(uint8_t(0x80u | ((uc >>  6) & 63)));
    _ptr[-1] = char(uint8_t(0x80u | ((uc      ) & 63)));
    return kErrorOk;
  }

  B2D_INLINE Error write3BytesUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 3);
    B2D_ASSERT(uc >= 0x800u && uc <= 0xFFFFu);

    _ptr[0] = char(uint8_t(0xE0u | ((uc >> 12)     )));
    _ptr[1] = char(uint8_t(0x80u | ((uc >>  6) & 63)));
    _ptr[2] = char(uint8_t(0x80u | ((uc      ) & 63)));

    _ptr += 3;
    return kErrorOk;
  }

  B2D_INLINE Error write4Bytes(uint32_t uc) noexcept {
    B2D_ASSERT(uc >= 0x10000u && uc <= 0x10FFFFu);

    _ptr += 4;
    if (B2D_UNLIKELY(_ptr > _end)) {
      _ptr -= 4;
      return DebugUtils::errored(kErrorStringNoSpaceLeft);
    }

    _ptr[-4] = char(uint8_t(0xF0u | ((uc >> 18)     )));
    _ptr[-3] = char(uint8_t(0x80u | ((uc >> 12) & 63)));
    _ptr[-2] = char(uint8_t(0x80u | ((uc >>  6) & 63)));
    _ptr[-1] = char(uint8_t(0x80u | ((uc      ) & 63)));
    return kErrorOk;
  }

  B2D_INLINE Error write4BytesUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 4);
    B2D_ASSERT(uc >= 0x10000u && uc <= 0x10FFFFu);

    _ptr[0] = char(uint8_t(0xF0u | ((uc >> 18)     )));
    _ptr[1] = char(uint8_t(0x80u | ((uc >> 12) & 63)));
    _ptr[2] = char(uint8_t(0x80u | ((uc >>  6) & 63)));
    _ptr[3] = char(uint8_t(0x80u | ((uc      ) & 63)));

    _ptr += 4;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  char* _ptr;
  char* _end;
};

// ============================================================================
// [b2d::Unicode - Utf16Writer]
// ============================================================================

//! \internal
template<uint32_t Endian = Globals::kByteOrderNative, uint32_t Alignment = 2>
class Utf16Writer {
public:
  typedef uint16_t CharType;

  B2D_INLINE Utf16Writer(uint16_t* dst, size_t size) noexcept {
    reset(dst, size);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(uint16_t* dst, size_t size) noexcept {
    _ptr = dst;
    _end = dst + size;
  }

  B2D_INLINE size_t index(const uint16_t* start) const noexcept {
    return (size_t)(_ptr - start);
  }

  B2D_INLINE bool atEnd() const noexcept { return _ptr == _end; }
  B2D_INLINE size_t remainingSize() const noexcept { return (size_t)(_end - _ptr); }

  // --------------------------------------------------------------------------
  // [Writer]
  // --------------------------------------------------------------------------

  B2D_INLINE Error write(uint32_t uc) noexcept {
    if (uc <= 0xFFFFu)
      return writeBMP(uc);
    else
      return writeSMP(uc);
  }

  B2D_INLINE Error writeBMP(uint32_t uc) noexcept {
    B2D_ASSERT(uc <= 0xFFFFu);

    if (B2D_UNLIKELY(atEnd()))
      return DebugUtils::errored(kErrorStringNoSpaceLeft);

    _writeU16(_ptr, uc);
    _ptr++;
    return kErrorOk;
  }

  B2D_INLINE Error writeBMPUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 1);

    _writeU16(_ptr, uc);
    _ptr++;
    return kErrorOk;
  }

  B2D_INLINE Error writeSMP(uint32_t uc) noexcept {
    B2D_ASSERT(uc >= 0x10000u && uc <= 0x10FFFFu);

    _ptr += 2;
    if (B2D_UNLIKELY(_ptr > _end)) {
      _ptr -= 2;
      return DebugUtils::errored(kErrorStringNoSpaceLeft);
    }

    uint32_t hi, lo;
    utf32ToSurrogate(uc, hi, lo);

    _writeU16(_ptr - 2, hi);
    _writeU16(_ptr - 1, lo);
    return kErrorOk;
  }

  B2D_INLINE Error writeSMPUnsafe(uint32_t uc) noexcept {
    B2D_ASSERT(remainingSize() >= 2);
    B2D_ASSERT(uc >= 0x10000u && uc <= 0x10FFFFu);

    uint32_t hi, lo;
    utf32ToSurrogate(uc, hi, lo);

    _writeU16(_ptr + 0, hi);
    _writeU16(_ptr + 1, lo);

    _ptr += 2;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static B2D_INLINE void _writeU16(void* dst, uint32_t value) noexcept {
    if (Endian == Globals::kByteOrderLE)
      Support::writeU16xLE<Alignment>(dst, value);
    else
      Support::writeU16xBE<Alignment>(dst, value);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint16_t* _ptr;
  uint16_t* _end;
};

// ============================================================================
// [b2d::Unicode - Utf32Writer]
// ============================================================================

//! \internal
template<uint32_t Endian = Globals::kByteOrderNative, uint32_t Alignment = 4>
class Utf32Writer {
public:
  typedef uint32_t CharType;

  B2D_INLINE Utf32Writer(uint32_t* dst, size_t size) noexcept {
    reset(dst, size);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset(uint32_t* dst, size_t size) noexcept {
    _ptr = dst;
    _end = dst + size;
  }

  B2D_INLINE size_t index(const uint32_t* start) const noexcept {
    return (size_t)(_ptr - start);
  }

  B2D_INLINE bool atEnd() const noexcept { return _ptr == _end; }
  B2D_INLINE size_t remainingSize() const noexcept { return (size_t)(_end - _ptr); }

  // --------------------------------------------------------------------------
  // [Writer]
  // --------------------------------------------------------------------------

  B2D_INLINE Error write(uint32_t uc) noexcept {
    if (B2D_UNLIKELY(atEnd()))
      return DebugUtils::errored(kErrorStringNoSpaceLeft);

    _writeU32(_ptr, uc);
    _ptr++;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static B2D_INLINE void _writeU32(void* dst, uint32_t value) noexcept {
    Support::writeU32x<Globals::kByteOrderNative, Alignment>(dst, value);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t* _ptr;
  uint32_t* _end;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_CORE_UNICODEIO_P_H
