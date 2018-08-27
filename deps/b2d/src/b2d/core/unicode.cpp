// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math.h"
#include "../core/unicode.h"
#include "../core/unicodeio_p.h"

B2D_BEGIN_SUB_NAMESPACE(Unicode)

// ============================================================================
// [b2d::Unicode - Validation]
// ============================================================================

// Not really anything to validate, we just want to calculate a corresponding UTf-8 size.
static B2D_INLINE Error validateLatin1String(const char* data, size_t size, ValidationState& state) noexcept {
  size_t extra = 0;
  state._utf16Index = size;
  state._utf32Index = size;

  for (size_t i = 0; i < size; i++)
    extra += size_t(uint8_t(data[i]) & 0xFFu) >> 7;

  Support::OverflowFlag of;
  size_t utf8Size = Support::safeAdd(size, extra, &of);

  if (B2D_UNLIKELY(of))
    return DebugUtils::errored(kErrorStringTooLarge);

  state._utf8Index = utf8Size;
  return kErrorOk;
}

template<typename Iterator, uint32_t Flags>
static B2D_INLINE Error validateUnicodeString(const void* data, size_t size, ValidationState& state) noexcept {
  Iterator it(data, size);
  Error err = it.template validate<Flags | kReadCalcUtfIndex>();
  state._utf8Index = it.utf8Index(data);
  state._utf16Index = it.utf16Index(data);
  state._utf32Index = it.utf32Index(data);
  return err;
}

