// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_UNICODE_H
#define _B2D_CORE_UNICODE_H

// [Dependencies]
#include "../core/support.h"
#include "../core/unicodedata.h"

B2D_BEGIN_SUB_NAMESPACE(Unicode)

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Unicode - Characters]
// ============================================================================

//! Special unicode characters.
enum CharConstants : uint32_t {
  kCharBOM                = 0x00FEFFu,   //!< Native Byte-Order-Mark.
  kCharMax                = 0x10FFFFu,   //!< Last code-point.

  kCharReplacement        = 0x00FFFDu,   //!< Replacement character.

  kCharFVS1               = 0x00180Bu,   //!< First char in Mongolian 'free variation selectors' FVS1..FVS3.
  kCharFVS3               = 0x00180Du,   //!< Last char in Mongolian 'free variation selectors' FVS1..FVS3.

  kCharVS1                = 0x00FE00u,   //!< First char in 'variation selectors' VS1..VS16.
  kCharVS16               = 0x00FE0Fu,   //!< Last char in 'variation selectors' VS1..VS16.

  kCharVS17               = 0x0E0100u,   //!< First char in 'variation selectors supplement' VS17..VS256.
  kCharVS256              = 0x0E01EFu,   //!< Last char in 'variation selectors supplement' VS17..VS256.

  kCharFirstSurrogate     = 0x00D800u,   //!< First surrogate code-point.
  kCharLastSurrogate      = 0x00DFFFu,   //!< Last surrogate code-point.

  kCharFirstHighSurrogate = 0x00D800u,   //!< First high-surrogate code-point
  kCharLastHighSurrogate  = 0x00DBFFu,   //!< Last high-surrogate code-point

  kCharFirstLowSurrogate  = 0x00DC00u,   //!< First low-surrogate code-point
  kCharLastLowSurrogate   = 0x00DFFFu    //!< Last low-surrogate code-point
};

// ============================================================================
// [b2d::Unicode - EncodingId]
// ============================================================================

//! Encoding ID.
//!
//! Encoding IDs provided by Blend2D match mostly Blend2D's requirements. UTF-8
//! is the first-class citizen in Blend2D and the preferred encoding for most
//! APIs, other encodings are required to support foreign APIs like Windows API
//! and reading text from files like images and fonts.
//!
//! Here are the reasons for each encoding supported by Blend2D:
//!
//!   - LATIN1   - This was added only to support old encodings like MacRoman in
//!                TrueType/OpenType fonts and to support APIs that use different
//!                string representations based on string content (like V8 engine).
//!
//!                NOTE: Latin1 encoding in Blend2D sense is encoding where each
//!                byte (value from 0..255) represents exactly these code points.
//!                It's not the same as ISO-8859-1, WINDOWS-1252, or others that
//!                require mapping tables.
//!
//!   - UTF8     - This is the default encoding Blend2D uses in various APIs.
//!
//!   - UTF16    - Native endian UTF-16, set to either UTF16-LE or UTF16-BE.
//!
//!   - UTF16-LE - Required to support unicode on Windows (WinAPI is UTF-16) and
//!                Mac (CoreFoundation), also used by Blend2D where applicable.
//!
//!   - UTF16-BE - Required to read unicode text from some files, like TrueType
//!                and OpenType fonts which store names in big endian.
//!
//!   - UTF32    - Ideal representation for text shaping, required to support
//!                DirectWrite and Harfbuzz.
//!
//! NOTE: It's not planned to extend encodings provided by Blend2D by 8-bit
//! code-page encodings. The Latin1 is the only non-unicode encoding exception
//! that is always used as an input and never as output. In addition, UTF-32
//! encoding is assumed to have machine endian as this encoding is not commonly
//! used for transfer or storage (as it's too wasteful). That's the main reason
//! that endianness can only be specified for UTF-16 encoding.
enum EncodingId : uint32_t {
  kEncodingUtf8       = 0,
  kEncodingLatin1     = 1,
  kEncodingUtf16LE    = 2,
  kEncodingUtf16BE    = 3,
  kEncodingUtf16      = B2D_ARCH_LE ? kEncodingUtf16LE : kEncodingUtf16BE,
  kEncodingUtf16Swap  = B2D_ARCH_LE ? kEncodingUtf16BE : kEncodingUtf16LE,
  kEncodingUtf32      = 4
};

// ============================================================================
// [b2d::Unicode - Utilities]
// ============================================================================

template<typename T>
static B2D_INLINE uint32_t utf8CharSize(T c) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return Data::utf8SizeTable[U(c)];
}

template<typename T>
static B2D_INLINE bool isValidUtf8(T c) noexcept {
  typedef typename std::make_unsigned<T>::type U;
  return U(c) < 128 || (U(c) - U(194) < U(245 - 194));
}

//! Get whether the unicode character `uc` is high or low surrogate.
template<typename T>
constexpr bool isSurrogate(const T& uc) noexcept { return uc >= kCharFirstSurrogate && uc <= kCharLastSurrogate; }

//! Get whether the unicode character `uc` is a high (leading) surrogate.
template<typename T>
constexpr bool isHighSurrogate(const T& uc) noexcept { return uc >= kCharFirstHighSurrogate && uc <= kCharLastHighSurrogate; }

//! Get whether the unicode character `uc` is a low (trailing) surrogate.
template<typename T>
constexpr bool isLowSurrogate(const T& uc) noexcept { return uc >= kCharFirstLowSurrogate && uc <= kCharLastLowSurrogate; }

