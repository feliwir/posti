// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTGLOBALS_P_H
#define _B2D_TEXT_OTGLOBALS_P_H

// [Dependencies]
#include "../core/globals.h"
#include "../core/support.h"
#include "../text/fontdata.h"
#include "../text/fonttag.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

//! \internal
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class OTFaceImpl;

// ============================================================================
// [b2d::OT::OTUtils]
// ============================================================================

template<typename TypeT, typename TableT>
constexpr const TypeT* fieldAt(const TableT* table, size_t offset) noexcept {
  return reinterpret_cast<const TypeT*>(reinterpret_cast<const char*>(table) + offset);
}

namespace OTUtils {
  // OpenType read IO, used where we cannot access data described as structs.
  // Calling these functions would also convert all data from BigEndian to
  // machine endian.

  template<typename T>
  B2D_INLINE int32_t readI8(const T* p) noexcept { return Support::readI8(p); }

  template<typename T>
  B2D_INLINE int32_t readI16(const T* p) noexcept { return Support::readI16uBE(p); }

  template<typename T>
  B2D_INLINE int32_t readI32(const T* p) noexcept { return Support::readI32uBE(p); }

  template<typename T>
  B2D_INLINE uint32_t readU8(const T* p) noexcept { return Support::readU8(p); }

  template<typename T>
  B2D_INLINE uint32_t readU16(const T* p) noexcept { return Support::readU16uBE(p); }

  template<typename T>
  B2D_INLINE uint32_t readU24(const T* p) noexcept { return Support::readU24uBE(p); }

  template<typename T>
  B2D_INLINE uint32_t readU32(const T* p) noexcept { return Support::readU32uBE(p); }

  template<typename T>
  B2D_INLINE uint32_t readOffset(const T* p, size_t offsetSize) noexcept {
    const uint8_t* oPtr = reinterpret_cast<const uint8_t*>(p);
    const uint8_t* oEnd = oPtr + offsetSize;

    uint32_t offset = 0;
    for (;;) {
      offset |= oPtr[0];
      if (++oPtr == oEnd)
        break;
      offset <<= 8;
    }
    return offset;
  }

  template<typename T>
  B2D_INLINE void readOffsetArray(const T* p, size_t offsetSize, uint32_t* offsetArrayOut, size_t n) noexcept {
    const uint8_t* oPtr = reinterpret_cast<const uint8_t*>(p);
    const uint8_t* oEnd = oPtr;

    for (size_t i = 0; i < n; i++) {
      uint32_t offset = 0;
      oEnd += offsetSize;

      for (;;) {
        offset |= oPtr[0];
        if (++oPtr == oEnd)
          break;
        offset <<= 8;
      }

      offsetArrayOut[i] = offset;
    }
  }

  // OpenType stores many fields as `int16_t`, we sometimes need to negate such
  // fields and store again as `int16_t`. These inlines ensure that we will not
  // trigger any undefined behavior by doing so...

  template<typename T>
  B2D_INLINE T safeNeg(const T& x) noexcept {
    return -(Math::max<T>(x, std::numeric_limits<T>::lowest() + 1));
  }

  template<typename T>
  B2D_INLINE T safeAbs(const T& x) noexcept {
    return Math::abs(Math::max<T>(x, std::numeric_limits<T>::lowest() + 1));
  }
}

// ============================================================================
// [b2d::OT::OTTypeIO]
// ============================================================================

template<size_t Size>
struct OTTypeIO {
  // ReadValue converts from BigEndian to machine endian, ReadRawValue doesn't do
  // that and should be faster on architectures where unaligned memory access is ok.
};

template<>
struct OTTypeIO<1> {
  template<size_t Alignment = 1>
  static B2D_INLINE uint32_t readValue(const uint8_t* data) noexcept { return Support::readU8(data); }

  template<size_t Alignment = 1>
  static B2D_INLINE void writeValue(uint8_t* data, uint32_t value) noexcept { Support::writeU8(data, value); }
};

template<>
struct OTTypeIO<2> {
  template<size_t Alignment = 1>
  static B2D_INLINE uint32_t readValue(const uint8_t* data) noexcept { return Support::readU16xBE<Alignment>(data); }