Error validateString(const void* data, size_t sizeInBytes, uint32_t encoding, ValidationState& state) noexcept {
  using Globals::kByteOrderLE;
  using Globals::kByteOrderBE;
  using Globals::kByteOrderNative;

  state.reset();

  switch (encoding) {
    case kEncodingUtf8: {
      return validateUnicodeString<Utf8Reader, kReadStrict>(data, sizeInBytes, state);
    }

    case kEncodingLatin1: {
      return validateLatin1String(static_cast<const char*>(data), sizeInBytes, state);
    }

    case kEncodingUtf16LE:
    case kEncodingUtf16BE: {
      Error err;

      // This will make sure we won't compile specialized code for architectures that don't penalize unaligned reads.
      if (Support::kUnalignedAccess16 || !Support::isAligned(data, 2)) {
        if (encoding == kEncodingUtf16)
          err = validateUnicodeString<Utf16Reader, kReadStrict | kReadUnaligned>(data, sizeInBytes, state);
        else
          err = validateUnicodeString<Utf16Reader, kReadStrict | kReadUnaligned | kReadByteSwap>(data, sizeInBytes, state);
      }
      else {
        if (encoding == kEncodingUtf16)
          err = validateUnicodeString<Utf16Reader, kReadStrict>(data, sizeInBytes, state);
        else
          err = validateUnicodeString<Utf16Reader, kReadStrict | kReadByteSwap>(data, sizeInBytes, state);
      }

      if (!err && B2D_UNLIKELY(sizeInBytes & 0x1))
        err = DebugUtils::errored(kErrorTruncatedString);

      return err;
    }

    case kEncodingUtf32: {
      Error err;

      // This will make sure we won't compile specialized code for architectures that don't penalize unaligned reads.
      if (Support::kUnalignedAccess32 || !Support::isAligned(data, 4))
        err = validateUnicodeString<Utf32Reader, kReadStrict | kReadUnaligned>(data, sizeInBytes, state);
      else
        err = validateUnicodeString<Utf32Reader, kReadStrict>(data, sizeInBytes, state);

      if (!err && B2D_UNLIKELY(sizeInBytes & 0x3))
        err = DebugUtils::errored(kErrorTruncatedString);

      return err;
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }
}

// ============================================================================
// [b2d::Unicode - Conversion]
// ============================================================================

static B2D_INLINE size_t offsetOfPtr(const void* base, const void* advanced) noexcept {
  return (size_t)(static_cast<const char*>(advanced) - static_cast<const char*>(base));
}

// A simple implementation. It iterates `src` char-by-char and writes it to the
// destination. The advantage of this implementation is that switching `Writer`
// and `Iterator` can customize strictness, endianness, etc, so we don't have
// to repeat the code for different variations of UTF16 and UTF32.
template<typename Writer, typename Iterator, uint32_t IterFlags>
static B2D_INLINE Error convertStringImpl(void* dst, size_t dstSizeInBytes, const void* src, size_t srcSizeInBytes, ConversionState& state) noexcept {
  typedef typename Writer::CharType DstChar;

  Writer writer(static_cast<DstChar*>(dst), dstSizeInBytes / sizeof(DstChar));
  Iterator iter(src, Support::alignDown(srcSizeInBytes, Iterator::kCharSize));

  Error err = kErrorOk;
  while (iter.hasNext()) {
    uint32_t uc;
    size_t ucSizeInBytes;

    err = iter.template next<IterFlags>(uc, ucSizeInBytes);
    if (B2D_UNLIKELY(err))
      break;

    err = writer.write(uc);
    if (B2D_UNLIKELY(err)) {
      state._dstIndex = offsetOfPtr(dst, writer._ptr);
      state._srcIndex = offsetOfPtr(src, iter._ptr) - ucSizeInBytes;
      return err;
    }
  }

  state._dstIndex = offsetOfPtr(dst, writer._ptr);
  state._srcIndex = offsetOfPtr(src, iter._ptr);

  if (Iterator::kCharSize > 1 && !err && !Support::isAligned(state._srcIndex, Iterator::kCharSize))
    return DebugUtils::errored(kErrorTruncatedString);
  else
    return err;
}

Error _convertString(void* dst, size_t dstSizeInBytes, const void* src, size_t srcSizeInBytes, uint32_t flags, ConversionState& state) noexcept {
  using Globals::kByteOrderLE;
  using Globals::kByteOrderBE;
  using Globals::kByteOrderNative;

  using Support::kUnalignedAccess16;
  using Support::kUnalignedAccess32;
  constexpr bool kUnalignedAccessAny = kUnalignedAccess16 && kUnalignedAccess32;

  Error err = kErrorOk;
  state.reset();

  uint32_t combination = flags & 0xFFFFu;
  switch (combination) {
    // ------------------------------------------------------------------------
    // [MemCpy]
    // ------------------------------------------------------------------------

    case (kEncodingLatin1 << 8) | kEncodingLatin1: {
      size_t copySize = Math::min(dstSizeInBytes, srcSizeInBytes);
      std::memcpy(dst, src, copySize);

      state._dstIndex = copySize;
      state._srcIndex = copySize;

      if (dstSizeInBytes < srcSizeInBytes)
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf8 <- Latin1]
    // ------------------------------------------------------------------------

    case (kEncodingUtf8 << 8) | kEncodingLatin1: {
      Utf8Writer writer(static_cast<char*>(dst), dstSizeInBytes);
      const uint8_t* src8 = static_cast<const uint8_t*>(src);

      if (dstSizeInBytes / 2 >= srcSizeInBytes) {
        // Fast case, there is enough space in `dst` even for the worst-case scenario.
        for (size_t i = 0; i < srcSizeInBytes; i++) {
          uint32_t uc = src8[i];
          if (uc <= 0x7F)
            writer.writeByteUnsafe(uc);
          else
            writer.write2BytesUnsafe(uc);
        }

        state._srcIndex = srcSizeInBytes;
        state._dstIndex = writer.index(static_cast<char*>(dst));
      }
      else {
        for (size_t i = 0; i < srcSizeInBytes; i++) {
          uint32_t uc = src8[i];
          if (uc <= 0x7F)
            err = writer.writeByte(uc);
          else
            err = writer.write2Bytes(uc);

          if (B2D_UNLIKELY(err)) {
            state._dstIndex = writer.index(static_cast<char*>(dst));
            state._srcIndex = i;
            break;
          }
        }
      }

      break;
    }

    // ------------------------------------------------------------------------
    // [Utf8 <- Utf8]
    // ------------------------------------------------------------------------

    case (kEncodingUtf8 << 8) | kEncodingUtf8: {
      size_t copySize = Math::min(dstSizeInBytes, srcSizeInBytes);
      ValidationState validationState;

      err = validateString(src, copySize, kEncodingUtf8, validationState);
      size_t validatedSize = validationState.utf8Index();

      if (validatedSize)
        std::memcpy(dst, src, validatedSize);

      // Prevent `kErrorTruncatedString` in case there is not enough space in destination.
      if (copySize < srcSizeInBytes && (err == kErrorOk || err == kErrorTruncatedString))
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);

      state._dstIndex = validatedSize;
      state._srcIndex = validatedSize;
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf8 <- Utf16]
    // ------------------------------------------------------------------------

    case (kEncodingUtf8 << 8) | kEncodingUtf16:
      if (kUnalignedAccess16 || !Support::isAligned(src, 2))
        err = convertStringImpl<Utf8Writer, Utf16Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf8Writer, Utf16Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    case (kEncodingUtf8 << 8) | kEncodingUtf16Swap:
      if (kUnalignedAccess16 || !Support::isAligned(src, 2))
        err = convertStringImpl<Utf8Writer, Utf16Reader, kReadStrict | kReadByteSwap | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf8Writer, Utf16Reader, kReadStrict | kReadByteSwap>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    // ------------------------------------------------------------------------
    // [Utf8 <- Utf32]
    // ------------------------------------------------------------------------

    case (kEncodingUtf8 << 8) | kEncodingUtf32: {
      if (kUnalignedAccess32 || !Support::isAligned(src, 4))
        err = convertStringImpl<Utf8Writer, Utf32Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf8Writer, Utf32Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf16 <- Latin1]
    // ------------------------------------------------------------------------

    case (kEncodingUtf16LE << 8) | kEncodingLatin1: {
      size_t count = Math::min(dstSizeInBytes / 2, srcSizeInBytes);

      if (kUnalignedAccess16 || Support::isAligned(dst, 2)) {
        for (size_t i = 0; i < count; i++)
          Support::writeU16aLE(static_cast<uint8_t*>(dst) + i * 2u, static_cast<const uint8_t*>(src)[i]);
      }
      else {
        for (size_t i = 0; i < count; i++)
          Support::writeU16uLE(static_cast<uint8_t*>(dst) + i * 2u, static_cast<const uint8_t*>(src)[i]);
      }

      if (count < srcSizeInBytes)
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);

      state._dstIndex = count * 2u;
      state._srcIndex = count;
      break;
    }

    case (kEncodingUtf16BE << 8) | kEncodingLatin1: {
      size_t count = Math::min(dstSizeInBytes / 2, srcSizeInBytes);

      if (kUnalignedAccess16 || Support::isAligned(dst, 2)) {
        for (size_t i = 0; i < count; i++)
          Support::writeU16aBE(static_cast<uint8_t*>(dst) + i * 2u, static_cast<const uint8_t*>(src)[i]);
      }
      else {
        for (size_t i = 0; i < count; i++)
          Support::writeU16uBE(static_cast<uint8_t*>(dst) + i * 2u, static_cast<const uint8_t*>(src)[i]);
      }

      if (count < srcSizeInBytes)
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);

      state._dstIndex = count * 2u;
      state._srcIndex = count;
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf16 <- Utf8]
    // ------------------------------------------------------------------------

    case (kEncodingUtf16LE << 8) | kEncodingUtf8: {
      if (kUnalignedAccess16 || !Support::isAligned(dst, 2))
        err = convertStringImpl<Utf16Writer<kByteOrderLE, 1>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf16Writer<kByteOrderLE, 2>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;
    }

    case (kEncodingUtf16BE << 8) | kEncodingUtf8: {
      if (kUnalignedAccess16 || !Support::isAligned(dst, 2))
        err = convertStringImpl<Utf16Writer<kByteOrderBE, 1>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf16Writer<kByteOrderBE, 2>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf16 <- Utf16]
    // ------------------------------------------------------------------------

    case (kEncodingUtf16LE << 8) | kEncodingUtf16LE:
    case (kEncodingUtf16LE << 8) | kEncodingUtf16BE:
    case (kEncodingUtf16BE << 8) | kEncodingUtf16LE:
    case (kEncodingUtf16BE << 8) | kEncodingUtf16BE: {
      size_t copySize = Support::alignDown(Math::min(dstSizeInBytes, srcSizeInBytes), 2);
      ValidationState validationState;

      err = validateString(src, copySize, flags & 0xFFu, validationState);
      size_t validatedSize = validationState.utf16Index() * 2;

      if ((combination >> 8) != (combination & 0xFFu)) {
        // ByteSwap (either UTF16-BE to LE or the opposite).
        if (!kUnalignedAccess16 && Support::isAligned(dst, 2) && Support::isAligned(src, 2)) {
          for (size_t i = 0; i < validatedSize; i += 2) {
            uint32_t value = Support::readU16a(static_cast<const uint8_t*>(src) + i);
            Support::writeU16a(static_cast<uint8_t*>(dst) + i, Support::byteswap16(value));
          }
        }
        else {
          for (size_t i = 0; i < validatedSize; i += 2) {
            uint8_t c0 = static_cast<const uint8_t*>(src)[i + 0];
            uint8_t c1 = static_cast<const uint8_t*>(src)[i + 1];

            static_cast<uint8_t*>(dst)[i + 0] = c1;
            static_cast<uint8_t*>(dst)[i + 1] = c0;
          }
        }
      }
      else {
        if (validatedSize)
          std::memmove(dst, src, validatedSize);
      }

      // Prevent `kErrorTruncatedString` in case there is not enough space in destination.
      if (copySize < srcSizeInBytes && (err == kErrorOk || err == kErrorTruncatedString))
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);

      // Report `kErrorTruncatedString` is everything went right, but the
      // source size was not aligned to 2 bytes.
      if (!err && !Support::isAligned(srcSizeInBytes, 2))
        err = DebugUtils::errored(kErrorTruncatedString);

      state._dstIndex = validatedSize;
      state._srcIndex = validatedSize;
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf16 <- Utf32]
    // ------------------------------------------------------------------------

    case (kEncodingUtf16LE << 8) | kEncodingUtf32: {
      if (kUnalignedAccessAny || !Support::isAligned(dst, 2) || !Support::isAligned(src, 4))
        err = convertStringImpl<Utf16Writer<kByteOrderLE, 1>, Utf32Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf16Writer<kByteOrderLE, 2>, Utf32Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;
    }

    case (kEncodingUtf16BE << 8) | kEncodingUtf32: {
      if (kUnalignedAccessAny || !Support::isAligned(dst, 2) || !Support::isAligned(src, 4))
        err = convertStringImpl<Utf16Writer<kByteOrderBE, 1>, Utf32Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf16Writer<kByteOrderBE, 2>, Utf32Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf32 <- Latin1]
    // ------------------------------------------------------------------------

    case (kEncodingUtf32 << 8) | kEncodingLatin1: {
      size_t count = Math::min(dstSizeInBytes / 4, srcSizeInBytes);

      if (kUnalignedAccess32 || Support::isAligned(dst, 4)) {
        for (size_t i = 0; i < count; i++)
          Support::writeU32a(static_cast<uint8_t*>(dst) + i * 4u, static_cast<const uint8_t*>(src)[i]);
      }
      else {
        for (size_t i = 0; i < count; i++)
          Support::writeU32u(static_cast<uint8_t*>(dst) + i * 4u, static_cast<const uint8_t*>(src)[i]);
      }

      if (count < srcSizeInBytes)
        err = DebugUtils::errored(kErrorStringNoSpaceLeft);

      state._dstIndex = count * 4u;
      state._srcIndex = count;
      break;
    }

    // ------------------------------------------------------------------------
    // [Utf32 <- Utf8]
    // ------------------------------------------------------------------------

    case (kEncodingUtf32 << 8) | kEncodingUtf8:
      if (kUnalignedAccess32 || !Support::isAligned(dst, 4))
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 1>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 4>, Utf8Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    // ------------------------------------------------------------------------
    // [Utf32 <- Utf16]
    // ------------------------------------------------------------------------

    case (kEncodingUtf32 << 8) | kEncodingUtf16:
      if (kUnalignedAccessAny || !Support::isAligned(dst, 4) || !Support::isAligned(src, 2))
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 1>, Utf16Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 4>, Utf16Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    case (kEncodingUtf32 << 8) | kEncodingUtf16Swap:
      if (kUnalignedAccessAny || !Support::isAligned(dst, 4) || !Support::isAligned(src, 2))
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 1>, Utf16Reader, kReadStrict | kReadByteSwap | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 4>, Utf16Reader, kReadStrict | kReadByteSwap>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    // ------------------------------------------------------------------------
    // [Utf32 <- Utf32]
    // ------------------------------------------------------------------------

    case (kEncodingUtf32 << 8) | kEncodingUtf32:
      if (kUnalignedAccess32 || !Support::isAligned(dst, 4) || !Support::isAligned(src, 4))
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 1>, Utf32Reader, kReadStrict>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      else
        err = convertStringImpl<Utf32Writer<kByteOrderNative, 4>, Utf32Reader, kReadStrict | kReadUnaligned>(dst, dstSizeInBytes, src, srcSizeInBytes, state);
      break;

    // ------------------------------------------------------------------------
    // [Invalid]
    // ------------------------------------------------------------------------

    default: {
      return DebugUtils::errored(kErrorInvalidArgument);
    }
  }

  return err;
}

