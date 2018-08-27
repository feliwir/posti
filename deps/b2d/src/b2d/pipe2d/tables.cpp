// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./tables_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::ModTable]
// ============================================================================

const ModTable gModTable[18] = {
  #define INV()  {{ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0   , 0   , 0   , 0   , 0   , 0   , 0    }}
  #define DEF(N) {{ 1%N, 2%N, 3%N, 4%N, 5%N, 6%N, 7%N, 8%N, 9%N, 10%N, 11%N, 12%N, 13%N, 14%N, 15%N, 16%N }}
  INV(),
  DEF(1),
  DEF(2),
  DEF(3),
  DEF(4),
  DEF(5),
  DEF(6),
  DEF(7),
  DEF(8),
  DEF(9),
  DEF(10),
  DEF(11),
  DEF(12),
  DEF(13),
  DEF(14),
  DEF(15),
  DEF(16),
  DEF(17)
  #undef DEF
  #undef INV
};

// ============================================================================
// [b2d::Pipe2D::Constants]
// ============================================================================

#define REPEAT_1X(...) __VA_ARGS__
#define REPEAT_2X(...) __VA_ARGS__, __VA_ARGS__
#define REPEAT_4X(...) REPEAT_2X(REPEAT_2X(__VA_ARGS__))
#define REPEAT_8X(...) REPEAT_4X(REPEAT_2X(__VA_ARGS__))

#define FLOAT_4X(A, B, C, D) float(A), float(B), float(C), float(D)