  template<size_t Alignment = 1>
  static B2D_INLINE void writeValue(uint8_t* data, uint32_t value) noexcept { Support::writeU16xBE<Alignment>(data, value); }
};

template<>
struct OTTypeIO<3> {
  // NOTE: Alignment doesn't make sense here, but it's a template and it must be compatible with others.

  template<size_t Alignment = 1>
  static B2D_INLINE uint32_t readValue(const uint8_t* data) noexcept { return Support::readU24uBE(data); }

  template<size_t Alignment = 1>
  static B2D_INLINE void writeValue(uint8_t* data, uint32_t value) noexcept { Support::writeU24uBE(data, value); }
};

template<>
struct OTTypeIO<4> {
  template<size_t Alignment = 1>
  static B2D_INLINE uint32_t readValue(const uint8_t* data) noexcept { return Support::readU32xBE<Alignment>(data); }

  template<size_t Alignment = 1>
  static B2D_INLINE void writeValue(uint8_t* data, uint32_t value) noexcept { Support::writeU32xBE<Alignment>(data, value); }
};

template<>
struct OTTypeIO<8> {
  template<size_t Alignment = 1>
  static B2D_INLINE uint64_t readValue(const uint8_t* data) noexcept { return Support::readU64xBE<Alignment>(data); }

  template<size_t Alignment = 1>
  static B2D_INLINE void writeValue(uint8_t* data, uint64_t value) noexcept { Support::writeU64xBE<Alignment>(data, value); }
};

// ============================================================================
// [b2d::OT::OTType]
// ============================================================================

#pragma pack(push, 1)
template<typename T, uint32_t Size>
struct OTType {
  typedef T Type;
  enum : uint32_t { kSize = Size };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline OTType() noexcept = default;
  constexpr OTType(const OTType& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<size_t Alignment = 1>
  B2D_INLINE T value() const noexcept {
    return T(OTTypeIO<Size>::template readValue<Alignment>(_data));
  }

  template<size_t Alignment = 1>
  B2D_INLINE void setValue(T value) noexcept {
    typedef typename std::make_unsigned<T>::type U;
    OTTypeIO<Size>::template writeValue<Alignment>(_data, U(value));
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE T operator()() const noexcept { return value(); }

  B2D_INLINE OTType& operator=(const OTType& other) noexcept = default;
  B2D_INLINE OTType& operator=(T other) noexcept { setValue(other); return *this; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _data[kSize];
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::OTTag]
// ============================================================================

#pragma pack(push, 1)
struct OTTag {
  typedef uint32_t Type;
  enum : uint32_t { kSize = 4 };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline OTTag() noexcept = default;
  constexpr OTTag(const OTTag& other) noexcept = default;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<size_t Alignment = 1>
  B2D_INLINE uint32_t value() const noexcept {
    return Support::readU32x<Globals::kByteOrderNative, Alignment>(_data);
  }

  template<size_t Alignment = 1>
  B2D_INLINE void setValue(uint32_t value) noexcept {
    return Support::writeU32x<Globals::kByteOrderNative, Alignment>(_data, value);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t operator()() const noexcept { return value(); }

  B2D_INLINE OTTag& operator=(const OTTag& other) noexcept = default;
  B2D_INLINE OTTag& operator=(uint32_t other) noexcept { setValue(other); return *this; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _data[kSize];
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::TypeDefs]
// ============================================================================

typedef OTType<int8_t  , 1> OTInt8;
typedef OTType<uint8_t , 1> OTUInt8;
typedef OTType<int16_t , 2> OTInt16;
typedef OTType<uint16_t, 2> OTUInt16;
typedef OTType<uint32_t, 3> OTUInt24;
typedef OTType<int32_t , 4> OTInt32;
typedef OTType<uint32_t, 4> OTUInt32;
typedef OTType<int64_t , 8> OTInt64;
typedef OTType<uint64_t, 8> OTUInt64;

typedef OTInt16  OTFWord;
typedef OTUInt16 OTUFWord;
typedef OTUInt16 OTF2Dot14;
typedef OTUInt32 OTFixed;
typedef OTUInt32 OTCheckSum;
typedef OTInt64 OTDateTime;

//! \}
//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTGLOBALS_P_H