//! Compose `hi` and `lo` surrogates into an unicode code-point.
template<typename T>
constexpr uint32_t utf32FromSurrogate(const T& hi, const T& lo) noexcept {
  return (uint32_t(hi) << 10) + uint32_t(lo) - uint32_t((kCharFirstSurrogate << 10) + kCharFirstLowSurrogate - 0x10000u);
}

//! Decompose an unicode code-point into  `hi` and `lo` surrogates.
template<typename T>
static B2D_INLINE void utf32ToSurrogate(uint32_t uc, T& hi, T& lo) noexcept {
  uc -= 0x10000u;
  hi = T(kCharFirstHighSurrogate | (uc >> 10));
  lo = T(kCharFirstLowSurrogate  | (uc & 0x3FFu));
}

// ============================================================================
// [b2d::Unicode - Validation]
// ============================================================================

struct ValidationState {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _utf8Index = 0;
    _utf16Index = 0;
    _utf32Index = 0;
  }

  B2D_INLINE bool hasSMP() const noexcept { return _utf16Index != _utf32Index; }

  B2D_INLINE size_t utf8Index() const noexcept { return _utf8Index; }
  B2D_INLINE size_t utf16Index() const noexcept { return _utf16Index; }
  B2D_INLINE size_t utf32Index() const noexcept { return _utf32Index; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _utf8Index;
  size_t _utf16Index;
  size_t _utf32Index;
};

B2D_API Error validateString(const void* data, size_t sizeInBytes, uint32_t encoding, ValidationState& state) noexcept;

static B2D_INLINE Error validateUtf8(const char* data, size_t size, ValidationState& state) noexcept { return validateString(data, size, kEncodingUtf8, state); }
static B2D_INLINE Error validateUtf16(const uint16_t* data, size_t size, ValidationState& state) noexcept { return validateString(data, size * 2u, kEncodingUtf16, state); }
static B2D_INLINE Error validateUtf32(const uint32_t* data, size_t size, ValidationState& state) noexcept { return validateString(data, size * 4u, kEncodingUtf32, state); }

// ============================================================================
// [b2d::Unicode - Conversion]
// ============================================================================

struct ConversionState {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _dstIndex = 0;
    _srcIndex = 0;
  }

  B2D_INLINE size_t dstIndex() const noexcept { return _dstIndex; }
  B2D_INLINE size_t srcIndex() const noexcept { return _srcIndex; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  size_t _dstIndex;
  size_t _srcIndex;
};

B2D_API Error _convertString(void* dst, size_t dstSizeInBytes, const void* src, size_t srcSizeInBytes, uint32_t flags, ConversionState& state) noexcept;

//! Convert a string from one encoding to another.
//!
//! Convert function works at byte-level. All sizes here are including those
//! stored in a `ConversionState` are byte entities. So for example to convert
//! a single UTF-16 BMP character the source size must be 2, etc...
static B2D_INLINE Error convertString(
    void      * dst, size_t dstSizeInBytes, uint32_t dstEncoding,
    const void* src, size_t srcSizeInBytes, uint32_t srcEncoding, ConversionState& state) noexcept {
  return _convertString(dst, dstSizeInBytes, src, srcSizeInBytes, (dstEncoding << 8) | srcEncoding, state);
}

static B2D_INLINE Error utf8FromLatin1(char* dst, size_t dstSize, const char* src, size_t srcSize, ConversionState& state) noexcept {
  return convertString(dst, dstSize, kEncodingUtf8, src, srcSize, kEncodingLatin1, state);
}

static B2D_INLINE Error utf8FromUtf16(char* dst, size_t dstSize, const uint16_t* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize, kEncodingUtf8, src, srcSize * 2u, kEncodingUtf16, state);
  state._srcIndex /= 2;
  return err;
}

static B2D_INLINE Error utf8FromUtf32(char* dst, size_t dstSize, const uint32_t* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize, kEncodingUtf8, src, srcSize * 4u, kEncodingUtf32, state);
  state._srcIndex /= 4;
  return err;
}

static B2D_INLINE Error utf16FromLatin1(uint16_t* dst, size_t dstSize, const char* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 2u, kEncodingUtf16, src, srcSize, kEncodingLatin1, state);
  state._dstIndex /= 2;
  return err;
}

static B2D_INLINE Error utf16FromUtf8(uint16_t* dst, size_t dstSize, const char* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 2u, kEncodingUtf16, src, srcSize, kEncodingUtf8, state);
  state._dstIndex /= 2;
  return err;
}

static B2D_INLINE Error utf16FromUtf32(uint16_t* dst, size_t dstSize, const uint32_t* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 2u, kEncodingUtf16, src, srcSize * 4u, kEncodingUtf32, state);
  state._dstIndex /= 2;
  state._srcIndex /= 4;
  return err;
}

static B2D_INLINE Error utf32FromLatin1(char* dst, size_t dstSize, const char* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 4u, kEncodingUtf32, src, srcSize, kEncodingLatin1, state);
  state._dstIndex /= 4;
  return err;
}

static B2D_INLINE Error utf32FromUtf8(char* dst, size_t dstSize, const char* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 4u, kEncodingUtf32, src, srcSize, kEncodingUtf8, state);
  state._dstIndex /= 4;
  return err;
}

static B2D_INLINE Error utf32FromUtf16(char* dst, size_t dstSize, const uint16_t* src, size_t srcSize, ConversionState& state) noexcept {
  Error err = convertString(dst, dstSize * 4u, kEncodingUtf32, src, srcSize * 2u, kEncodingUtf32, state);
  state._dstIndex /= 4;
  state._srcIndex /= 2;
  return err;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_CORE_UNICODE_H