const Constants gConstants = {
  // --------------------------------------------------------------------------
  // [XMM Constants]
  // --------------------------------------------------------------------------

  { REPEAT_8X(0x0000u) },                              // xmm_zeros.
  { REPEAT_8X(0xFFFFu) },                              // xmm_ones.

  { REPEAT_8X(0x007Fu) },                              // xmm_u16_007F.
  { REPEAT_8X(0x0080u) },                              // xmm_u16_0080.
  { REPEAT_8X(0x00FFu) },                              // xmm_u16_00FF.
  { REPEAT_8X(0x0100u) },                              // xmm_u16_0100.
  { REPEAT_8X(0x0101u) },                              // xmm_u16_0101.
  { REPEAT_8X(0x01FFu) },                              // xmm_u16_01FF.
  { REPEAT_8X(0x0200u) },                              // xmm_u16_0200.
  { REPEAT_8X(0x8000u) },                              // xmm_u16_8000.

  { REPEAT_2X(0, 0, 0, 0xFFu) },                       // xmm_u16_0_0_0_FF.

  { REPEAT_4X(511) },                                  // xmm_u32_511.

  { REPEAT_4X(0x000000FFu) },                          // xmm_u32_000000FF.
  { REPEAT_4X(0x00000100u) },                          // xmm_u32_00000100.
  { REPEAT_4X(0x0000FFFFu) },                          // xmm_u32_0000FFFF.
  { REPEAT_4X(0x00020000u) },                          // xmm_u32_00020000 (256 << 9).
  { REPEAT_4X(0x00FFFFFFu) },                          // xmm_u32_00FFFFFF.
  { REPEAT_4X(0xFF000000u) },                          // xmm_u32_FF000000.
  { REPEAT_4X(0xFFFF0000u) },                          // xmm_u32_FFFF0000.

  { REPEAT_2X(0xFFFFFFFFu, 0) },                       // xmm_u32_0_FFFFFFFF.
  { REPEAT_2X(0, 0xFFFFFFFFu) },                       // xmm_u32_FFFFFFFF_0.

  { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0 },        // xmm_u32_FFFFFFFF_FFFFFFFF_FFFFFFFF_0.

  { 0, 1, 2, 3 },                                      // xmm_u32_0_1_2_3.
  { REPEAT_4X(4) },                                    // xmm_u32_4.

  { REPEAT_2X(0x000000FF00FF00FFu) },                  // xmm_u64_000000FF00FF00FF.
  { REPEAT_2X(0x0000010001000100u) },                  // xmm_u64_0000010001000100.
  { REPEAT_2X(0x0000080000000800u) },                  // xmm_u64_0000080000000800.
  { REPEAT_2X(0x0000FFFFFFFFFFFFu) },                  // xmm_u64_0000FFFFFFFFFFFF.
  { REPEAT_2X(0x00FF000000000000u) },                  // xmm_u64_00FF000000000000.
  { REPEAT_2X(0x0100000000000000u) },                  // xmm_u64_0100000000000000.
  { REPEAT_2X(0x0101010100000000u) },                  // xmm_u64_0101010100000000.
  { REPEAT_2X(0xFFFF000000000000u) },                  // xmm_u64_FFFF000000000000.

  { REPEAT_2X(0x80000000, 0x80000000) },               // xmm_f_sgn.
  { REPEAT_2X(0x7FFFFFFF, 0x7FFFFFFF) },               // xmm_f_abs.
  { REPEAT_2X(0x7FFFFFFF, 0xFFFFFFFF) },               // xmm_f_abs_lo.
  { REPEAT_2X(0xFFFFFFFF, 0x7FFFFFFF) },               // xmm_f_abs_hi.

  { REPEAT_1X(0x8000000000000000u, 0x8000000000000000u) }, // xmm_d_sgn.
  { REPEAT_1X(0x7FFFFFFFFFFFFFFFu, 0x7FFFFFFFFFFFFFFFu) }, // xmm_d_abs.
  { REPEAT_1X(0x7FFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu) }, // xmm_d_abs_lo.
  { REPEAT_1X(0xFFFFFFFFFFFFFFFFu, 0x7FFFFFFFFFFFFFFFu) }, // xmm_d_abs_hi.

  { REPEAT_4X(1.0f)              }, // xmm_f_1.
  { REPEAT_4X(1.0f / 255.0f)     }, // xmm_f_1_div_255.
  { REPEAT_4X(255.0f)            }, // xmm_f_255.
  { REPEAT_4X(1e-3f)             }, // xmm_f_1e_m3.
  { REPEAT_4X(1e-20f)            }, // xmm_f_1e_m20.
  { REPEAT_4X(4.0f)              }, // xmm_f_4.
  { 0.0f  , 1.0f  , 2.0f  , 3.0f }, // xmm_f_0123.

  { 1e-20, 1e-20                 }, // xmm_d_1e_m20.
  { 4.0   , 4.0                  }, // xmm_d_4.
  {-1.0   ,-1.0                  }, // xmm_d_m1.

  #define Z 0x80 // PSHUFB zeroing.
  { 0 , 4 , 8 , 12, 0 , 4 , 8 , 12, 0 , 4 , 8 , 12, 0 , 4 , 8 , 12 }, // xmm_pshufb_u32_to_u8_lo
  { 0 , 1 , 4 , 5 , 8 , 9 , 12, 13, 0 , 1 , 4 , 5 , 8 , 9 , 12, 13 }, // xmm_pshufb_u32_to_u16_lo
  { 3 , Z , 3 , Z , 3 , Z , 3 , Z , 7 , Z , 7 , Z , 7 , Z , 7 , Z  }, // xmm_pshufb_packed_argb32_2x_lo_to_unpacked_a8
  { 11, Z , 11, Z , 11, Z , 11, Z , 15, Z , 15, Z , 15, Z , 15, Z  }, // xmm_pshufb_packed_argb32_2x_lo_to_unpacked_a8
  #undef Z

  // --------------------------------------------------------------------------
  // [XMM Gradients]
  // --------------------------------------------------------------------------

  // CONICAL GRADIENT:
  //
  // Polynomial to approximate `atan(x) * N / 2PI`:
  //   `x * (Q0 + x^2 * (Q1 + x^2 * (Q2 + x^2 * Q3)))`
  //
  // The following numbers were obtained by `lolremez` - minmax tool for approx.:
  //
  // Atan is an odd function, so we take advantage of it (see lolremez docs):
  //   1. E=|atan(x) * N / 2PI - P(x)                  | <- subs. `P(x)` by `x*Q(x^2))`
  //   2. E=|atan(x) * N / 2PI - x*Q(x^2)              | <- subs. `x^2` by `y`
  //   3. E=|atan(sqrt(y)) * N / 2PI - sqrt(y) * Q(y)  | <- eliminate `y` from Q side - div by `y`
  //   4. E=|atan(sqrt(y)) * N / (2PI * sqrt(y)) - Q(y)|
  //
  // LolRemez C++ code:
  //   real f(real const& x) {
  //     real y = sqrt(x);
  //     return atan(y) * real(N) / (real(2) * real::R_PI * y);
  //   }
  //   real g(real const& x) {
  //     return re(sqrt(x));
  //   }
  //   int main(int argc, char **argv) {
  //     RemezSolver<3, real> solver;
  //     solver.Run("1e-1000", 1, f, g, 40);
  //     return 0;
  //   }
  {
    #define REC(N, Q0, Q1, Q2, Q3) {    \
      { FLOAT_4X(N  , N  , N  , N  ) }, \
      { FLOAT_4X(N/2, N/2, N/2, N/2) }, \
      { FLOAT_4X(N/4, N/4, N/4, N/4) }, \
      { FLOAT_4X(N/2, N  , N/2, N  ) }, \
      { FLOAT_4X(Q0 , Q0 , Q0 , Q0 ) }, \
      { FLOAT_4X(Q1 , Q1 , Q1 , Q1 ) }, \
      { FLOAT_4X(Q2 , Q2 , Q2 , Q2 ) }, \
      { FLOAT_4X(Q3 , Q3 , Q3 , Q3 ) }  \
    }
    REC(256 , 4.071421038552e+1, -1.311160794048e+1, 6.017670215625   , -1.623253505085   ),
    REC(512 , 8.142842077104e+1, -2.622321588095e+1, 1.203534043125e+1, -3.246507010170   ),
    REC(1024, 1.628568415421e+2, -5.244643176191e+1, 2.407068086250e+1, -6.493014020340   ),
    REC(2048, 3.257136830841e+2, -1.048928635238e+2, 4.814136172500e+1, -1.298602804068e+1),
    REC(4096, 6.514273661683e+2, -2.097857270476e+2, 9.628272344999e+1, -2.597205608136e+1)
    #undef REC
  },

  // --------------------------------------------------------------------------
  // [Div24Bit]
  // --------------------------------------------------------------------------

  {
    0x00000000, 0x00FF0000, 0x007F8000, 0x00550000, 0x003FC000, 0x00330000, 0x002A8000, 0x00246DB7,
    0x001FE000, 0x001C5556, 0x00198000, 0x00172E8C, 0x00154000, 0x00139D8A, 0x001236DC, 0x00110000,
    0x000FF000, 0x000F0000, 0x000E2AAB, 0x000D6BCB, 0x000CC000, 0x000C2493, 0x000B9746, 0x000B1643,
    0x000AA000, 0x000A3334, 0x0009CEC5, 0x000971C8, 0x00091B6E, 0x0008CB09, 0x00088000, 0x000839CF,
    0x0007F800, 0x0007BA2F, 0x00078000, 0x00074925, 0x00071556, 0x0006E454, 0x0006B5E6, 0x000689D9,
    0x00066000, 0x00063832, 0x0006124A, 0x0005EE24, 0x0005CBA3, 0x0005AAAB, 0x00058B22, 0x00056CF0,
    0x00055000, 0x0005343F, 0x0005199A, 0x00050000, 0x0004E763, 0x0004CFB3, 0x0004B8E4, 0x0004A2E9,
    0x00048DB7, 0x00047944, 0x00046585, 0x00045271, 0x00044000, 0x00042E2A, 0x00041CE8, 0x00040C31,
    0x0003FC00, 0x0003EC4F, 0x0003DD18, 0x0003CE55, 0x0003C000, 0x0003B217, 0x0003A493, 0x00039770,
    0x00038AAB, 0x00037E40, 0x0003722A, 0x00036667, 0x00035AF3, 0x00034FCB, 0x000344ED, 0x00033A55,
    0x00033000, 0x000325EE, 0x00031C19, 0x00031282, 0x00030925, 0x00030000, 0x0002F712, 0x0002EE59,
    0x0002E5D2, 0x0002DD7C, 0x0002D556, 0x0002CD5D, 0x0002C591, 0x0002BDF0, 0x0002B678, 0x0002AF29,
    0x0002A800, 0x0002A0FE, 0x00029A20, 0x00029365, 0x00028CCD, 0x00028657, 0x00028000, 0x000279CA,
    0x000273B2, 0x00026DB7, 0x000267DA, 0x00026218, 0x00025C72, 0x000256E7, 0x00025175, 0x00024C1C,
    0x000246DC, 0x000241B3, 0x00023CA2, 0x000237A7, 0x000232C3, 0x00022DF3, 0x00022939, 0x00022493,
    0x00022000, 0x00021B82, 0x00021715, 0x000212BC, 0x00020E74, 0x00020A3E, 0x00020619, 0x00020205,
    0x0001FE00, 0x0001FA0C, 0x0001F628, 0x0001F253, 0x0001EE8C, 0x0001EAD4, 0x0001E72B, 0x0001E38F,
    0x0001E000, 0x0001DC80, 0x0001D90C, 0x0001D5A4, 0x0001D24A, 0x0001CEFB, 0x0001CBB8, 0x0001C881,
    0x0001C556, 0x0001C235, 0x0001BF20, 0x0001BC15, 0x0001B915, 0x0001B61F, 0x0001B334, 0x0001B052,
    0x0001AD7A, 0x0001AAAB, 0x0001A7E6, 0x0001A52A, 0x0001A277, 0x00019FCC, 0x00019D2B, 0x00019A91,
    0x00019800, 0x00019578, 0x000192F7, 0x0001907E, 0x00018E0D, 0x00018BA3, 0x00018941, 0x000186E6,
    0x00018493, 0x00018246, 0x00018000, 0x00017DC2, 0x00017B89, 0x00017958, 0x0001772D, 0x00017508,
    0x000172E9, 0x000170D1, 0x00016EBE, 0x00016CB2, 0x00016AAB, 0x000168AA, 0x000166AF, 0x000164B9,
    0x000162C9, 0x000160DE, 0x00015EF8, 0x00015D18, 0x00015B3C, 0x00015966, 0x00015795, 0x000155C8,
    0x00015400, 0x0001523E, 0x0001507F, 0x00014EC5, 0x00014D10, 0x00014B5F, 0x000149B3, 0x0001480B,
    0x00014667, 0x000144C7, 0x0001432C, 0x00014194, 0x00014000, 0x00013E71, 0x00013CE5, 0x00013B5D,
    0x000139D9, 0x00013859, 0x000136DC, 0x00013563, 0x000133ED, 0x0001327B, 0x0001310C, 0x00012FA1,
    0x00012E39, 0x00012CD5, 0x00012B74, 0x00012A16, 0x000128BB, 0x00012763, 0x0001260E, 0x000124BD,
    0x0001236E, 0x00012223, 0x000120DA, 0x00011F94, 0x00011E51, 0x00011D11, 0x00011BD4, 0x00011A99,
    0x00011962, 0x0001182C, 0x000116FA, 0x000115CA, 0x0001149D, 0x00011372, 0x0001124A, 0x00011124,
    0x00011000, 0x00010EE0, 0x00010DC1, 0x00010CA5, 0x00010B8B, 0x00010A73, 0x0001095E, 0x0001084B,
    0x0001073A, 0x0001062C, 0x0001051F, 0x00010415, 0x0001030D, 0x00010207, 0x00010103, 0x00010000
  }
};

B2D_END_SUB_NAMESPACE
