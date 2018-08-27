// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Support - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
template<typename T>
B2D_INLINE bool testIsConsecutiveBitMask(T x) {
  if (x == 0)
    return false;

  T m = x;
  while ((m & 0x1) == 0)
    m >>= 1;
  return ((m + 1) & m) == 0;
}

template<typename T>
static void testArrays(const T* a, const T* b, size_t size) noexcept {
  for (size_t i = 0; i < size; i++)
    EXPECT(a[i] == b[i], "Mismatch at %u", unsigned(i));
}

static void testAlignment() noexcept {
  INFO("Support::isAligned()");
  EXPECT(Support::isAligned<size_t>(0xFFFF,  4) == false);
  EXPECT(Support::isAligned<size_t>(0xFFF4,  4) == true);
  EXPECT(Support::isAligned<size_t>(0xFFF8,  8) == true);
  EXPECT(Support::isAligned<size_t>(0xFFF0, 16) == true);

  INFO("Support::alignUp()");
  EXPECT(Support::alignUp<size_t>(0xFFFF,  4) == 0x10000);
  EXPECT(Support::alignUp<size_t>(0xFFF4,  4) == 0x0FFF4);
  EXPECT(Support::alignUp<size_t>(0xFFF8,  8) == 0x0FFF8);
  EXPECT(Support::alignUp<size_t>(0xFFF0, 16) == 0x0FFF0);
  EXPECT(Support::alignUp<size_t>(0xFFF0, 32) == 0x10000);

  INFO("Support::alignUpDiff()");
  EXPECT(Support::alignUpDiff<size_t>(0xFFFF,  4) == 1);
  EXPECT(Support::alignUpDiff<size_t>(0xFFF4,  4) == 0);
  EXPECT(Support::alignUpDiff<size_t>(0xFFF8,  8) == 0);
  EXPECT(Support::alignUpDiff<size_t>(0xFFF0, 16) == 0);
  EXPECT(Support::alignUpDiff<size_t>(0xFFF0, 32) == 16);

  INFO("Support::alignUpPowerOf2()");
  EXPECT(Support::alignUpPowerOf2<size_t>(0x0000) == 0x00000);
  EXPECT(Support::alignUpPowerOf2<size_t>(0xFFFF) == 0x10000);
  EXPECT(Support::alignUpPowerOf2<size_t>(0xF123) == 0x10000);
  EXPECT(Support::alignUpPowerOf2<size_t>(0x0F00) == 0x01000);
  EXPECT(Support::alignUpPowerOf2<size_t>(0x0100) == 0x00100);
  EXPECT(Support::alignUpPowerOf2<size_t>(0x1001) == 0x02000);
}

static void testBitUtils() noexcept {
  uint32_t i;

  INFO("Support::shl() / shr()");
  EXPECT(Support::shl<int32_t >(0x00001111 , 16) == 0x11110000 );
  EXPECT(Support::shl<uint32_t>(0x00001111 , 16) == 0x11110000 );
  EXPECT(Support::shr<int32_t >(0x11110000u, 16) == 0x00001111u);
  EXPECT(Support::shr<uint32_t>(0x11110000u, 16) == 0x00001111u);
  EXPECT(Support::sar<int32_t >(0xFFFF0000u, 16) == 0xFFFFFFFFu);

  INFO("Support::rol() / ror()");
  EXPECT(Support::rol<int32_t >(0x00100000 , 16) == 0x00000010 );
  EXPECT(Support::rol<uint32_t>(0x00100000u, 16) == 0x00000010u);
  EXPECT(Support::ror<int32_t >(0x00001000 , 16) == 0x10000000 );
  EXPECT(Support::ror<uint32_t>(0x00001000u, 16) == 0x10000000u);

  INFO("Support::isPowerOf2()");
  for (i = 0; i < 64; i++) {
    EXPECT(Support::isPowerOf2(uint64_t(1) << i) == true);
    EXPECT(Support::isPowerOf2((uint64_t(1) << i) ^ 0x001101) == false);
  }

  INFO("Support::isConsecutiveBitMask()");
  i = 0;
  for (;;) {
    bool result = Support::isConsecutiveBitMask(i);
    bool expect = testIsConsecutiveBitMask(i);
    EXPECT(result == expect);

    if (i == 0xFFFFu)
      break;
    i++;
  }
}

