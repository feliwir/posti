// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_LOOKUPTABLE_P_H
#define _B2D_CORE_LOOKUPTABLE_P_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

//! \internal
//! \{

// ============================================================================
// [b2d::ByteLUT]
// ============================================================================

template<typename Predicate, uint32_t Index>
struct ByteLUT_ByteMask {
  enum : uint32_t {
    kValue = (uint32_t(Predicate::test(Index + 0)) << 0)
           | (uint32_t(Predicate::test(Index + 1)) << 1)
           | (uint32_t(Predicate::test(Index + 2)) << 2)
           | (uint32_t(Predicate::test(Index + 3)) << 3)
           | (uint32_t(Predicate::test(Index + 4)) << 4)
           | (uint32_t(Predicate::test(Index + 5)) << 5)
           | (uint32_t(Predicate::test(Index + 6)) << 6)
           | (uint32_t(Predicate::test(Index + 7)) << 7)
  };
};

template<typename Predicate, uint32_t Index>
struct ByteLUT_UInt32Mask {
  enum : uint32_t {
    kValue = ((ByteLUT_ByteMask<Predicate, Index +  0>::kValue) <<  0)
           | ((ByteLUT_ByteMask<Predicate, Index +  8>::kValue) <<  8)
           | ((ByteLUT_ByteMask<Predicate, Index + 16>::kValue) << 16)
           | ((ByteLUT_ByteMask<Predicate, Index + 32>::kValue) << 24)
  };
};

template<typename Predicate>
struct ByteLUT {
  static B2D_INLINE bool testInternal(size_t index) noexcept {
    static const uint32_t lut[256 / 32] = {
      ByteLUT_UInt32Mask<Predicate, 32 * 0>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 1>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 2>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 3>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 4>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 5>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 6>::kValue,
      ByteLUT_UInt32Mask<Predicate, 32 * 7>::kValue
    };
    return bool((lut[index / 32u] >> (index % 32u)) & 0x1);
  }

  template<typename T>
  static B2D_INLINE bool test(const T& value) noexcept {
    typedef typename std::make_unsigned<T>::type U;
    return testInternal(size_t(U(value)));
  }
};

// ============================================================================
// [B2D_LOOKUP_TABLE]
// ============================================================================

#define B2D_LOOKUP_TABLE_4(T, I) T((I)), T((I+1)), T((I+2)), T((I+3))
#define B2D_LOOKUP_TABLE_8(T, I) B2D_LOOKUP_TABLE_4(T, I), B2D_LOOKUP_TABLE_4(T, I + 4)
#define B2D_LOOKUP_TABLE_16(T, I) B2D_LOOKUP_TABLE_8(T, I), B2D_LOOKUP_TABLE_8(T, I + 8)
#define B2D_LOOKUP_TABLE_32(T, I) B2D_LOOKUP_TABLE_16(T, I), B2D_LOOKUP_TABLE_16(T, I + 16)
#define B2D_LOOKUP_TABLE_64(T, I) B2D_LOOKUP_TABLE_32(T, I), B2D_LOOKUP_TABLE_32(T, I + 32)
#define B2D_LOOKUP_TABLE_128(T, I) B2D_LOOKUP_TABLE_64(T, I), B2D_LOOKUP_TABLE_64(T, I + 64)
#define B2D_LOOKUP_TABLE_256(T, I) B2D_LOOKUP_TABLE_128(T, I), B2D_LOOKUP_TABLE_128(T, I + 128)
#define B2D_LOOKUP_TABLE_512(T, I) B2D_LOOKUP_TABLE_256(T, I), B2D_LOOKUP_TABLE_256(T, I + 256)
#define B2D_LOOKUP_TABLE_1024(T, I) B2D_LOOKUP_TABLE_512(T, I), B2D_LOOKUP_TABLE_512(T, I + 512)

//! \}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_LOOKUPTABLE_P_H
