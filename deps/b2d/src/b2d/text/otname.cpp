// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/lookuptable_p.h"
#include "../core/support.h"
#include "../core/unicode.h"
#include "../text/otface_p.h"
#include "../text/otname_p.h"
#include "../text/otplatform_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

#if defined(B2D_TRACE_OT) && !defined(B2D_TRACE_OT_NAME)
  #define B2D_TRACE_OT_NAME
#endif

#ifdef B2D_TRACE_OT_NAME
  #define B2D_LOG_OT_NAME(...) DebugUtils::debugFormat(__VA_ARGS__)
#else
  #define B2D_LOG_OT_NAME(...) (void)0
#endif

namespace NameImpl {

// ============================================================================
// [b2d::OT::NameImpl - Utilities]
// ============================================================================

template<typename T>
constexpr bool isAsciiAlpha(const T& x) noexcept { return T(x | 0x20) >= T('a') && T(x | 0x20) <= T('z'); }

template<typename T>
constexpr bool isAsciiAlnum(const T& x) noexcept { return isAsciiAlpha(x) || (x >= T('0') && x <= T('9')); }

template<typename T>
constexpr T asciiToLower(const T& x) noexcept { return (x >= T(uint8_t('A')) && x <= T(uint8_t('Z'))) ? T(x |  T(0x20)) : x; }

template<typename T>
constexpr T asciiToUpper(const T& x) noexcept { return (x >= T(uint8_t('a')) && x <= T(uint8_t('z'))) ? T(x & ~T(0x20)) : x; }

static uint32_t encodingFromPlatformId(uint32_t platformId) noexcept {
  // Both Unicode and Windows platform use 'UTF16-BE' encoding.
  bool isUnicode = platformId == Platform::kPlatformUnicode ||
                   platformId == Platform::kPlatformWindows ;
  return isUnicode ? Unicode::kEncodingUtf16BE : Unicode::kEncodingLatin1;
}

// Helper function that tries to trim space and dashes that could be left-overs
// from removing subfamily name from family name.
static size_t trimRightIndexAfterChange(const char* data, size_t i) noexcept {
  // The following is an attempt to remove "-" and "- ". Some fonts use dash
  // character "-" as a separator between its family-name and subfamily-name.
  if (i && data[i - 1] == ' ') i--; // Remove a possible ' ' (space).
  if (i && data[i - 1] == '-') i--; // Remove a possible '-' (dash).

  // Trim all remaining spaces.
  while (i && data[i - 1] == ' ')
    i--;

  return i;
}

static Error normalizeFamilyAndSubfamily(OTFaceImpl* faceI) noexcept {
  auto& familyName = faceI->_familyName;
  auto& subfamilyName = faceI->_subfamilyName;

  // Some fonts duplicate font subfamily-name in family-name, we try to match
  // such cases and truncate the sub-family in such case.
  if (familyName.size() >= subfamilyName.size() && !subfamilyName.empty()) {
    // Base size is a size of family name after the whole subfamily was removed
    // from it (if matched). It's basically the minimum length we would end up
    // when subfamily-name matches the end of family-name fully.
    size_t baseSize = familyName.size() - subfamilyName.size();
    if (std::memcmp(familyName.data() + baseSize, subfamilyName.data(), subfamilyName.size()) == 0) {
      B2D_LOG_OT_NAME("  [FIXUP] Subfamily '%s' is redundant\n", subfamilyName.data());
      subfamilyName.reset();
      faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagRedundantSubfamilyName;
    }
  }

  return kErrorOk;
}

// ============================================================================
// [b2d::OT::NameImpl - Init]
// ============================================================================

Error initName(OTFaceImpl* faceI, FontData& fontData) noexcept {
  typedef NameTable T;

  FontDataTable<NameTable> name;
  if (fontData.queryTableByTag(&name, FontTag::name) == 0 || !name.fitsTable())
    return DebugUtils::errored(kErrorFontInvalidData);

  B2D_LOG_OT_NAME("OT::Init 'name'\n");
  B2D_LOG_OT_NAME("  Size: %zu\n", name.size());

  if (B2D_UNLIKELY(name.size() < NameTable::kMinSize))
    return DebugUtils::errored(kErrorFontInvalidData);

  uint32_t format = name->format();
  uint32_t recordCount = name->recordCount();

  B2D_LOG_OT_NAME("  Format: %u\n", format);
  B2D_LOG_OT_NAME("  RecordCount: %u\n", recordCount);

  uint32_t stringRegionOffset = name->stringOffset();
  if (B2D_UNLIKELY(stringRegionOffset >= name.size()))
    return DebugUtils::errored(kErrorFontInvalidData);

  // Only formats 0 and 1 are defined.
  if (B2D_UNLIKELY(format > 1))
    return DebugUtils::errored(kErrorFontInvalidData);

  // There must be some names otherwise this table is invalid. Also make sure
  // that the number of records doesn't overflow the size of 'name' itself.
  if (B2D_UNLIKELY(recordCount == 0) || !name.fitsSize(6 + recordCount * sizeof(T::NameRecord)))
    return DebugUtils::errored(kErrorFontInvalidData);

  // Mask of name IDs which we are interested in.
  //
  // NOTE: We are not interested in WWS family and subfamily names as those may
  // include subfamilies, which we expect to be separate. We would only use WWS
  // names if there is no other choice.
  constexpr uint32_t kImportantNameIdMask =
    Support::bitMask<uint32_t>(
      T::kIdFamilyName,
      T::kIdSubfamilyName,
      T::kIdFullName,
      T::kIdPostScriptName,
      T::kIdTypographicFamilyName,
      T::kIdTypographicSubfamilyName,
      T::kIdWWSFamilyName,
      T::kIdWWSSubfamilyName);

  // Scoring is used to select the best records as the same NameId can be
  // repeated multiple times having a different `platformId`, `specificId`,
  // and `languageId`.
  uint16_t nameIdScore[T::kIdStandarizedNamesCount] = { 0 }; // Score of each interesting NameId.
  uint32_t nameIdIndex[T::kIdStandarizedNamesCount];         // Record index of matched NameId.
  uint32_t nameUtf8Size[T::kIdStandarizedNamesCount];        // UTF-8 length of a matched NameId.
  uint32_t nameIdMask = 0;                                   // Mask of all matched NameIds.

  Utf8String tmpString;

  const T::NameRecord* nameRecords = name->nameRecords();
  uint32_t stringRegionSize = uint32_t(name.size() - stringRegionOffset);

  for (uint32_t i = 0; i < recordCount; i++) {
    const T::NameRecord& nameRecord = nameRecords[i];

    // Don't bother with a NameId we are not interested in.
    uint32_t nameId = nameRecord.nameId();
    if (nameId >= T::kIdStandarizedNamesCount || !(kImportantNameIdMask & (1 << nameId)))
      continue;

    uint32_t stringOffset = nameRecord.offset();
    uint32_t stringLength = nameRecord.length();

    // Offset could be anything if length is zero.
    if (stringLength == 0)
      stringOffset = 0;

    // Fonts are full of wrong data, if the offset is outside of the string data we simply skip the record.
    if (B2D_UNLIKELY(stringOffset >= stringRegionSize || stringRegionSize - stringOffset < stringLength)) {
      B2D_LOG_OT_NAME("  [WARN] Invalid Region {NameId=%u Offset=%u Length=%u}", stringOffset, stringLength);
      continue;
    }

    uint32_t platformId = nameRecord.platformId();
    uint32_t specificId = nameRecord.specificId();
    uint32_t languageId = nameRecord.languageId();

    uint32_t score = 0;
    switch (platformId) {
      case Platform::kPlatformUnicode: {
        score = 3;
        break;
      }

      case Platform::kPlatformMac: {
        // Sucks, but better than nothing...
        if (specificId == Platform::kMacEncodingRoman)
          score = 2;
        else
          continue;

        if (languageId == Platform::kMacLanguageEnglish) {
          score |= (0x01u << 8);
        }

        break;
      }

      case Platform::kPlatformWindows: {
        if (specificId == Platform::kWindowsEncodingSymbol)
          score = 1;
        else if (specificId == Platform::kWindowsEncodingUCS2)
          score = 4;
        else
          continue;

        // We use the term "locale" instead of "language" when it comes to
        // Windows platform. Locale specifies both primary language and
        // sub-language, which is usually related to a geographic location.
        uint32_t localeId = languageId;
        uint32_t primaryLangId = localeId & 0xFFu;

        // Check primary language.
        if (primaryLangId == Platform::kWindowsLanguageEnglish) {
          if (localeId == Platform::kWindowsLocaleEnglish_US)
            score |= (0x04u << 8);
          else if (localeId == Platform::kWindowsLocaleEnglish_UK)
            score |= (0x03u << 8);
          else
            score |= (0x02u << 8);
        }
        break;
      }
    }

    if (score) {
      // Make sure this string is decodable before using this entry.
      uint32_t encoding = encodingFromPlatformId(platformId);
      Unicode::ValidationState validationState;
      Unicode::ConversionState conversionState;

      const void* src = name.data<char>() + stringRegionOffset + stringOffset;
      Error err = Unicode::validateString(src, stringLength, encoding, validationState);

      if (!err) {
        size_t utf8Size = validationState.utf8Index();
        err = tmpString.resize(utf8Size);
        if (!err) {
          Unicode::convertString(tmpString.data(), utf8Size, Unicode::kEncodingUtf8, src, stringLength, encoding, conversionState);
          size_t verifiedSize = std::strlen(tmpString.data());
          if (verifiedSize != utf8Size) {
            if (verifiedSize + 1 == utf8Size) {
              // Font-data contains null terminator at the end.
              tmpString.truncate(verifiedSize);
            }
            else {
              // Either more null terminators or the data is corrupted or in other encoding
              // we don't understand. There are some fonts that store some names in UTF32-BE
              // encoding, we refuse these names as it's not anywhere in the specification.
              err = DebugUtils::errored(kErrorFontInvalidData);

              #ifdef B2D_TRACE_OT_NAME
              B2D_LOG_OT_NAME("  [WARN] Failed to decode '%s' <- [", tmpString.data());
              for (size_t j = 0; j < stringLength; j++)
                B2D_LOG_OT_NAME(" %02X", static_cast<const uint8_t*>(src)[j]);
              B2D_LOG_OT_NAME(" ]\n");
              #endif
            }
          }
        }
      }

      if (err) {
        score = 0;
        faceI->_diagnosticsInfo._flags |= FontDiagnosticsInfo::kFlagInvalidNameData;
      }
      else {
        // If this is a subfamily (NameId=2) on a MAC platform and it's empty
        // we prefer it, because many fonts have this field correctly empty on
        // MAC platform and filled incorrectly on Windows platform.
        if (platformId == Platform::kPlatformMac && nameId == T::kIdSubfamilyName && tmpString.empty())
          score = 0xFFFFu;
      }

      #ifdef B2D_TRACE_OT_NAME
      B2D_LOG_OT_NAME("  [%s] \"%s\" [Size=%u] {NameId=%u PlatformId=%u SpecificId=%u LanguageId=%u Score=%u}\n",
                      score > nameIdScore[nameId] ? "SELECT" : "DROP",
                      err ? "Failed" : tmpString.data(),
                      stringLength,
                      nameId,
                      platformId,
                      specificId,
                      languageId,
                      score);
      #endif

      // Update if we have found a better candidate or this is was first one.
      if (score > nameIdScore[nameId]) {
        nameIdScore[nameId] = uint16_t(score);
        nameIdIndex[nameId] = i;
        nameUtf8Size[nameId] = uint32_t(tmpString.size());
        nameIdMask |= Support::bitMask<uint32_t>(nameId);
      }
    }
  }

  // Prefer TypographicFamilyName over FamilyName and WWSFamilyName.
  if (Support::bitTest(nameIdMask, T::kIdTypographicFamilyName)) {
    nameIdMask &= ~Support::bitMask<uint32_t>(T::kIdFamilyName, T::kIdWWSFamilyName);
  }

  // Prefer TypographicSubfamilyName over SubfamilyName and WWSSubfamilyName.
  if (Support::bitTest(nameIdMask, T::kIdTypographicSubfamilyName)) {
    nameIdMask &= ~Support::bitMask<uint32_t>(T::kIdSubfamilyName, T::kIdWWSSubfamilyName);
  }

  if (Support::bitMatch(nameIdMask, Support::bitMask<uint32_t>(T::kIdTypographicFamilyName, T::kIdTypographicSubfamilyName))) {
    B2D_LOG_OT_NAME("  Has Typographic FamilyName and SubfamilyName\n");
    faceI->_faceFlags |= FontFace::kFlagTypographicNames;
  }

  Support::BitWordIterator<uint32_t> bitWordIterator(nameIdMask);
  while (bitWordIterator.hasNext()) {
    uint32_t nameId = bitWordIterator.next();
    const T::NameRecord& nameRecord = nameRecords[nameIdIndex[nameId]];

    uint32_t platformId = nameRecord.platformId();
    uint32_t stringOffset = nameRecord.offset();
    uint32_t stringLength = nameRecord.length();

    // Offset could be anything if length is zero.
    if (stringLength == 0)
      stringOffset = 0;

    // This should have already been filtered out, but one is never sure...
    if (B2D_UNLIKELY(stringOffset >= stringRegionSize || stringRegionSize - stringOffset < stringLength))
      return DebugUtils::errored(kErrorFontInvalidData);

    // Make `stringOffset` relative to the start of the table as it's specified
    // relatively to the start of string data (not 'name' table).
    stringOffset += stringRegionOffset;

    // Now we are at the end of the string. We know it's valid and we know its UTF8 size.
    char* utf8Dst = nullptr;
    size_t utf8Size = nameUtf8Size[nameId];
    uint32_t encoding = encodingFromPlatformId(platformId);

    switch (nameId) {
      case T::kIdFullName:
        B2D_PROPAGATE(faceI->_fullName.resize(utf8Size));
        utf8Dst = faceI->_fullName.data();
        break;

      case T::kIdFamilyName:
      case T::kIdWWSFamilyName:
      case T::kIdTypographicFamilyName:
        B2D_PROPAGATE(faceI->_familyName.resize(utf8Size));
        utf8Dst = faceI->_familyName.data();
        break;

      case T::kIdSubfamilyName:
      case T::kIdWWSSubfamilyName:
      case T::kIdTypographicSubfamilyName:
        B2D_PROPAGATE(faceI->_subfamilyName.resize(utf8Size));
        utf8Dst = faceI->_subfamilyName.data();
        break;

      case T::kIdPostScriptName:
        B2D_PROPAGATE(faceI->_postScriptName.resize(utf8Size));
        utf8Dst = faceI->_postScriptName.data();
        break;
    }

    if (utf8Dst) {
      // This should not fail as we have already validated the string.
      Unicode::ConversionState conversionState;

      // It could fail with NoSpaceLeft error, but we don't care as it may be
      // a null terminator that we have already truncated before we selected
      // this NameId.
      Unicode::convertString(
        utf8Dst, utf8Size, Unicode::kEncodingUtf8,
        name.data<char>() + stringOffset, stringLength, encoding, conversionState);
    }
  }

  B2D_PROPAGATE(normalizeFamilyAndSubfamily(faceI));
  B2D_LOG_OT_NAME("  Family=%s [SubFamily=%s] {PostScriptName=%s}\n",
                  faceI->familyName(),
                  faceI->subfamilyName(),
                  faceI->postScriptName());
  return kErrorOk;
}

} // NameImpl namespace

B2D_END_SUB_NAMESPACE
