// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ARGB_H
#define _B2D_CORE_ARGB_H

// [Dependencies]
#include "../core/simd.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Argb32;
struct Argb64;

// ============================================================================
// [b2d::ArgbUtil]
// ============================================================================

namespace ArgbUtil {
  // --------------------------------------------------------------------------
  // [Argb32]
  // --------------------------------------------------------------------------

  constexpr uint32_t pack32(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept {
    return (a << 24) + (r << 16) + (g << 8) + b;
  }

  static B2D_INLINE void unpack32(uint32_t val32, uint32_t& a, uint32_t& r, uint32_t& g, uint32_t& b) noexcept {
    b = val32 & 0xFFFF;
    r = val32 >> 16;

    g = b >> 8; b &= 0xFF;
    a = r >> 8; r &= 0xFF;
  }

  static B2D_INLINE void unpackRgb32(uint32_t val32, uint32_t& r, uint32_t& g, uint32_t& b) noexcept {
    r = (val32 >> 16) & 0xFF;
    g = (val32 >>  8) & 0xFF;
    b = (val32      ) & 0xFF;
  }

  constexpr bool isOpaque32(uint32_t val32) noexcept {
    return val32 >= 0xFF000000u;
  }

  constexpr bool isTransparent32(uint32_t val32) noexcept {
    return val32 <= 0x00FFFFFFu;
  }

  // --------------------------------------------------------------------------
  // [Argb64]
  // --------------------------------------------------------------------------

  constexpr uint64_t pack64(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept {
    return (uint64_t((a << 16) + r) << 32) |
           (uint64_t((g << 16) + b)      ) ;
  }

  static B2D_INLINE void unpack64(uint64_t val64, uint32_t& a, uint32_t& r, uint32_t& g, uint32_t& b) noexcept {
    uint32_t lo = uint32_t(val64 >> 32);
    uint32_t hi = uint32_t(val64 & 0xFFFFFFFF);

    a = lo >> 16; r = lo & 0xFFFF;
    g = hi >> 16; b = hi & 0xFFFF;
  }

  #if B2D_ARCH_BITS >= 64
  static B2D_INLINE bool isOpaque64(uint64_t val64) noexcept { return val64 >= 0xFFFF000000000000u; }
  static B2D_INLINE bool isTransparent64(uint64_t val64) noexcept { return val64 <= 0x0000FFFFFFFFFFFFu; }
  #else
  static B2D_INLINE bool isOpaque64(uint64_t val64) noexcept { return uint32_t(val64 >> 32) >= 0xFFFF0000u; }
  static B2D_INLINE bool isTransparent64(uint64_t val64) noexcept { return uint32_t(val64 >> 32) <= 0x0000FFFFu; }
  #endif

  // --------------------------------------------------------------------------
  // [Convert]
  // --------------------------------------------------------------------------

  static B2D_INLINE uint32_t convert32From64(uint64_t val64) noexcept {
    #if B2D_ARCH_BITS >= 64 && B2D_ARCH_SSE2

    SIMD::I128 x = SIMD::vsrli16<8>(SIMD::vcvtu64i128(val64));
    return SIMD::vcvti128u32(SIMD::vpackzzwb(x));

    #elif B2D_ARCH_BITS >= 64

    uint64_t t0 = val64 >>  8;
    uint64_t t1 = val64 >> 16;

    t0 &= 0x000000FF000000FFu;               // [000R000B].
    t1 &= 0x0000FF000000FF00u;               // [00A000G0].

    t0 += t1;                                // [00AR00GB].
    t0 *= 0x00010001;                        // [ARARGBGB].
    return uint32_t((t0 >> 16) & 0xFFFFFFFFu);

    #else

    uint32_t hi = uint32_t(val64 >> 32);
    uint32_t lo = uint32_t(val64 & 0xFFFFFFFFu);

    return ((hi & 0xFF000000)      ) +
           ((lo & 0xFF000000) >> 16) +
           ((hi & 0x0000FF00) <<  8) +
           ((lo & 0x0000FF00) >>  8) ;

    #endif
  }

  static B2D_INLINE uint64_t convert64From32(uint32_t val32) noexcept {
    #if B2D_ARCH_BITS >= 64 && B2D_SIMD_I

    SIMD::I128 x = SIMD::vcvtu32i128(val32);
    return SIMD::vcvti128u64(SIMD::vunpackli8(x, x));

    #elif B2D_ARCH_BITS >= 64

    uint64_t val64;
    uint32_t gb = val32 << 8;                // [RGB0].
    uint32_t ar = val32 & 0xFF0000FF;        // [A00B].

    gb |= ar;                                // [XGBB].
    ar |= val32 >> 8;                        // [AARX].

    val64 = (uint64_t(ar) << 24) | uint64_t(gb);
    val64 &= 0x00FF00FF00FF00FFu;
    return val64 * 0x0101u;

    #else

    uint32_t t0 = (val32 & 0xFF);            // [000B].
    uint32_t t1 = (val32 << 8);              // [RGB0].
    uint32_t t2 = (val32 & 0xFF000000);      // [A000].

    t0 += t1;                                // [RGBB].
    t1 >>= 24;                               // [000R].
    t2 >>= 8;                                // [0A00].

    t0 &= 0x00FF00FF;                        // [0G0B].
    t1 += t2;                                // [0A0R].

    t0 *= 0x0101;                            // [GGBB].
    t1 *= 0x0101;                            // [AARR].
    return (uint64_t(t1) << 32) + uint64_t(t0);

    #endif
  }
};

// ============================================================================
// [b2d::Argb32]
// ============================================================================

//! 32-bit ARGB (8-bit per component).
struct Argb32 {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Argb32() noexcept = default;
  constexpr Argb32(const Argb32& other) noexcept = default;
  constexpr explicit Argb32(uint32_t val32) noexcept : _value(val32) {}

  constexpr Argb32(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept :
    _value(ArgbUtil::pack32(a, r, g, b)) {}

  static B2D_INLINE Argb32 from64(const Argb64& val64) noexcept;
  static B2D_INLINE Argb32 from64(uint64_t val64) noexcept { return Argb32(ArgbUtil::convert32From64(val64)); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept { _value = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get packed ARGB32 value.
  constexpr uint32_t value() const noexcept { return _value; }

  //! Get alpha component.
  constexpr uint32_t a() const noexcept { return (_value >> 24); }
  //! Get red component.
  constexpr uint32_t r() const noexcept { return (_value >> 16) & 0xFF; }
  //! Get green component.
  constexpr uint32_t g() const noexcept { return (_value >> 8) & 0xFF; }
  //! Get blue component.
  constexpr uint32_t b() const noexcept { return (_value     ) & 0xFF; }

  //! Set packed ARGB32 value.
  inline Argb32& setValue(const Argb32& val32) noexcept {
    _value = val32._value;
    return *this;
  }

  //! Set packed ARGB32 value.
  inline Argb32& setValue(uint32_t val32) noexcept {
    _value = val32;
    return *this;
  }

  //! Set packed ARGB32 value.
  inline Argb32& setValue(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept {
    _value = ArgbUtil::pack32(a, r, g, b);
    return *this;
  }

  //! Set alpha.
  inline Argb32& setA(uint32_t a) noexcept {
    B2D_ASSERT(a <= 0xFF);

    _value = (_value & 0x00FFFFFFu) + (a << 24);
    return *this;
  }

  //! Set red.
  inline Argb32& setR(uint32_t r) noexcept {
    B2D_ASSERT(r <= 0xFF);

    _value = (_value & 0xFF00FFFFu) + (r << 16);
    return *this;
  }

  //! Set green.
  inline Argb32& setG(uint32_t g) noexcept {
    B2D_ASSERT(g <= 0xFF);

    _value = (_value & 0xFFFF00FFu) + (g << 8);
    return *this;
  }

  //! Set blue.
  inline Argb32& setB(uint32_t b) noexcept {
    B2D_ASSERT(b <= 0xFF);

    _value = (_value & 0xFFFFFF00u) + b;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [IsOpaque / IsTransparent]
  // --------------------------------------------------------------------------

  //! Get whether the color is fully-opaque (255 in decimal).
  inline bool isOpaque() const noexcept { return ArgbUtil::isOpaque32(_value); }
  //! Get whether the color is fully-transparent (0 in decimal).
  inline bool isTransparent() const noexcept { return ArgbUtil::isTransparent32(_value); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline Argb32& operator=(const Argb32& val32) noexcept = default;
  inline Argb32& operator=(uint32_t val32) noexcept { _value = val32; return *this; }

  constexpr bool operator==(const Argb32& val32) const noexcept { return _value == val32._value; }
  constexpr bool operator!=(const Argb32& val32) const noexcept { return _value == val32._value; }

  constexpr bool operator==(uint32_t val32) const noexcept { return _value == val32; }
  constexpr bool operator!=(uint32_t val32) const noexcept { return _value == val32; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Packed ARGB32 value.
  uint32_t _value;
};
static_assert(sizeof(Argb32) == 4, "b2d::Argb32 size must be 4 exactly bytes");

// ============================================================================
// [b2d::Argb64]
// ============================================================================

//! 64-bit ARGB (16-bit per component).
struct Argb64 {
  enum Index : uint32_t {
    kIndexA = B2D_ARCH_LE ? 3 : 0,
    kIndexR = B2D_ARCH_LE ? 2 : 1,
    kIndexG = B2D_ARCH_LE ? 1 : 2,
    kIndexB = B2D_ARCH_LE ? 0 : 3
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Argb64() noexcept = default;
  constexpr Argb64(const Argb64& argb64) noexcept = default;
  constexpr explicit Argb64(uint64_t val64) noexcept : _value(val64) {}

  inline explicit Argb64(const Argb32& argb32) noexcept :
    _value(ArgbUtil::convert64From32(argb32._value)) {}

  constexpr Argb64(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept :
    _value(ArgbUtil::pack64(a, r, g, b)) {}

  static B2D_INLINE Argb64 from32(const Argb32& val32) noexcept {
    return Argb64(ArgbUtil::convert64From32(val32._value));
  }

  static B2D_INLINE Argb64 from32(uint32_t val32) noexcept {
    return Argb64(ArgbUtil::convert64From32(val32));
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline Argb64& reset() noexcept {
    _value = 0;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get packed ARGB64 value.
  inline uint64_t value() const noexcept { return _value; }

  //! Set packed ARGB64 value.
  inline Argb64& setValue(const Argb64& val64) noexcept {
    _value = val64._value;
    return *this;
  }

  //! Set packed ARGB64 value.
  inline Argb64& setValue(uint64_t val64) noexcept {
    _value = val64;
    return *this;
  }

  //! Set packed ARGB64 value.
  inline Argb64& setValue(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept {
    _value = ArgbUtil::pack64(a, r, g, b);
    return *this;
  }

  //! Get low part of packed 64-bit ARGB value.
  inline uint32_t lo() const noexcept { return _lo; }
  //! Get high part of packed 64-bit ARGB value.
  inline uint32_t hi() const noexcept { return _hi; }

  //! Set low part of packed 64-bit ARGB value.
  inline Argb64& setLo(uint32_t lo) noexcept { _lo = lo; return *this; }
  //! Set high part of packed 64-bit ARGB value.
  inline Argb64& setHi(uint32_t hi) noexcept { _hi = hi; return *this; }

  //! Get alpha.
  inline uint32_t a() const noexcept { return _val16[kIndexA]; }
  //! Get red.
  inline uint32_t r() const noexcept { return _val16[kIndexR]; }
  //! Get green.
  inline uint32_t g() const noexcept { return _val16[kIndexG]; }
  //! Get blue.
  inline uint32_t b() const noexcept { return _val16[kIndexB]; }

  inline Argb64& _replaceComponent(uint32_t shift, uint32_t value) noexcept {
    B2D_ASSERT(shift <= 48);
    B2D_ASSERT(value <= 0xFFFFu);

    #if B2D_ARCH_BITS >= 64
      _value = (_value & ~(uint64_t(0xFFFFu) << shift)) | (value << shift);
    #else
      if (shift >= 32) {
        shift -= 32;
        _hi = (_hi & ~uint32_t(0xFFFF << shift)) | (value << shift);
      }
      else {
        _lo = (_lo & ~uint32_t(0xFFFF << shift)) | (value << shift);
      }
    #endif
    return *this;
  }

  //! Set alpha component.
  inline Argb64& setA(uint32_t value) noexcept { return _replaceComponent(48, value); }
  //! Set red component.
  inline Argb64& setR(uint32_t value) noexcept { return _replaceComponent(32, value); }
  //! Set green component.
  inline Argb64& setG(uint32_t value) noexcept { return _replaceComponent(16, value); }
  //! Set blue component.
  inline Argb64& setB(uint32_t value) noexcept { return _replaceComponent(0, value); }

  // --------------------------------------------------------------------------
  // [IsFullyOpaque / IsTransparent]
  // --------------------------------------------------------------------------

  //! Get whether the alpha is fully-opaque (255 in decimal).
  constexpr bool isFullyOpaque() const noexcept { return B2D_ARCH_BITS >= 64 ? _value >= 0xFFFF000000000000u : _hi >= 0xFFFF0000u; }
  //! Get whether the alpha is fully-transparent (0 in decimal).
  constexpr bool isTransparent() const noexcept { return B2D_ARCH_BITS >= 64 ? _value <= 0x0000FFFFFFFFFFFFu : _hi <= 0x0000FFFFu; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline Argb64& operator=(const Argb64& val64) noexcept = default;
  inline Argb64& operator=(const Argb32& val32) noexcept { _value = ArgbUtil::convert64From32(val32._value); return *this; }

  constexpr bool operator==(const Argb64& val64) const noexcept { return _value == val64._value; }
  constexpr bool operator!=(const Argb64& val64) const noexcept { return _value == val64._value; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint64_t _value;                     //! ARGB64 value.
    uint32_t _val32[2];                  //! Packed low and high 32-bit values.
    uint16_t _val16[4];                  //! Packed components.

    struct {
      #if B2D_ARCH_LE
      uint16_t _b, _g, _r, _a;
      #else
      uint16_t _a, _r, _g, _b;
      #endif
    };

    struct {
      #if B2D_ARCH_LE
      uint32_t _lo;
      uint32_t _hi;
      #else
      uint32_t _hi;
      uint32_t _lo;
      #endif
    };
  };
};
static_assert(sizeof(Argb64) == 8, "b2d::Argb64 size must be exactly 8 bytes");

// ============================================================================
// [Implemented-Later]
// ============================================================================

inline Argb32 Argb32::from64(const Argb64& val64) noexcept {
  return Argb32(ArgbUtil::convert32From64(val64._value));
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ARGB_H