static void testIntUtils() noexcept {
  uint32_t i;

  INFO("Support::byteswap()");
  EXPECT(Support::byteswap16(int32_t(0x0102)) == int16_t(0x0201));
  EXPECT(Support::byteswap16(uint16_t(0x0102)) == uint16_t(0x0201));
  EXPECT(Support::byteswap24(int32_t(0x00010203)) == int32_t(0x00030201));
  EXPECT(Support::byteswap24(uint32_t(0x00010203)) == uint32_t(0x00030201));
  EXPECT(Support::byteswap32(int32_t(0x01020304)) == int32_t(0x04030201));
  EXPECT(Support::byteswap32(uint32_t(0x01020304)) == uint32_t(0x04030201));
  EXPECT(Support::byteswap64(uint64_t(0x0102030405060708)) == uint64_t(0x0807060504030201));

  INFO("Support::bytepack()");
  union BytePackData {
    uint8_t bytes[4];
    uint32_t u32;
  } bpdata;

  bpdata.u32 = Support::bytepack32_4x8(0x00, 0x11, 0x22, 0x33);
  EXPECT(bpdata.bytes[0] == 0x00);
  EXPECT(bpdata.bytes[1] == 0x11);
  EXPECT(bpdata.bytes[2] == 0x22);
  EXPECT(bpdata.bytes[3] == 0x33);

  INFO("Support::clampToByte()");
  EXPECT(Support::clampToByte(-1) == 0);
  EXPECT(Support::clampToByte(42) == 42);
  EXPECT(Support::clampToByte(255) == 0xFF);
  EXPECT(Support::clampToByte(256) == 0xFF);
  EXPECT(Support::clampToByte(0x7FFFFFFF) == 0xFF);
  EXPECT(Support::clampToByte(0x7FFFFFFFu) == 0xFF);
  EXPECT(Support::clampToByte(0xFFFFFFFFu) == 0xFF);

  INFO("Support::clampToWord()");
  EXPECT(Support::clampToWord(-1) == 0);
  EXPECT(Support::clampToWord(42) == 42);
  EXPECT(Support::clampToWord(0xFFFF) == 0xFFFF);
  EXPECT(Support::clampToWord(0x10000) == 0xFFFF);
  EXPECT(Support::clampToWord(0x10000u) == 0xFFFF);
  EXPECT(Support::clampToWord(0x7FFFFFFF) == 0xFFFF);
  EXPECT(Support::clampToWord(0x7FFFFFFFu) == 0xFFFF);
  EXPECT(Support::clampToWord(0xFFFFFFFFu) == 0xFFFF);

  INFO("Support::udiv255()");
  for (i = 0; i < 255 * 255; i++) {
    uint32_t result = Support::udiv255(i);
    uint32_t j = i + 128;

    // This version doesn't overflow 16 bits.
    uint32_t expected = (j + (j >> 8)) >> 8;

    EXPECT(result == expected, "b2d::Support::udiv255(%u) -> %u (Expected %u)", i, result, expected);
  }
}

