// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_TABLES_P_H
#define _B2D_PIPE2D_TABLES_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::ModTable]
// ============================================================================

//! Table that contains precomputed `{1..16} % N`.
struct alignas(16) ModTable {
  uint8_t x1_16[16];
};
extern const ModTable gModTable[18];

// ============================================================================
// [b2d::Pipe2D::Constants]
// ============================================================================

//! Pipeline LUT (lookup tables).
//!
//! To save registers we use one lookup table that is suitable for all generated
//! pipelines. This means this table contains everything, from simple constants
//! used by some SIMD instructions to constants used by function approximations.
struct alignas(32) Constants {
  enum TableId {
    kTable256   = 0,
    kTable512   = 1,
    kTable1024  = 2,
    kTable2048  = 3,
    kTable4096  = 4,
    kTableCount = 5
  };

  // --------------------------------------------------------------------------
  // [XMM Commons]
  // --------------------------------------------------------------------------

  uint16_t xmm_zeros[8];                 //!< All zeros (xmm).
  uint16_t xmm_ones[8];                  //!< All ones (xmm).

  uint16_t xmm_u16_007F[8];              //!< uint16_t(128) (xmm).
  uint16_t xmm_u16_0080[8];              //!< uint16_t(128) (xmm).
  uint16_t xmm_u16_00FF[8];              //!< uint16_t(255) (xmm).
  uint16_t xmm_u16_0100[8];              //!< uint16_t(256) (xmm).
  uint16_t xmm_u16_0101[8];              //!< uint16_t(257) (xmm).
  uint16_t xmm_u16_01FF[8];              //!< uint16_t(511) (xmm).
  uint16_t xmm_u16_0200[8];              //!< uint16_t(512) (xmm).
  uint16_t xmm_u16_8000[8];              //!< uint16_t(0x8000) (xmm).
  uint16_t xmm_u16_0_0_0_FF[8];          //!< uint16_t({0, 0, 0, 0xFF})

  uint32_t xmm_u32_511[4];               //!< uint32_t(511) (xmm).
  uint32_t xmm_u32_000000FF[4];          //!< uint32_t(0x000000FF) (xmm).
  uint32_t xmm_u32_00000100[4];          //!< uint32_t(0x00000100) (xmm).
  uint32_t xmm_u32_0000FFFF[4];          //!< uint32_t(0x0000FFFF) (xmm).
  uint32_t xmm_u32_00020000[4];          //!< uint32_t(0x00020000) (xmm).
  uint32_t xmm_u32_00FFFFFF[4];          //!< uint32_t(0x00FFFFFF) (xmm).
  uint32_t xmm_u32_FF000000[4];          //!< uint32_t(0xFF000000) (xmm).
  uint32_t xmm_u32_FFFF0000[4];          //!< uint32_t(0xFFFF0000) (xmm).

  uint32_t xmm_u32_0_FFFFFFFF[4];        //!< uint32_t(0), uint32_t(0xFFFFFFFF) (xmm).
  uint32_t xmm_u32_FFFFFFFF_0[4];        //!< uint32_t(0xFFFFFFFF), uint32_t(0) (xmm).

  //! uint32_t(0), uint32_t(0xFFFFFFFF), uint32_t(0xFFFFFFFF), uint32_t(0xFFFFFFFF) (xmm).
  uint32_t xmm_u32_FFFFFFFF_FFFFFFFF_FFFFFFFF_0[4];

  uint32_t xmm_u32_0_1_2_3[4];           //!< uint32_t(0), uint32_t(1), uint32_t(2), uint32_t(3) (xmm).
  uint32_t xmm_u32_4[4];                 //!< uint32_t(4) (xmm).

  uint64_t xmm_u64_000000FF00FF00FF[2];  //!< uint64_t(0x000000FF00FF00FF) (xmm).
  uint64_t xmm_u64_0000010001000100[2];  //!< uint64_t(0x0000010001000100) (xmm).
  uint64_t xmm_u64_0000080000000800[2];  //!< uint64_t(0x0000080000000800) (xmm).
  uint64_t xmm_u64_0000FFFFFFFFFFFF[2];  //!< uint64_t(0x0000FFFFFFFFFFFF) (xmm).
  uint64_t xmm_u64_00FF000000000000[2];  //!< uint64_t(0x00FF000000000000) (xmm).
  uint64_t xmm_u64_0100000000000000[2];  //!< uint64_t(0x0100000000000000) (xmm).
  uint64_t xmm_u64_0101010100000000[2];  //!< uint64_t(0x0101010100000000) (xmm).
  uint64_t xmm_u64_FFFF000000000000[2];  //!< uint64_t(0xFFFF000000000000) (xmm).