// ============================================================================
// [b2d::Unicode - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_unicode_convert) {
  struct TestEntry {
    char dst[24];
    char src[24];

    uint8_t dstSize;
    uint8_t srcSize;
    uint8_t dstEncoding;
    uint8_t srcEncoding;

    Error result;
  };

  static const TestEntry testEntries[] = {
    #define ENTRY(DST, DST_ENC, SRC, SRC_ENC, ERROR_CODE) { \
      DST,                                                  \
      SRC,                                                  \
      uint8_t(sizeof(DST) - 1),                             \
      uint8_t(sizeof(SRC) - 1),                             \
      uint8_t(kEncoding##DST_ENC),                          \
      uint8_t(kEncoding##SRC_ENC),                          \
      ERROR_CODE                                            \
    }

    ENTRY("Test"                            , Latin1 , "Test"                            , Latin1 , kErrorOk),
    ENTRY("Test"                            , Utf8   , "Test"                            , Latin1 , kErrorOk),
    ENTRY("Test"                            , Utf8   , "Test"                            , Utf8   , kErrorOk),
    ENTRY("Test"                            , Utf8   , "\0T\0e\0s\0t"                    , Utf16BE, kErrorOk),
    ENTRY("Test"                            , Utf8   , "T\0e\0s\0t\0"                    , Utf16LE, kErrorOk),

    // Tests a Czech word (Rain in english) with diacritic marks, at most 2 BYTEs per character.
    ENTRY("\x44\xC3\xA9\xC5\xA1\xC5\xA5"    , Utf8   , "\x00\x44\x00\xE9\x01\x61\x01\x65", Utf16BE, kErrorOk),
    ENTRY("\x44\xC3\xA9\xC5\xA1\xC5\xA5"    , Utf8   , "\x44\x00\xE9\x00\x61\x01\x65\x01", Utf16LE, kErrorOk),
    ENTRY("\x00\x44\x00\xE9\x01\x61\x01\x65", Utf16BE, "\x44\xC3\xA9\xC5\xA1\xC5\xA5"    , Utf8   , kErrorOk),
    ENTRY("\x44\x00\xE9\x00\x61\x01\x65\x01", Utf16LE, "\x44\xC3\xA9\xC5\xA1\xC5\xA5"    , Utf8   , kErrorOk),

    // Tests full-width digit zero (3 BYTEs per UTF-8 character).
    ENTRY("\xEF\xBC\x90"                    , Utf8   , "\xFF\x10"                        , Utf16BE, kErrorOk),
    ENTRY("\xEF\xBC\x90"                    , Utf8   , "\x10\xFF"                        , Utf16LE, kErrorOk),
    ENTRY("\xFF\x10"                        , Utf16BE, "\xEF\xBC\x90"                    , Utf8   , kErrorOk),
    ENTRY("\x10\xFF"                        , Utf16LE, "\xEF\xBC\x90"                    , Utf8   , kErrorOk),

    // Tests `kCharMax` character (4 BYTEs per UTF-8 character, the highest possible unicode code-point).
    ENTRY("\xF4\x8F\xBF\xBF"                , Utf8   , "\xDB\xFF\xDF\xFF"                , Utf16BE, kErrorOk),
    ENTRY("\xF4\x8F\xBF\xBF"                , Utf8   , "\xFF\xDB\xFF\xDF"                , Utf16LE, kErrorOk),
    ENTRY("\xDB\xFF\xDF\xFF"                , Utf16BE, "\xF4\x8F\xBF\xBF"                , Utf8   , kErrorOk),
    ENTRY("\xFF\xDB\xFF\xDF"                , Utf16LE, "\xF4\x8F\xBF\xBF"                , Utf8   , kErrorOk),

    #if B2D_ARCH_LE
    ENTRY("Test"                            , Utf8   , "T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , kErrorOk),
    ENTRY("\0T\0e\0s\0t"                    , Utf16BE, "T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , kErrorOk),
    ENTRY("T\0e\0s\0t\0"                    , Utf16LE, "T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , kErrorOk),
    ENTRY("T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , "T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , kErrorOk),
    ENTRY("T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , "T\0e\0s\0t\0"                    , Utf16LE, kErrorOk),
    ENTRY("T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , "\0T\0e\0s\0t"                    , Utf16BE, kErrorOk),
    ENTRY("T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , "Test"                            , Latin1 , kErrorOk),
    ENTRY("T\0\0\0e\0\0\0s\0\0\0t\0\0\0"    , Utf32  , "Test"                            , Utf8   , kErrorOk),
    #else
    ENTRY("Test"                            , Utf8   , "\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , kErrorOk),
    ENTRY("\0T\0e\0s\0t"                    , Utf16BE, "\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , kErrorOk),
    ENTRY("T\0e\0s\0t\0"                    , Utf16LE, "\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , kErrorOk),
    ENTRY("\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , "\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , kErrorOk),
    ENTRY("\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , "T\0e\0s\0t\0"                    , Utf16LE, kErrorOk),
    ENTRY("\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , "\0T\0e\0s\0t"                    , Utf16BE, kErrorOk)
    ENTRY("\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , "Test"                            , Latin1 , kErrorOk),
    ENTRY("\0\0\0T\0\0\0e\0\0\0s\0\0\0t"    , Utf32  , "Test"                            , Utf8   , kErrorOk),
    #endif

    // Truncated characters.
    ENTRY(""                                , Utf8   , "\xC5"                            , Utf8   , kErrorTruncatedString),
    ENTRY(""                                , Utf8   , "\xEF"                            , Utf8   , kErrorTruncatedString),
    ENTRY(""                                , Utf8   , "\xEF\xBC"                        , Utf8   , kErrorTruncatedString),
    ENTRY(""                                , Utf8   , "\xF4"                            , Utf8   , kErrorTruncatedString),
    ENTRY(""                                , Utf8   , "\xF4\x8F"                        , Utf8   , kErrorTruncatedString),
    ENTRY(""                                , Utf8   , "\xF4\x8F\xBF"                    , Utf8   , kErrorTruncatedString),

    // Truncated character at the end (the converter must output the content, which was correct).
    ENTRY("a"                               , Utf8   , "a\xF4\x8F\xBF"                   , Utf8   , kErrorTruncatedString),
    ENTRY("ab"                              , Utf8   , "ab\xF4\x8F\xBF"                  , Utf8   , kErrorTruncatedString),
    ENTRY("TestString"                      , Utf8   , "TestString\xC5"                  , Utf8   , kErrorTruncatedString),
    ENTRY("\0T\0e\0s\0t\0S\0t\0r\0i\0n\0g"  , Utf16BE, "TestString\xC5"                  , Utf8   , kErrorTruncatedString),
    ENTRY("T\0e\0s\0t\0S\0t\0r\0i\0n\0g\0"  , Utf16LE, "TestString\xC5"                  , Utf8   , kErrorTruncatedString),

    // Invalid UTf-8 characters.
    ENTRY(""                                , Utf8   , "\x80"                            , Utf8   , kErrorInvalidString),
    ENTRY(""                                , Utf8   , "\xC1"                            , Utf8   , kErrorInvalidString),
    ENTRY(""                                , Utf8   , "\xF5\x8F\xBF\xBF"                , Utf8   , kErrorInvalidString),
    ENTRY(""                                , Utf8   , "\x91\x8F\xBF\xBF"                , Utf8   , kErrorInvalidString),
    ENTRY(""                                , Utf8   , "\xF6\x8F\xBF\xBF"                , Utf8   , kErrorInvalidString),
    ENTRY(""                                , Utf8   , "\xF4\xFF\xBF\xBF"                , Utf8   , kErrorInvalidString),

    // Overlong UTF-8 characters.
    ENTRY(""                                , Utf8   , "\xC0\xA0"                        , Utf8   , kErrorInvalidString)

    #undef ENTRY
  };

  for (size_t i = 0; i < B2D_ARRAY_SIZE(testEntries); i++) {
    const TestEntry& entry = testEntries[i];

    char output[20];
    ConversionState state;

    Error err = convertString(output   , 20           , entry.dstEncoding,
                              entry.src, entry.srcSize, entry.srcEncoding, state);

    bool failed = (err != entry.result) ||
                  (state.dstIndex() != entry.dstSize) ||
                  (std::memcmp(output, entry.dst, state.dstIndex()) != 0);

    if (failed) {
      size_t inputSize = entry.srcSize;
      size_t outputSize = state.dstIndex();
      size_t expectedSize = entry.dstSize;

      std::printf("  Failed Entry #%u\n", unsigned(i));

      std::printf("    Input   :");
      for (size_t j = 0; j < inputSize; j++)
        std::printf(" %02X", uint8_t(entry.src[j]));
      std::printf("%s\n", inputSize ? "" : " (Nothing)");

      std::printf("    Output  :");
      for (size_t j = 0; j < outputSize; j++)
        std::printf(" %02X", uint8_t(output[j]));
      std::printf("%s\n", outputSize ? "" : " (Nothing)");

      std::printf("    Expected:");
      for (size_t j = 0; j < expectedSize; j++)
        std::printf(" %02X", uint8_t(entry.dst[j]));
      std::printf("%s\n", expectedSize ? "" : " (Nothing)");

      std::printf("    ErrorCode: Actual(%u) %s Expected(%u)\n",
                  err,
                  (err == entry.result) ? "==" : "!=",
                  entry.result);
    }

    EXPECT(!failed);
  }
}
#endif

B2D_END_SUB_NAMESPACE