static void testSafeUtils() noexcept {
  INFO("Support::safeAdd()");
  Support::OverflowFlag of;

  EXPECT(Support::safeAdd<int32_t>(0, 0, &of) == 0 && !of);
  EXPECT(Support::safeAdd<int32_t>(0, 1, &of) == 1 && !of);
  EXPECT(Support::safeAdd<int32_t>(1, 0, &of) == 1 && !of);

  EXPECT(Support::safeAdd<int32_t>(2147483647, 0, &of) == 2147483647 && !of);
  EXPECT(Support::safeAdd<int32_t>(0, 2147483647, &of) == 2147483647 && !of);
  EXPECT(Support::safeAdd<int32_t>(2147483647, -1, &of) == 2147483646 && !of);
  EXPECT(Support::safeAdd<int32_t>(-1, 2147483647, &of) == 2147483646 && !of);

  EXPECT(Support::safeAdd<int32_t>(-2147483647, 0, &of) == -2147483647 && !of);
  EXPECT(Support::safeAdd<int32_t>(0, -2147483647, &of) == -2147483647 && !of);
  EXPECT(Support::safeAdd<int32_t>(-2147483647, -1, &of) == -2147483647 - 1 && !of);
  EXPECT(Support::safeAdd<int32_t>(-1, -2147483647, &of) == -2147483647 - 1 && !of);

  Support::safeAdd<int32_t>(2147483647, 1, &of); EXPECT(of);
  Support::safeAdd<int32_t>(1, 2147483647, &of); EXPECT(of);
  Support::safeAdd<int32_t>(-2147483647, -2, &of); EXPECT(of);
  Support::safeAdd<int32_t>(-2, -2147483647, &of); EXPECT(of);

  EXPECT(Support::safeAdd<uint32_t>(0u, 0u, &of) == 0 && !of);
  EXPECT(Support::safeAdd<uint32_t>(0u, 1u, &of) == 1 && !of);
  EXPECT(Support::safeAdd<uint32_t>(1u, 0u, &of) == 1 && !of);

  EXPECT(Support::safeAdd<uint32_t>(2147483647u, 1u, &of) == 2147483648u && !of);
  EXPECT(Support::safeAdd<uint32_t>(1u, 2147483647u, &of) == 2147483648u && !of);
  EXPECT(Support::safeAdd<uint32_t>(0xFFFFFFFFu, 0u, &of) == 0xFFFFFFFFu && !of);
  EXPECT(Support::safeAdd<uint32_t>(0u, 0xFFFFFFFFu, &of) == 0xFFFFFFFFu && !of);

  Support::safeAdd<uint32_t>(0xFFFFFFFFu, 1u, &of); EXPECT(of);
  Support::safeAdd<uint32_t>(1u, 0xFFFFFFFFu, &of); EXPECT(of);

  Support::safeAdd<uint32_t>(0x80000000u, 0xFFFFFFFFu, &of); EXPECT(of);
  Support::safeAdd<uint32_t>(0xFFFFFFFFu, 0x80000000u, &of); EXPECT(of);
  Support::safeAdd<uint32_t>(0xFFFFFFFFu, 0xFFFFFFFFu, &of); EXPECT(of);

  INFO("Support::safeSub()");
  EXPECT(Support::safeSub<int32_t>(0, 0, &of) ==  0 && !of);
  EXPECT(Support::safeSub<int32_t>(0, 1, &of) == -1 && !of);
  EXPECT(Support::safeSub<int32_t>(1, 0, &of) ==  1 && !of);
  EXPECT(Support::safeSub<int32_t>(0, -1, &of) ==  1 && !of);
  EXPECT(Support::safeSub<int32_t>(-1, 0, &of) == -1 && !of);

  EXPECT(Support::safeSub<int32_t>(2147483647, 1, &of) == 2147483646 && !of);
  EXPECT(Support::safeSub<int32_t>(2147483647, 2147483647, &of) == 0 && !of);
  EXPECT(Support::safeSub<int32_t>(-2147483647, 1, &of) == -2147483647 - 1 && !of);
  EXPECT(Support::safeSub<int32_t>(-2147483647, -1, &of) == -2147483646 && !of);
  EXPECT(Support::safeSub<int32_t>(-2147483647 - 0, -2147483647 - 0, &of) == 0 && !of);
  EXPECT(Support::safeSub<int32_t>(-2147483647 - 1, -2147483647 - 1, &of) == 0 && !of);

  Support::safeSub<int32_t>(-2, 2147483647, &of); EXPECT(of);
  Support::safeSub<int32_t>(-2147483647, 2, &of); EXPECT(of);

  Support::safeSub<int32_t>(-2147483647    , 2147483647, &of); EXPECT(of);
  Support::safeSub<int32_t>(-2147483647 - 1, 2147483647, &of); EXPECT(of);

  Support::safeSub<int32_t>(2147483647, -2147483647 - 0, &of); EXPECT(of);
  Support::safeSub<int32_t>(2147483647, -2147483647 - 1, &of); EXPECT(of);

  EXPECT(Support::safeSub<uint32_t>(0, 0, &of) ==  0 && !of);
  EXPECT(Support::safeSub<uint32_t>(1, 0, &of) ==  1 && !of);

  EXPECT(Support::safeSub<uint32_t>(0xFFFFFFFFu, 0u, &of) == 0xFFFFFFFFu && !of);
  EXPECT(Support::safeSub<uint32_t>(0xFFFFFFFFu, 0xFFFFFFFFu, &of) == 0u && !of);

  Support::safeSub<uint32_t>(0u, 1u, &of); EXPECT(of);
  Support::safeSub<uint32_t>(1u, 2u, &of); EXPECT(of);

  Support::safeSub<uint32_t>(0u, 0xFFFFFFFFu, &of); EXPECT(of);
  Support::safeSub<uint32_t>(1u, 0xFFFFFFFFu, &of); EXPECT(of);

  Support::safeSub<uint32_t>(0u, 0x7FFFFFFFu, &of); EXPECT(of);
  Support::safeSub<uint32_t>(1u, 0x7FFFFFFFu, &of); EXPECT(of);

  Support::safeSub<uint32_t>(0x7FFFFFFEu, 0x7FFFFFFFu, &of); EXPECT(of);
  Support::safeSub<uint32_t>(0xFFFFFFFEu, 0xFFFFFFFFu, &of); EXPECT(of);

  INFO("Support::safeMul()");
  EXPECT(Support::safeMul<int32_t>(0, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<int32_t>(0, 1, &of) == 0 && !of);
  EXPECT(Support::safeMul<int32_t>(1, 0, &of) == 0 && !of);

  EXPECT(Support::safeMul<int32_t>( 1,  1, &of) ==  1 && !of);
  EXPECT(Support::safeMul<int32_t>( 1, -1, &of) == -1 && !of);
  EXPECT(Support::safeMul<int32_t>(-1,  1, &of) == -1 && !of);
  EXPECT(Support::safeMul<int32_t>(-1, -1, &of) ==  1 && !of);

  EXPECT(Support::safeMul<int32_t>( 32768,  65535, &of) ==  2147450880 && !of);
  EXPECT(Support::safeMul<int32_t>( 32768, -65535, &of) == -2147450880 && !of);
  EXPECT(Support::safeMul<int32_t>(-32768,  65535, &of) == -2147450880 && !of);
  EXPECT(Support::safeMul<int32_t>(-32768, -65535, &of) ==  2147450880 && !of);

  EXPECT(Support::safeMul<int32_t>(2147483647, 1, &of) == 2147483647 && !of);
  EXPECT(Support::safeMul<int32_t>(1, 2147483647, &of) == 2147483647 && !of);

  EXPECT(Support::safeMul<int32_t>(-2147483647 - 1, 1, &of) == -2147483647 - 1 && !of);
  EXPECT(Support::safeMul<int32_t>(1, -2147483647 - 1, &of) == -2147483647 - 1 && !of);

  Support::safeMul<int32_t>( 65535,  65535, &of); EXPECT(of);
  Support::safeMul<int32_t>( 65535, -65535, &of); EXPECT(of);
  Support::safeMul<int32_t>(-65535,  65535, &of); EXPECT(of);
  Support::safeMul<int32_t>(-65535, -65535, &of); EXPECT(of);

  Support::safeMul<int32_t>( 2147483647    ,  2147483647    , &of); EXPECT(of);
  Support::safeMul<int32_t>( 2147483647    , -2147483647 - 1, &of); EXPECT(of);
  Support::safeMul<int32_t>(-2147483647 - 1,  2147483647    , &of); EXPECT(of);
  Support::safeMul<int32_t>(-2147483647 - 1, -2147483647 - 1, &of); EXPECT(of);

  EXPECT(Support::safeMul<uint32_t>(0, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint32_t>(0, 1, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint32_t>(1, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint32_t>(1, 1, &of) == 1 && !of);

  EXPECT(Support::safeMul<uint32_t>(0x10000000u, 15, &of) == 0xF0000000u && !of);
  EXPECT(Support::safeMul<uint32_t>(15, 0x10000000u, &of) == 0xF0000000u && !of);

  EXPECT(Support::safeMul<uint32_t>(0xFFFFFFFFu, 1, &of) == 0xFFFFFFFFu && !of);
  EXPECT(Support::safeMul<uint32_t>(1, 0xFFFFFFFFu, &of) == 0xFFFFFFFFu && !of);

  Support::safeMul<uint32_t>(0xFFFFFFFFu, 2, &of); EXPECT(of);
  Support::safeMul<uint32_t>(2, 0xFFFFFFFFu, &of); EXPECT(of);

  Support::safeMul<uint32_t>(0x80000000u, 2, &of); EXPECT(of);
  Support::safeMul<uint32_t>(2, 0x80000000u, &of); EXPECT(of);

  EXPECT(Support::safeMul<int64_t>(0, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<int64_t>(0, 1, &of) == 0 && !of);
  EXPECT(Support::safeMul<int64_t>(1, 0, &of) == 0 && !of);

  EXPECT(Support::safeMul<int64_t>( 1,  1, &of) ==  1 && !of);
  EXPECT(Support::safeMul<int64_t>( 1, -1, &of) == -1 && !of);
  EXPECT(Support::safeMul<int64_t>(-1,  1, &of) == -1 && !of);
  EXPECT(Support::safeMul<int64_t>(-1, -1, &of) ==  1 && !of);

  EXPECT(Support::safeMul<int64_t>( 32768,  65535, &of) ==  2147450880 && !of);
  EXPECT(Support::safeMul<int64_t>( 32768, -65535, &of) == -2147450880 && !of);
  EXPECT(Support::safeMul<int64_t>(-32768,  65535, &of) == -2147450880 && !of);
  EXPECT(Support::safeMul<int64_t>(-32768, -65535, &of) ==  2147450880 && !of);

  EXPECT(Support::safeMul<int64_t>(2147483647, 1, &of) == 2147483647 && !of);
  EXPECT(Support::safeMul<int64_t>(1, 2147483647, &of) == 2147483647 && !of);

  EXPECT(Support::safeMul<int64_t>(-2147483647 - 1, 1, &of) == -2147483647 - 1 && !of);
  EXPECT(Support::safeMul<int64_t>(1, -2147483647 - 1, &of) == -2147483647 - 1 && !of);

  EXPECT(Support::safeMul<int64_t>( 65535,  65535, &of) ==  int64_t(4294836225) && !of);
  EXPECT(Support::safeMul<int64_t>( 65535, -65535, &of) == -int64_t(4294836225) && !of);
  EXPECT(Support::safeMul<int64_t>(-65535,  65535, &of) == -int64_t(4294836225) && !of);
  EXPECT(Support::safeMul<int64_t>(-65535, -65535, &of) ==  int64_t(4294836225) && !of);

  EXPECT(Support::safeMul<int64_t>( 2147483647    ,  2147483647    , &of) ==  int64_t(4611686014132420609) && !of);
  EXPECT(Support::safeMul<int64_t>( 2147483647    , -2147483647 - 1, &of) == -int64_t(4611686016279904256) && !of);
  EXPECT(Support::safeMul<int64_t>(-2147483647 - 1,  2147483647    , &of) == -int64_t(4611686016279904256) && !of);
  EXPECT(Support::safeMul<int64_t>(-2147483647 - 1, -2147483647 - 1, &of) ==  int64_t(4611686018427387904) && !of);

  EXPECT(Support::safeMul<int64_t>(int64_t(0x7FFFFFFFFFFFFFFF), int64_t(1), &of) == int64_t(0x7FFFFFFFFFFFFFFF) && !of);
  EXPECT(Support::safeMul<int64_t>(int64_t(1), int64_t(0x7FFFFFFFFFFFFFFF), &of) == int64_t(0x7FFFFFFFFFFFFFFF) && !of);

  Support::safeMul<int64_t>(int64_t(0x7FFFFFFFFFFFFFFF), int64_t(2), &of); EXPECT(of);
  Support::safeMul<int64_t>(int64_t(2), int64_t(0x7FFFFFFFFFFFFFFF), &of); EXPECT(of);

  Support::safeMul<int64_t>(int64_t( 0x7FFFFFFFFFFFFFFF), int64_t( 0x7FFFFFFFFFFFFFFF), &of); EXPECT(of);
  Support::safeMul<int64_t>(int64_t( 0x7FFFFFFFFFFFFFFF), int64_t(-0x7FFFFFFFFFFFFFFF), &of); EXPECT(of);
  Support::safeMul<int64_t>(int64_t(-0x7FFFFFFFFFFFFFFF), int64_t( 0x7FFFFFFFFFFFFFFF), &of); EXPECT(of);
  Support::safeMul<int64_t>(int64_t(-0x7FFFFFFFFFFFFFFF), int64_t(-0x7FFFFFFFFFFFFFFF), &of); EXPECT(of);

  EXPECT(Support::safeMul<uint64_t>(0, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint64_t>(0, 1, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint64_t>(1, 0, &of) == 0 && !of);
  EXPECT(Support::safeMul<uint64_t>(1, 1, &of) == 1 && !of);

  EXPECT(Support::safeMul<uint64_t>(0x1000000000000000u, 15, &of) == 0xF000000000000000u && !of);
  EXPECT(Support::safeMul<uint64_t>(15, 0x1000000000000000u, &of) == 0xF000000000000000u && !of);

  EXPECT(Support::safeMul<uint64_t>(0xFFFFFFFFFFFFFFFFu, 1, &of) == 0xFFFFFFFFFFFFFFFFu && !of);
  EXPECT(Support::safeMul<uint64_t>(1, 0xFFFFFFFFFFFFFFFFu, &of) == 0xFFFFFFFFFFFFFFFFu && !of);

  Support::safeMul<uint64_t>(0xFFFFFFFFFFFFFFFFu, 2, &of); EXPECT(of);
  Support::safeMul<uint64_t>(2, 0xFFFFFFFFFFFFFFFFu, &of); EXPECT(of);

  Support::safeMul<uint64_t>(0x8000000000000000u, 2, &of); EXPECT(of);
  Support::safeMul<uint64_t>(2, 0x8000000000000000u, &of); EXPECT(of);
}

static void testReadWrite() noexcept {
  INFO("Support::readX() / writeX()");
  {
    uint8_t arr[32] = { 0 };

    Support::writeU16uBE(arr + 1, 0x0102u);
    Support::writeU16uBE(arr + 3, 0x0304u);
    EXPECT(Support::readU32uBE(arr + 1) == 0x01020304u);
    EXPECT(Support::readU32uLE(arr + 1) == 0x04030201u);
    EXPECT(Support::readU32uBE(arr + 2) == 0x02030400u);
    EXPECT(Support::readU32uLE(arr + 2) == 0x00040302u);

    Support::writeU32uLE(arr + 5, 0x05060708u);
    EXPECT(Support::readU64uBE(arr + 1) == 0x0102030408070605u);
    EXPECT(Support::readU64uLE(arr + 1) == 0x0506070804030201u);

    Support::writeU64uLE(arr + 7, 0x1122334455667788u);
    EXPECT(Support::readU32uBE(arr + 8) == 0x77665544u);
  }
}

static void testSorting() noexcept {
  INFO("Support::qSort() - Testing qsort and isort of predefined arrays");
  {
    constexpr size_t kArraySize = 11;

    int ref_[kArraySize] = { -4, -2, -1, 0, 1, 9, 12, 13, 14, 19, 22 };
    int arr1[kArraySize] = { 0, 1, -1, 19, 22, 14, -4, 9, 12, 13, -2 };
    int arr2[kArraySize];

    std::memcpy(arr2, arr1, kArraySize * sizeof(int));

    Support::iSort(arr1, kArraySize);
    Support::qSort(arr2, kArraySize);
    testArrays(arr1, ref_, kArraySize);
    testArrays(arr2, ref_, kArraySize);
  }

  INFO("Support::qSort() - Testing qsort and isort of artificial arrays");
  {
    constexpr size_t kArraySize = 200;

    int arr1[kArraySize];
    int arr2[kArraySize];
    int ref_[kArraySize];

    for (size_t size = 2; size < kArraySize; size++) {
      for (size_t i = 0; i < size; i++) {
        arr1[i] = int(size - 1 - i);
        arr2[i] = int(size - 1 - i);
        ref_[i] = int(i);
      }

      Support::iSort(arr1, size);
      Support::qSort(arr2, size);
      testArrays(arr1, ref_, size);
      testArrays(arr2, ref_, size);
    }
  }

  INFO("Support::qSort() - Testing qsort and isort having unstable compare function");
  {
    constexpr size_t kArraySize = 5;

    float arr1[kArraySize] = { 1.0f, 0.0f, 3.0f, -1.0f, std::numeric_limits<float>::quiet_NaN() };
    float arr2[kArraySize] = { };

    std::memcpy(arr2, arr1, kArraySize * sizeof(float));

    // We don't test as it's undefined where the NaN would be.
    Support::iSort(arr1, kArraySize);
    Support::qSort(arr2, kArraySize);
  }
}

UNIT(b2d_core_support) {
  testAlignment();
  testBitUtils();
  testIntUtils();
  testSafeUtils();
  testReadWrite();
  testSorting();
}
#endif

B2D_END_NAMESPACE