  uint32_t xmm_f_sgn[4];                 //!< Mask of all `float` bits containing a sign (xmm).
  uint32_t xmm_f_abs[4];                 //!< Mask of all `float` bits without a sign (xmm).
  uint32_t xmm_f_abs_lo[4];              //!< Mask of all LO `float` bits without a sign (xmm).
  uint32_t xmm_f_abs_hi[4];              //!< Mask of all HI `float` bits without a sign (xmm).

  uint64_t xmm_d_sgn[2];                 //!< Mask of all `double` bits containing a sign (xmm).
  uint64_t xmm_d_abs[2];                 //!< Mask of all `double` bits without a sign (xmm).
  uint64_t xmm_d_abs_lo[2];              //!< Mask of LO `double` bits without a sign (xmm).
  uint64_t xmm_d_abs_hi[2];              //!< Mask of HI `double` bits without a sign (xmm).

  float xmm_f_1[4];                      //!< Vector of `1.0f` values (xmm).
  float xmm_f_1_div_255[4];              //!< Vector of `1.0f / 255.0f` values (xmm).
  float xmm_f_255[4];                    //!< Vector of `255.0f` values (xmm).

  float xmm_f_1e_m3[4];                  //!< Vector of `1e-3` values (xmm).
  float xmm_f_1e_m20[4];                 //!< Vector of `1e-20` values (xmm).
  float xmm_f_4[4];                      //!< Vector of `4.0f` values (xmm).
  float xmm_f_0123[4];                   //!< Vector containing `[0f, 1f, 2f, 3f]`.

  double xmm_d_1e_m20[2];                //!< Vector of `1e-20` values (xmm).
  double xmm_d_4[2];                     //!< Vector of `4.0` values (xmm).
  double xmm_d_m1[2];                    //!< Vector of `-1.0` values (xmm).

  uint8_t xmm_pshufb_u32_to_u8_lo[16];   //!< PSHUFB predicate for performing u32 to u8 shuffle (no saturation).
  uint8_t xmm_pshufb_u32_to_u16_lo[16];  //!< PSHUFB predicate for performing u32 to u16 shuffle (no saturation).
  uint8_t xmm_pshufb_packed_argb32_2x_lo_to_unpacked_a8[16];  //!< PSHUFB predicate for unpacking ARGB32 into A8 components (LO 2 pixels).
  uint8_t xmm_pshufb_packed_argb32_2x_hi_to_unpacked_a8[16];  //!< PSHUFB predicate for unpacking ARGB32 into A8 components (HI 2 pixels).

  // --------------------------------------------------------------------------
  // [XMM Conical]
  // --------------------------------------------------------------------------

  struct Conical {
    float n_div_1[4];
    float n_div_2[4];
    float n_div_4[4];
    float n_extra[4];

    // Polynomial to approximate `atan(x) * N / 2PI`:
    //   `x * (Q0 + x*x * (Q1 + x*x * (Q2 + x*x * Q3)))`
    // Where:
    //   `x >= 0 && x <= 1`
    float q0[4];
    float q1[4];
    float q2[4];
    float q3[4];
  } xmm_f_con[kTableCount];

  // --------------------------------------------------------------------------
  // [Div24Bit]
  // --------------------------------------------------------------------------

  //! Table, which can be used to turn integer division into multiplication and
  //! shift. It supports division by 0 (multiplies by zero) up to 255 using 24
  //! bits of precision. The multiplied product has to be shifted to the right
  //! by 16 bits to receive the final result.
  //!
  //! Usage:
  //!   `if (b) ? (a * 255) / b : 0` can be rewritten to `(a * div24bit[b]) >> 16`.
  uint32_t div24bit[256];
};
extern const Constants gConstants;

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_TABLES_P_H
