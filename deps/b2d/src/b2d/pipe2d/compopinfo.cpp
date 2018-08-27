// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./compopinfo_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Prepare]
// ============================================================================

// Defines how source and destination (backdrop) pixels mix to produce the
// desired result. The compositing and blending functions use the following
// variables:
//
//   - Sca  - Source color, premultiplied: `Sc * Sa`.
//   - Sc   - Source color.
//   - Sa   - Source alpha.
//
//   - Dca  - Destination color, premultiplied: `Dc * Da`.
//   - Dc   - Destination color.
//   - Da   - Destination alpha.
//
//   - Dca' - Resulting color (premultiplied).
//   - Da'  - Resulting alpha.
//
//   - m    - Mask (if used).
//
// Blending function F(Sc, Dc) is used in the following way if destination
// or source contains alpha channel (otherwise it's assumed to be `1.0`):
//
//  - Dca' = Func(Sc, Dc) * Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
//  - Da'  = Da + Sa.(1 - Da)
template<int FROM_OP, int FROM_DST, int FROM_SRC>
struct CompOpConvT {
  enum : uint32_t {
    kToOp  = FROM_OP,
    kToDst = FROM_DST,
    kToSrc = FROM_SRC
  };
};

#define B2D_DEFINE_COMP_OP_CONV(FROM_OP, FROM_DST, FROM_SRC, TO_OP, TO_DST, TO_SRC)  \
template<>                                                                    \
struct CompOpConvT<FROM_OP, PixelFormat::FROM_DST, PixelFormat::FROM_SRC> {   \
  enum : uint32_t  {                                                          \
    kToOp  = TO_OP,                                                           \
    kToDst = PixelFormat::TO_DST,                                             \
    kToSrc = PixelFormat::TO_SRC                                              \
  };                                                                          \
}

#define FROM(OP) CompOp::k##OP
#define TO(OP) CompOp::k##OP

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Src]
// ============================================================================

// [Src PRGBxPRGB]
//   Dca' = Sca                            Dca' = Sca.m + Dca.(1 - m)
//   Da'  = Sa                             Da'  = Sa .m + Da .(1 - m)
//
// [Src PRGBxXRGB] ~= [Src PRGBxPRGB]
//   Dca' = Sc                             Dca' = Sc.m + Dca.(1 - m)
//   Da'  = 1                              Da'  = 1 .m + Da .(1 - m)
//
// [Src XRGBxPRGB] ~= [Src PRGBxPRGB]
//   Dc'  = Sca                            Dc'  = Sca.m + Dc.(1 - m)
//
// [Src XRGBxXRGB] ~= [Src PRGBxPRGB]
//   Dc'  = Sc                             Dc'  = Sc.m + Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(Src), kPRGB32, kXRGB32, TO(Src), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Src), kXRGB32, kPRGB32, TO(Src), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Src), kXRGB32, kXRGB32, TO(Src), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - SrcOver]
// ============================================================================

// [SrcOver PRGBxPRGB]
//   Dca' = Sca + Dca.(1 - Sa)             Dca' = Sca.m + Dca.(1 - Sa.m)
//   Da'  = Sa  + Da .(1 - Sa)             Da'  = Sa .m + Da .(1 - Sa.m)
//
// [SrcOver PRGBxXRGB] ~= [Src PRGBxPRGB]
//   Dca' = Sc                             Dca' = Sc.m + Dca.(1 - m)
//   Da'  = 1                              Da'  = 1 .m + Da .(1 - m)
//
// [SrcOver XRGBxPRGB] ~= [SrcOver PRGBxPRGB]
//   Dc'  = Sca   + Dc.(1 - Sa  )          Dc'  = Sca.m + Dc.(1 - Sa.m)
//
// [SrcOver XRGBxXRGB] ~= [Src PRGBxPRGB]
//   Dc'  = Sc                             Dc'  = Sc.m + Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOver), kPRGB32, kXRGB32, TO(Src    ), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOver), kXRGB32, kPRGB32, TO(SrcOver), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOver), kXRGB32, kXRGB32, TO(Src    ), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - SrcIn]
// ============================================================================

// [SrcIn PRGBxPRGB]
//   Dca' = Sca.Da                         Dca' = Sca.Da.m + Dca.(1 - m)
//   Da'  = Sa .Da                         Da'  = Sa .Da.m + Da .(1 - m)
//
// [SrcIn PRGBxXRGB] ~= [SrcIn PRGBxPRGB]
//   Dca' = Sc.Da                          Dca' = Sc.Da.m + Dca.(1 - m)
//   Da'  = 1 .Da                          Da'  = 1 .Da.m + Da .(1 - m)
//
// [SrcIn XRGBxPRGB]
//   Dc'  = Sca                            Dc'  = Sca.m + Dc.(1 - m)
//
// [SrcIn XRGBxXRGB] ~= [SrcIn XRGBxPRGB]
//   Dca' = Sc                             Dca' = Sc.m + Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(SrcIn), kPRGB32, kXRGB32, TO(SrcIn), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcIn), kXRGB32, kXRGB32, TO(SrcIn), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - SrcOut]
// ============================================================================

// [SrcOut PRGBxPRGB]
//   Dca' = Sca.(1 - Da)                   Dca' = Sca.m.(1 - Da) + Dca.(1 - m)
//   Da'  = Sa .(1 - Da)                   Da'  = Sa .m.(1 - Da) + Da .(1 - m)
//
// [SrcOut PRGBxXRGB] ~= [SrcOut PRGBxPRGB]
//   Dca' = Sc.(1 - Da)                    Dca' = Sc.m.(1 - Da) + Dca.(1 - m)
//   Da'  = 1 .(1 - Da)                    Da'  = 1 .m.(1 - Da) + Da .(1 - m)
//
// [SrcOut XRGBxPRGB] ~= [Clear XRGBxPRGB]
//   Dc'  = 0                              Dc'  = Dc.(1 - m)
//
// [SrcOut XRGBxXRGB] ~= [Clear XRGBxPRGB]
//   Dc'  = 0                              Dc'  = Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOut), kPRGB32, kXRGB32, TO(SrcOut), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOut), kXRGB32, kPRGB32, TO(Clear ), kXRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcOut), kXRGB32, kXRGB32, TO(Clear ), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - SrcAtop]
// ============================================================================

// [SrcAtop PRGBxPRGB]
//   Dca' = Sca.Da + Dca.(1 - Sa)          Dca' = Sca.Da.m + Dca.(1 - Sa.m)
//   Da'  = Sa .Da + Da .(1 - Sa) = Da     Da'  = Sa .Da.m + Da .(1 - Sa.m) = Da
//
// [SrcAtop PRGBxXRGB] ~= [SrcIn PRGBxPRGB]
//   Dca' = Sc.Da                          Dca' = Sc.Da.m + Dca.(1 - m)
//   Da'  = 1 .Da                          Da'  = 1 .Da.m + Da .(1 - m)
//
// [SrcAtop XRGBxPRGB] ~= [SrcOver PRGBxPRGB]
//   Dc'  = Sca + Dc.(1 - Sa)              Dc'  = Sca.m + Dc.(1 - Sa.m)
//
// [SrcAtop XRGBxXRGB] ~= [Src PRGBxPRGB]
//   Dc'  = Sc                             Dc'  = Sc.m + Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(SrcAtop), kPRGB32, kXRGB32, TO(SrcIn  ), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcAtop), kXRGB32, kPRGB32, TO(SrcOver), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(SrcAtop), kXRGB32, kXRGB32, TO(Src    ), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Dst]
// ============================================================================

// [Dst ANYxANY]
//   Dca' = Dca
//   Da   = Da

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - DstOver]
// ============================================================================

// [DstOver PRGBxPRGB]
//   Dca' = Dca + Sca.(1 - Da)             Dca' = Dca + Sca.m.(1 - Da)
//   Da'  = Da  + Sa .(1 - Da)             Da'  = Da  + Sa .m.(1 - Da)
//
// [DstOver PRGBxXRGB] ~= [DstOver PRGBxPRGB]
//   Dca' = Dca + Sc.(1 - Da)              Dca' = Dca + Sc.m.(1 - Da)
//   Da'  = Da  + 1 .(1 - Da)              Da'  = Da  + 1 .m.(1 - Da)
//
// [DstOver XRGBxPRGB] ~= [Dst]
//   Dc'  = Dc
//
// [DstOver XRGBxXRGB] ~= [Dst]
//   Dc'  = Dc
B2D_DEFINE_COMP_OP_CONV(FROM(DstOver), kPRGB32, kXRGB32, TO(DstOver), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstOver), kXRGB32, kPRGB32, TO(Dst    ), kXRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstOver), kXRGB32, kXRGB32, TO(Dst    ), kXRGB32, kXRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - DstIn]
// ============================================================================

// [DstIn PRGBxPRGB]
//   Dca' = Dca.Sa                         Dca' = Dca.Sa.m + Dca.(1 - m)
//   Da'  = Da .Sa                         Da'  = Da .Sa.m + Da .(1 - m)
//
// [DstIn PRGBxXRGB] ~= [Dst]
//   Dca' = Dca
//   Da'  = Da
//
// [DstIn XRGBxPRGB]
//   Dc'  = Dc.Sa                          Dc'  = Dc.Sa.m + Dc.(1 - m)
//
// [DstIn XRGBxXRGB] ~= [Dst]
//   Dc'  = Dc
B2D_DEFINE_COMP_OP_CONV(FROM(DstIn), kPRGB32, kXRGB32, TO(Dst), kPRGB32, kXRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstIn), kXRGB32, kXRGB32, TO(Dst), kXRGB32, kXRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - DstOut]
// ============================================================================

// [DstOut PRGBxPRGB]
//   Dca' = Dca.(1 - Sa)                   Dca' = Dca.(1 - Sa.m)
//   Da'  = Da .(1 - Sa)                   Da'  = Da .(1 - Sa.m)
//
// [DstOut PRGBxXRGB] ~= [Clear PRGBxPRGB]
//   Dca' = 0
//   Da'  = 0
//
// [DstOut XRGBxPRGB]
//   Dc'  = Dc.(1 - Sa)                    Dc'  = Dc.(1 - Sa.m)
//
// [DstOut XRGBxXRGB] ~= [Clear XRGBxPRGB]
//   Dc'  = 0
B2D_DEFINE_COMP_OP_CONV(FROM(DstOut), kPRGB32, kXRGB32, TO(Clear), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstOut), kXRGB32, kXRGB32, TO(Clear), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - DstAtop]
// ============================================================================

// [DstAtop PRGBxPRGB]
//   Dca' = Dca.Sa + Sca.(1 - Da)          Dca' = Dca.(1 - m.(1 - Sa)) + Sca.m.(1 - Da)
//   Da'  = Da .Sa + Sa .(1 - Da) = Sa     Da'  = Da .(1 - m.(1 - Sa)) + Sa .m.(1 - Da)
//
// [DstAtop PRGBxXRGB] ~= [DstOver PRGBxPRGB]
//   Dca' = Dca + Sc.(1 - Da)              Dca' = Dca + Sc.m.(1 - Da)
//   Da'  = Da  + 1 .(1 - Da) = 1          Da'  = Da  + 1 .m.(1 - Da)
//
// [DstAtop XRGBxPRGB] ~= [DstIn XRGBxPRGB]
//   Dc'  = Dc.Sa                          Dc'  = Dc.(1 - m.(1 - Sa)) = Dc.(1 - m) + Dc.Sa.m
//
// [DstAtop XRGBxXRGB] ~= [Dst]
//   Dc'  = Dc
B2D_DEFINE_COMP_OP_CONV(FROM(DstAtop), kPRGB32, kXRGB32, TO(DstOver), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstAtop), kXRGB32, kPRGB32, TO(DstIn  ), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(DstAtop), kXRGB32, kXRGB32, TO(Dst    ), kXRGB32, kXRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Xor]
// ============================================================================

// [Xor PRGBxPRGB]
//   Dca' = Dca.(1 - Sa) + Sca.(1 - Da)    Dca' = Dca.(1 - Sa.m) + Sca.m.(1 - Da)
//   Da'  = Da .(1 - Sa) + Sa .(1 - Da)    Da'  = Da .(1 - Sa.m) + Sa .m.(1 - Da)
//
// [Xor PRGBxXRGB] ~= [SrcOut PRGBxPRGB]
//   Dca' = Sca.(1 - Da)                   Dca' = Sca.m.(1 - Da) + Dca.(1 - m)
//   Da'  = 1  .(1 - Da)                   Da'  = 1  .m.(1 - Da) + Da .(1 - m)
//
// [Xor XRGBxPRGB] ~= [DstOut XRGBxPRGB]
//   Dc'  = Dc.(1 - Sa)                    Dc'  = Dc.(1 - Sa.m)
//
// [Xor XRGBxXRGB] ~= [Clear XRGBxPRGB]
//   Dc'  = 0                              Dc'  = Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(Xor), kPRGB32, kXRGB32, TO(SrcOut), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Xor), kXRGB32, kPRGB32, TO(DstOut), kXRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Xor), kXRGB32, kXRGB32, TO(Clear ), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Clear]
// ============================================================================

// [Clear PRGBxPRGB]
//   Dca' = 0                              Dca' = Dca.(1 - m)
//   Da'  = 0                              Da'  = Da .(1 - m)
//
// [Clear XRGBxPRGB]
//   Dc'  = 0                              Dc'  = Dca.(1 - m)
//
// [Clear PRGBxXRGB] ~= [Clear PRGBxPRGB]
// [Clear XRGBxXRGB] ~= [Clear XRGBxPRGB]
B2D_DEFINE_COMP_OP_CONV(FROM(Clear), kPRGB32, kXRGB32, TO(Clear), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Clear), kXRGB32, kXRGB32, TO(Clear), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Plus]
// ============================================================================

// [Plus PRGBxPRGB]
//   Dca' = Clamp(Dca + Sca)               Dca' = Clamp(Dca + Sca).m + Dca.(1 - m)
//   Da'  = Clamp(Da  + Sa )               Da'  = Clamp(Da  + Sa ).m + Da .(1 - m)
//
// [Plus PRGBxXRGB] ~= [Plus PRGBxPRGB]
//   Dca' = Clamp(Dca + Sc)                Dca' = Clamp(Dca + Sc).m + Dca.(1 - m)
//   Da'  = Clamp(Da  + 1 )                Da'  = Clamp(Da  + 1 ).m + Da .(1 - m)
//
// [Plus XRGBxPRGB] ~= [Plus PRGBxPRGB]
//   Dc'  = Clamp(Dc + Sca)                Dc'  = Clamp(Dc + Sca).m + Dca.(1 - m)
//
// [Plus XRGBxXRGB] ~= [Plus PRGBxPRGB]
//   Dc'  = Clamp(Dc + Sc)                 Dc'  = Clamp(Dc + Sc).m + Dc.(1 - m)
B2D_DEFINE_COMP_OP_CONV(FROM(Plus), kPRGB32, kXRGB32, TO(Plus), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Plus), kXRGB32, kPRGB32, TO(Plus), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Plus), kXRGB32, kXRGB32, TO(Plus), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - PlusAlt]
// ============================================================================

// [PlusAlt PRGBxPRGB]
//   Dca' = Clamp(Dca + Sca)               Dca' = Clamp(Dca + Sca.m)
//   Da'  = Clamp(Da  + Sa )               Da'  = Clamp(Da  + Sa .m)
//
// [PlusAlt PRGBxXRGB] ~= [PlusAlt PRGBxPRGB]
//   Dca' = Clamp(Dca + Sc)                Dca' = Clamp(Dca + Sc.m)
//   Da'  = Clamp(Da  + 1 )                Da'  = Clamp(Da  + 1 .m)
//
// [PlusAlt XRGBxPRGB] ~= [PlusAlt PRGBxPRGB]
//   Dc'  = Clamp(Dc + Sca)                Dc'  = Clamp(Dc + Sca.m)
//
// [PlusAlt XRGBxXRGB] ~= [PlusAlt PRGBxPRGB]
//   Dc'  = Clamp(Dc + Sc)                 Dc'  = Clamp(Dc + Sc.m)
B2D_DEFINE_COMP_OP_CONV(FROM(PlusAlt), kPRGB32, kXRGB32, TO(PlusAlt), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(PlusAlt), kXRGB32, kPRGB32, TO(PlusAlt), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(PlusAlt), kXRGB32, kXRGB32, TO(PlusAlt), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Minus]
// ============================================================================

// [Minus PRGBxPRGB]
//   Dca' = Clamp(Dca - Sca)               Dca' = Clamp(Dca - Sca).m + Dca.(1 - m)
//   Da'  = Da + Sa.(1 - Da)               Da'  = Da + Sa.m(1 - Da)
//
// [Minus PRGBxXRGB] ~= [Minus PRGBxPRGB]
//   Dca' = Clamp(Dca - Sc)                Dca' = Clamp(Dca - Sc).m + Dca.(1 - m)
//   Da'  = Da + 1.(1 - Da) = 1            Da'  = Da + 1.m(1 - Da)
//
// [Minus XRGBxPRGB]
//   Dc'  = Clamp(Dc - Sca)                Dc'  = Clamp(Dc - Sca).m + Dc.(1 - m)
//
// [Minus XRGBxXRGB] ~= [Minus XRGBxPRGB]
//   Dc'  = Clamp(Dc - Sc)                 Dc'  = Clamp(Dc - Sc).m + Dc.(1 - m)
//
// NOTE:
//   `Clamp(a - b)` == `Max(a - b, 0)` == `1 - Min(1 - a + b, 1)`
B2D_DEFINE_COMP_OP_CONV(FROM(Minus), kPRGB32, kXRGB32, TO(Minus), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Minus), kXRGB32, kXRGB32, TO(Minus), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Multiply]
// ============================================================================

// [Multiply PRGBxPRGB]
//   Dca' = Dca.(Sca + 1 - Sa) + Sca.(1 - Da)
//   Da'  = Da .(Sa  + 1 - Sa) + Sa .(1 - Da) = Da + Sa.(1 - Da)
//
//   Dca' = Dca.(Sca.m + 1 - Sa.m) + Sca.m(1 - Da)
//   Da'  = Da .(Sa .m + 1 - Sa.m) + Sa .m(1 - Da) = Da + Sa.m(1 - Da)
//
// [Multiply PRGBxXRGB]
//   Dca' = Sc.(Dca + 1 - Da)
//   Da'  = 1 .(Da  + 1 - Da) = 1
//
//   Dca' = Dca.(Sc.m + 1 - 1.m) + Sc.m(1 - Da)
//   Da'  = Da .(1 .m + 1 - 1.m) + 1 .m(1 - Da) = Da + Sa.m(1 - Da)
//
// [Multiply XRGBxPRGB]
//   Dc'  = Dc.(Sca   + 1 - Sa  )
//   Dc'  = Dc.(Sca.m + 1 - Sa.m)
//
// [Multiply XRGBxXRGB]
//   Dc'  = Dc.(Sc   + 1 - 1  )
//   Dc'  = Dc.(Sc.m + 1 - 1.m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Screen]
// ============================================================================

// [Screen PRGBxPRGB]
//   Dca' = Dca + Sca.(1 - Dca)
//   Da'  = Da  + Sa .(1 - Da )
//
//   Dca' = Dca + Sca.m.(1 - Dca)
//   Da'  = Da  + Sa .m.(1 - Da )
//
// [Screen PRGBxXRGB] ~= [Screen PRGBxPRGB]
//   Dca' = Dca + Sc.(1 - Dca)
//   Da'  = Da  + 1 .(1 - Da )
//
//   Dca' = Dca + Sc.m.(1 - Dca)
//   Da'  = Da  + 1 .m.(1 - Da )
//
// [Screen XRGBxPRGB] ~= [Screen PRGBxPRGB]
//   Dc'  = Dc + Sca  .(1 - Dca)
//   Dc'  = Dc + Sca.m.(1 - Dca)
//
// [Screen PRGBxPRGB] ~= [Screen PRGBxPRGB]
//   Dc'  = Dc + Sc  .(1 - Dc)
//   Dc'  = Dc + Sc.m.(1 - Dc)
B2D_DEFINE_COMP_OP_CONV(FROM(Screen), kPRGB32, kXRGB32, TO(Screen), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Screen), kXRGB32, kPRGB32, TO(Screen), kPRGB32, kPRGB32);
B2D_DEFINE_COMP_OP_CONV(FROM(Screen), kXRGB32, kXRGB32, TO(Screen), kPRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Overlay]
// ============================================================================

// [Overlay PRGBxPRGB]
//   if (2.Dca < Da)
//     Dca' = Dca + Sca - (Dca.Sa + Sca.Da - 2.Sca.Dca)
//     Da'  = Da  + Sa  - Sa.Da
//   else
//     Dca' = Dca + Sca + (Dca.Sa + Sca.Da - 2.Sca.Dca) - Sa.Da
//     Da'  = Da  + Sa  - Sa.Da
//
// [Overlay PRGBxXRGB]
//   if (2.Dca - Da < 0)
//     Dca' = Sc.(2.Dca - Da + 1)
//     Da'  = 1
//   else
//     Dca' = 2.Dca - Da - Sc.(1 - (2.Dca - Da))
//     Da'  = 1
//
// [Overlay XRGBxPRGB]
//   if (2.Dca < Da)
//     Dc'  = Dc - (Dc.Sa - 2.Sca.Dc)
//   else
//     Dc'  = Dc + 2.Sca - Sa + (Dca.Sa - 2.Sca.Dc)
//
// [Overlay XRGBxXRGB]
//   if (2.Dc - 1 < 0)
//     Dc'  = 2.Dc.Sc
//   else
//     Dc'  = 2.(Dc + Sc) - 2.Sc.Dc - 1

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Darken]
// ============================================================================

// [Darken PRGBxPRGB]
//   Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
//   Da'  = min(Sa .Da, Da .Sa) + Sa .(1 - Da) + Da .(1 - Sa)
//        = Sa + Da - Sa.Da
//
//   Dca' = min(Sca.m.Da, Dca.Sa.m) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//   Da'  = min(Sa .m.Da, Da .Sa.m) + Sa .m.(1 - Da) + Da .(1 - Sa.m)
//        = Sa.m + Da - Sa.m.Da
//
// [Darken PRGBxXRGB]
//   Dca' = min(Sc.Da, Dca) + Sc.(1 - Da)
//   Da'  = min(1 .Da, Da ) + 1 .(1 - Da)
//        = Sa + Da - Sa.Da
//
//   Dca' = min(Sc.m.Da, Dca.m) + Sc.m.(1 - Da) + Dca.(1 - 1.m)
//   Da'  = min(1 .m.Da, Da .m) + 1 .m.(1 - Da) + Da .(1 - 1.m)
//        = 1.m + Da - 1.m.Da
//
// [Darken XRGBxPRGB]
//   Dc'  = min(Sca  , Dc.Sa  ) + Dc.(1 - Sa  )
//   Dc'  = min(Sca.m, Dc.Sa.m) + Dc.(1 - Sa.m)
//
// [Darken XRGBxXRGB]
//   Dc'  = min(Sc, Dc)
//   Dc'  = min(Sc, Dc).m + Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Lighten]
// ============================================================================

// [Lighten PRGBxPRGB]
//   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
//   Da'  = max(Sa .Da, Da .Sa) + Sa .(1 - Da) + Da .(1 - Sa)
//        = Sa + Da - Sa.Da
//
//   Dca' = max(Sca.m.Da, Dca.Sa.m) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//   Da'  = max(Sa .m.Da, Da .Sa.m) + Sa .m.(1 - Da) + Da .(1 - Sa.m)
//        = Sa.m + Da - Sa.m.Da
//
// [Lighten PRGBxXRGB]
//   Dca' = max(Sc.Da, Dca) + Sc.(1 - Da)
//   Da'  = max(1 .Da, Da ) + 1 .(1 - Da)
//        = Sa + Da - Sa.Da
//
//   Dca' = max(Sc.m.Da, Dca.m) + Sc.m.(1 - Da) + Dca.(1 - 1.m)
//   Da'  = max(1 .m.Da, Da .m) + 1 .m.(1 - Da) + Da .(1 - 1.m)
//        = 1.m + Da - 1.m.Da
//
// [Lighten XRGBxPRGB]
//   Dc'  = max(Sca  , Dc.Sa  ) + Dc.(1 - Sa  )
//   Dc'  = max(Sca.m, Dc.Sa.m) + Dc.(1 - Sa.m)
//
// [Lighten XRGBxXRGB]
//   Dc'  = max(Sc, Dc)
//   Dc'  = max(Sc, Dc).m + Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - ColorDodge]
// ============================================================================

// [ColorDodge PRGBxPRGB]
//   Dca' = min(Dca.Sa.Sa / max(Sa - Sca, 0.001), Da.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
//   Da'  = Sa + Da - Sa.Da
//
//   Dca' = min(Dca.Sa.m.Sa.m / max(Sa.m - Sca.m, 0.001), Da.Sa.m) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//   Da'  = Sa.m + Da - Sa.m.Da
//
// [ColorDodge PRGBxXRGB]
//   Dca' = min(Dca / max(1 - Sc, 0.001), Da) + Sc.(1 - Da)
//   Da'  = 1
//
//   Dca' = min(Dca.1.m.1.m / max(1.m - Sc.m, 0.001), Da.1.m) + Sc.m.(1 - Da) + Dca.(1 - 1.m)
//   Da'  = 1.m + Da - 1.m.Da
//
// [ColorDodge XRGBxPRGB]
//   Dc'  = min(Dc.Sa  .Sa   / max(Sa   - Sca  , 0.001), Sa)   + Dc.(1 - Sa)
//   Dc'  = min(Dc.Sa.m.Sa.m / max(Sa.m - Sca.m, 0.001), Sa.m) + Dc.(1 - Sa.m)
//
// [ColorDodge XRGBxXRGB]
//   Dc'  = min(Dc / max(1 - Sc, 0.001), 1)
//   Dc'  = min(Dc / max(1 - Sc, 0.001), 1).m + Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - ColorBurn]
// ============================================================================

// [ColorBurn PRGBxPRGB]
//   Dca' = Sa.Da - min(Sa.Da, (Da - Dca).Sa.Sa / max(Sca, 0.001)) + Sca.(1 - Da) + Dca.(1 - Sa)
//   Da'  = Sa + Da - Sa.Da
//
//   Dca' = Sa.m.Da - min(Sa.m.Da, (Da - Dca).Sa.m.Sa.m / max(Sca.m, 0.001)) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//   Da'  = Sa.m + Da - Sa.m.Da
//
// [ColorBurn PRGBxXRGB]
//   Dca' = 1.Da - min(Da, (Da - Dca) / max(Sc, 0.001)) + Sc.(1 - Da)
//   Da'  = 1
//
//   Dca' = m.Da - min(1.m.Da, (Da - Dca).1.m.1.m / max(Sc.m, 0.001)) + Sc.m.(1 - Da) + Dca.(1 - 1.m)
//   Da'  = 1.m + Da - 1.m.Da
//
// [ColorBurn XRGBxPRGB]
//   Dc'  = Sa   - min(Sa  , (1 - Dc).Sa  .Sa   / max(Sca  , 0.001)) + Dc.(1 - Sa)
//   Dc'  = Sa.m - min(Sa.m, (1 - Dc).Sa.m.Sa.m / max(Sca.m, 0.001)) + Dc.(1 - Sa.m)
//
// [ColorBurn XRGBxXRGB]
//   Dc'  = (1 - min(1, (1 - Dc) / max(Sc, 0.001)))
//   Dc'  = (1 - min(1, (1 - Dc) / max(Sc, 0.001))).m + Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - LinearBurn]
// ============================================================================

// [LinearBurn PRGBxPRGB]
//   Dca' = Clamp(Dca + Sca - Sa.Da)
//   Da'  = Da + Sa - Sa.Da
//
//   Dca' = Clamp(Dca + Sca - Sa.Da).m + Dca.(1 - m)
//   Da'  = Sa.m.(1 - Da) + Da
//
// [LinearBurn PRGBxXRGB]
//   Dca' = Clamp(Dca + Sc - Da)
//   Da'  = 1
//
//   Dca' = Clamp(Dca + Sc - Da).m + Dca.(1 - m)
//   Da'  = Da + Sa - Sa.Da
//
// [LinearBurn XRGBxPRGB]
//   Dc'  = Clamp(Dc + Sca - Sa)
//   Dc'  = Clamp(Dc + Sca - Sa).m + Dc.(1 - m)
//
// [LinearBurn XRGBxXRGB]
//   Dc'  = Clamp(Dc + Sc - 1)
//   Dc'  = Clamp(Dc + Sc - 1).m + Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - LinearLight]
// ============================================================================

// [LinearLight PRGBxPRGB]
//   Dca' = min(max((Dca.Sa + 2.Sca.Da - Sa.Da), 0), Sa.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
//   Da'  = Da + Sa - Sa.Da
//
//   Dca' = min(max((Dca.Sa.m + 2.Sca.m.Da - Sa.m.Da), 0), Sa.m.Da) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//   Da'  = Da + Sa.m - Sa.m.Da
//
// [LinearLight PRGBxXRGB]
//   Dca' = min(max((Dca + 2.Sc.Da - Da), 0), Da) + Sc.(1 - Da)
//   Da'  = 1
//
//   Dca' = min(max((Dca.1.m + 2.Sc.m.Da - 1.m.Da), 0), 1.m.Da) + Sc.m.(1 - Da) + Dca.(1 - m)
//   Da'  = Da + Sa.m - Sa.m.Da
//
// [LinearLight XRGBxPRGB]
//   Dca' = min(max((Dc.Sa   + 2.Sca   - Sa  ), 0), Sa  ) + Dca.(1 - Sa)
//   Dca' = min(max((Dc.Sa.m + 2.Sca.m - Sa.m), 0), Sa.m) + Dca.(1 - Sa.m)
//
// [LinearLight XRGBxXRGB]
//   Dc'  = min(max((Dc + 2.Sc - 1), 0), 1)
//   Dc'  = min(max((Dc + 2.Sc - 1), 0), 1).m + Dca.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - PinLight]
// ============================================================================

// [PinLight PRGBxPRGB]
//   if 2.Sca <= Sa
//     Dca' = min(Dca.Sa, 2.Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
//     Da'  = Da + Sa.(1 - Da)
//   else
//     Dca' = max(Dca.Sa, 2.Sca.Da - Sa.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
//     Da'  = Da + Sa.(1 - Da)
//
//   if 2.Sca.m <= Sa.m
//     Dca' = min(Dca.Sa.m, 2.Sca.m.Da) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//     Da'  = Da + Sa.m.(1 - Da)
//   else
//     Dca' = max(Dca.Sa.m, 2.Sca.m.Da - Sa.m.Da) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//     Da'  = Da + Sa.m.(1 - Da)
//
// [PinLight PRGBxXRGB]
//   if 2.Sc <= 1
//     Dca' = min(Dca, 2.Sc.Da) + Sc.(1 - Da)
//     Da'  = 1
//   else
//     Dca' = max(Dca, 2.Sc.Da - Da) + Sc.(1 - Da)
//     Da'  = 1
//
//   if 2.Sc.m <= 1.m
//     Dca' = min(Dca.m, 2.Sc.m.Da) + Sc.m.(1 - Da) + Dca.(1 - m)
//     Da'  = Da + m.(1 - Da)
//   else
//     Dca' = max(Dca.m, 2.Sc.m.Da - m.Da) + Sc.m.(1 - Da) + Dc.(1 - m)
//     Da'  = Da + m.(1 - Da)
//
// [PinLight XRGBxPRGB]
//   if 2.Sca <= Sa
//     Dc'  = min(Dc.Sa, 2.Sca) + Dc.(1 - Sa)
//   else
//     Dc'  = max(Dc.Sa, 2.Sca - Sa) + Dc.(1 - Sa)
//
//   if 2.Sca.m <= Sa.m
//     Dc'  = min(Dc.Sa.m, 2.Sca.m) + Dc.(1 - Sa.m)
//   else
//     Dc'  = max(Dc.Sa.m, 2.Sca.m - Sa.m) + Dc.(1 - Sa.m)
//
// [PinLight XRGBxXRGB]
//   if 2.Sc <= 1
//     Dc'  = min(Dc, 2.Sc)
//   else
//     Dc'  = max(Dc, 2.Sc - 1)
//
//   if 2.Sca.m <= Sa.m
//     Dc'  = min(Dc, 2.Sc).m + Dca.(1 - m)
//   else
//     Dc'  = max(Dc, 2.Sc - 1).m + Dca.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - HardLight]
// ============================================================================

// [HardLight PRGBxPRGB]
//   if (2.Sca <= Sa)
//     Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
//     Da'  = Sa + Da - Sa.Da
//   else
//     Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
//     Da'  = Sa + Da - Sa.Da
//
//   if (2.Sca.m <= Sa.m)
//     Dca' = 2.Sca.m.Dca + Sca.m(1 - Da) + Dca.(1 - Sa.m)
//     Da'  = Sa.m + Da - Sa.m.Da
//   else
//     Dca' = Sa.m.Da - 2.(Da - Dca).(Sa.m - Sca.m) + Sca.m.(1 - Da) + Dca.(1 - Sa.m)
//     Da'  = Sa.m + Da - Sa.m.Da
//
// [HardLight PRGBxXRGB]
//   if (2.Sc <= 1)
//     Dca' = 2.Sc.Dca + Sc.(1 - Da)
//     Da'  = 1
//   else
//     Dca' = Da - 2.(Da - Dca).(1 - Sc) + Sc.(1 - Da)
//     Da'  = 1
//
//   if (2.Sc.m <= m)
//     Dca' = 2.Sc.m.Dca + Sc.m(1 - Da) + Dca.(1 - m)
//     Da'  = Da + m.(1 - Da)
//   else
//     Dca' = 1.m.Da - 2.(Da - Dca).((1 - Sc).m) + Sc.m.(1 - Da) + Dca.(1 - m)
//     Da'  = Da + m.(1 - Da)
//
// [HardLight XRGBxPRGB]
//   if (2.Sca <= Sa)
//     Dc'  = 2.Sca.Dc + Dc.(1 - Sa)
//   else
//     Dc'  = Sa - 2.(1 - Dc).(Sa - Sca) + Dc.(1 - Sa)
//
//   if (2.Sca.m <= Sa.m)
//     Dc'  = 2.Sca.m.Dc + Dc.(1 - Sa.m)
//   else
//     Dc'  = Sa.m - 2.(1 - Dc).(Sa.m - Sca.m) + Dc.(1 - Sa.m)
//
// [HardLight XRGBxXRGB]
//   if (2.Sc <= 1)
//     Dc'  = 2.Sc.Dc
//   else
//     Dc'  = 1 - 2.(1 - Dc).(1 - Sc)
//
//   if (2.Sc.m <= 1.m)
//     Dc'  = 2.Sc.Dc.m + Dc.(1 - m)
//   else
//     Dc'  = (1 - 2.(1 - Dc).(1 - Sc)).m - Dc.(1 - m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - SoftLight]
// ============================================================================

// [SoftLight PRGBxPRGB]
//   Dc = Dca/Da
//   if 2.Sca - Sa <= 0
//     Dca' = Dca + Sca.(1 - Da) + (2.Sca - Sa).Da.[[              Dc.(1 - Dc)           ]]
//     Da'  = Da + Sa - Sa.Da
//   else if 2.Sca - Sa > 0 and 4.Dc <= 1
//     Dca' = Dca + Sca.(1 - Da) + (2.Sca - Sa).Da.[[ 4.Dc.(4.Dc.Dc + Dc - 4.Dc + 1) - Dc]]
//     Da'  = Da + Sa - Sa.Da
//   else
//     Dca' = Dca + Sca.(1 - Da) + (2.Sca - Sa).Da.[[             sqrt(Dc) - Dc          ]]
//     Da'  = Da + Sa - Sa.Da
//
// [SoftLight XRGBxXRGB]
//   if 2.Sc <= 1
//     Dc' = Dc + (2.Sc - 1).[[              Dc.(1 - Dc)           ]]
//   else if 2.Sc > 1 and 4.Dc <= 1
//     Dc' = Dc + (2.Sc - 1).[[ 4.Dc.(4.Dc.Dc + Dc - 4.Dc + 1) - Dc]]
//   else
//     Dc' = Dc + (2.Sc - 1).[[             sqrt(Dc) - Dc          ]]

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Difference]
// ============================================================================

// [Difference PRGBxPRGB]
//   Dca' = Dca + Sca - 2.min(Sca.Da, Dca.Sa)
//   Da'  = Sa + Da - Sa.Da
//
//   Dca' = Dca + Sca.m - 2.min(Sca.m.Da, Dca.Sa.m)
//   Da'  = Sa.m + Da - Sa.m.Da
//
// [Difference PRGBxXRGB]
//   Dca' = Dca + Sc - 2.min(Sc.Da, Dca)
//   Da'  = 1
//
//   Dca' = Dca + Sc.m - 2.min(Sc.m.Da, Dca)
//   Da'  = Da + 1.m - m.Da
//
// [Difference XRGBxPRGB]
//   Dc'  = Dc + Sca   - 2.min(Sca  , Dc.Sa)
//   Dc'  = Dc + Sca.m - 2.min(Sca.m, Dc.Sa.m)
//
// [Difference XRGBxXRGB]
//   Dc'  = Dc + Sc   - 2.min(Sc  , Dc  )
//   Dc'  = Dc + Sc.m - 2.min(Sc.m, Dc.m)

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Exclusion]
// ============================================================================

// [Exclusion PRGBxPRGB]
//   Dca' = Dca + Sca.(Da - 2.Dca)
//   Da'  = Da  + Sa - Sa.Da
//
//   Dca' = Dca + Sca.m.(Da - 2.Dca)
//   Da'  = Da  + Sa.m - Sa.m.Da
//
// [Exclusion PRGBxXRGB] ~= [Exclusion PRGBxPRGB]
//   Dca' = Dca + Sc.(Da - 2.Dca)
//   Da'  = Da  + 1 - 1.Da
//
//   Dca' = Dca + Sc.m.(Da - 2.Dca)
//   Da'  = Da  + 1.m - 1.m.Da
//
// [Exclusion XRGBxPRGB]
//   Dc'  = Dc + Sca  .(1 - 2.Dc)
//   Dc'  = Dc + Sca.m.(1 - 2.Dc)
//
// [Exclusion XRGBxXRGB] ~= [Exclusion XRGBxPRGB]
//   Dc'  = Dc + Sc  .(1 - 2.Dc)
B2D_DEFINE_COMP_OP_CONV(FROM(Exclusion), kPRGB32, kXRGB32, TO(Exclusion), kPRGB32, kPRGB32);
//   Dc'  = Dc + Sc.m.(1 - 2.Dc)
B2D_DEFINE_COMP_OP_CONV(FROM(Exclusion), kXRGB32, kXRGB32, TO(Exclusion), kXRGB32, kPRGB32);

// ============================================================================
// [b2d::Pipe2D::CompOpConvT - Cleanup]
// ============================================================================

#undef FROM
#undef TO
#undef B2D_DEFINE_COMP_OP_CONV

// ============================================================================
// [b2d::Pipe2D::CompOpFlagsT]
// ============================================================================

template<uint32_t ID>
struct CompOpFlagsT {
  enum Flags : uint32_t {
    kFlags = 0
  };
};

#define B2D_DEFINE_COMP_OP_FLAGS(ID, FLAGS) \
template<>                                  \
struct CompOpFlagsT<CompOp::ID> {           \
  enum Flags : uint32_t {                   \
    kFlags = FLAGS                          \
  };                                        \
};

#define F(FLAG) CompOpInfo::kFlag##FLAG

// ---------------------+------------+----------+-------+-------+-------+-------+-----------+----------+
//                      |  Operator  |   Type   | UseDc | UseDa | UseSc | UseSa |   NopDa   |   NopSa  |
// ---------------------+------------+----------+-------+-------+-------+-------+-----------+----------+
B2D_DEFINE_COMP_OP_FLAGS(kSrc        , F(TypeB) | 0     | 0     | F(Sc) | F(Sa) | 0         | 0        )
B2D_DEFINE_COMP_OP_FLAGS(kSrcOver    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kSrcIn      , F(TypeB) | 0     | F(Da) | F(Sc) | F(Sa) | F(NopDa0) | 0        )
B2D_DEFINE_COMP_OP_FLAGS(kSrcOut     , F(TypeB) | 0     | F(Da) | F(Sc) | F(Sa) | 0         | 0        )
B2D_DEFINE_COMP_OP_FLAGS(kSrcAtop    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | F(NopDa0) | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kDst        , F(TypeC) | F(Dc) | F(Da) | 0     | 0     | F(Nop)    | F(Nop)   )
B2D_DEFINE_COMP_OP_FLAGS(kDstOver    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | F(NopDa1) | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kDstIn      , F(TypeB) | F(Dc) | F(Da) | 0     | F(Sa) | 0         | F(NopSa1))
B2D_DEFINE_COMP_OP_FLAGS(kDstOut     , F(TypeA) | F(Dc) | F(Da) | 0     | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kDstAtop    , F(TypeB) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | 0        )
B2D_DEFINE_COMP_OP_FLAGS(kXor        , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kClear      , F(TypeC) | 0     | 0     | 0     | 0     | F(NopDa0) | 0        )

B2D_DEFINE_COMP_OP_FLAGS(kPlus       , F(TypeC) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kPlusAlt    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kMinus      , F(TypeC) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kMultiply   , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | F(NopDa0) | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kScreen     , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kOverlay    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kDarken     , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kLighten    , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kColorDodge , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kColorBurn  , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kLinearBurn , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kLinearLight, F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kPinLight   , F(TypeC) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kHardLight  , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kSoftLight  , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kDifference , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))
B2D_DEFINE_COMP_OP_FLAGS(kExclusion  , F(TypeA) | F(Dc) | F(Da) | F(Sc) | F(Sa) | 0         | F(NopSa0))

B2D_DEFINE_COMP_OP_FLAGS(_kAlphaSet  , F(TypeC) | 0     | 0     | 0     | 0     | F(NopSa1) | 0        )
B2D_DEFINE_COMP_OP_FLAGS(_kAlphaInv  , F(TypeC) | 0     | 0     | 0     | 0     | 0         | 0        )

#undef F
#undef B2D_DEFINE_COMP_OP_FLAGS

// ============================================================================
// [b2d::Pipe2D::CompOpInfo - Table]
// ============================================================================

const CompOpInfo CompOpInfo::table[CompOp::_kInternalCount] = {
  #define ROW(ARGB_ARGB, ARGB_RGB, ARGB_A, ARGB_0, RGB_ARGB, RGB_RGB, RGB_A, RGB_0, A_A, A_1, A_0) \
  {                                                                        \
    CompOpFlagsT<CompOp::ARGB_ARGB>::kFlags,                               \
    {                                                                      \
      0             , 0             , 0               , 0                , \
      CompOp::A_0   , CompOp::A_A   , CompOp::A_1     , CompOp::A_A      , \
      CompOp::RGB_0 , CompOp::RGB_A , CompOp::RGB_RGB , CompOp::RGB_ARGB , \
      CompOp::ARGB_0, CompOp::ARGB_A, CompOp::ARGB_RGB, CompOp::ARGB_ARGB  \
    }                                                                      \
  }

  // +------------+-------------+-------------+-----------+-------------+-------------+------------+-----------+-----------+-----------+-----------+
  // | ARGB<-ARGB |  ARGB<-RGB  |   ARGB<-A   |  ARGB<-0  |  RGB<-ARGB  |  RGB<-RGB   |   RGB<-A   |  RGB<-0   | A<-A[RGB] |    A<-1   |    A<-0   |
  // +------------+-------------+-------------+-----------+-------------+-------------+------------+-----------+-----------+-----------+-----------+

  ROW(kSrc        , kSrc        , kSrc        , kClear    , kSrc        , kSrc        , kSrc       , kClear    , kSrc      , _kAlphaSet , kClear    ),
  ROW(kSrcOver    , kSrc        , kSrcOver    , kDst      , kSrcOver    , kSrc        , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet , kDst      ),
  ROW(kSrcIn      , kSrcIn      , kSrcIn      , kClear    , kSrc        , kSrc        , kSrc       , kClear    , kSrcIn    , kDst      , kClear    ),
  ROW(kSrcOut     , kSrcOut     , kSrcOut     , kClear    , kClear      , kClear      , kClear     , kClear    , kSrcOut   , _kAlphaInv, kClear    ),
  ROW(kSrcAtop    , kSrcIn      , kSrcAtop    , kDst      , kSrcOver    , kSrc        , kSrcOver   , kDst      , kDst      , kDst      , kDst      ),
  ROW(kDst        , kDst        , kDst        , kDst      , kDst        , kDst        , kDst       , kDst      , kDst      , kDst      , kDst      ),
  ROW(kDstOver    , kDstOver    , kDstOver    , kDst      , kDst        , kDst        , kDst       , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kDstIn      , kDst        , kDstIn      , kClear    , kDstIn      , kDst        , kDstIn     , kClear    , kSrcIn    , kDst      , kClear    ),
  ROW(kDstOut     , kClear      , kDstOut     , kDst      , kDstOut     , kClear      , kDstOut    , kDst      , kDstOut   , kClear    , kDst      ),
  ROW(kDstAtop    , kDstOver    , kDstAtop    , kClear    , kDstIn      , kDst        , kDstIn     , kClear    , kSrc      , _kAlphaSet, kClear    ),
  ROW(kXor        , kSrcOut     , kXor        , kDst      , kDstOut     , kClear      , kDstOut    , kDst      , kXor      , _kAlphaInv, kDst      ),
  ROW(kClear      , kClear      , kClear      , kClear    , kClear      , kClear      , kClear     , kClear    , kClear    , kClear    , kClear    ),

  ROW(kPlus       , kPlus       , kPlus       , kDst      , kPlus       , kPlus       , kPlus      , kDst      , kPlus     , _kAlphaSet, kDst      ),
  ROW(kPlusAlt    , kPlusAlt    , kPlusAlt    , kDst      , kPlusAlt    , kPlusAlt    , kPlusAlt   , kDst      , kPlusAlt  , _kAlphaSet, kDst      ),
  ROW(kMinus      , kMinus      , kMinus      , kDst      , kMinus      , kMinus      , kMinus     , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kMultiply   , kMultiply   , kDstOver    , kDst      , kMultiply   , kMultiply   , kDst       , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kScreen     , kScreen     , kSrcOver    , kDst      , kScreen     , kScreen     , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kOverlay    , kOverlay    , kOverlay    , kDst      , kOverlay    , kOverlay    , kOverlay   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kDarken     , kDarken     , kDstOver    , kDst      , kDarken     , kDarken     , kDst       , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kLighten    , kLighten    , kSrcOver    , kDst      , kLighten    , kLighten    , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kColorDodge , kColorDodge , kSrcOver    , kDst      , kColorDodge , kColorDodge , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kColorBurn  , kColorBurn  , kSrcOver    , kDst      , kColorBurn  , kColorBurn  , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kLinearBurn , kLinearBurn , kDstOver    , kDst      , kLinearBurn , kLinearBurn , kDstOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kLinearLight, kLinearLight, kSrcOver    , kDst      , kLinearLight, kLinearLight, kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kPinLight   , kPinLight   , kSrcOver    , kDst      , kPinLight   , kPinLight   , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kHardLight  , kHardLight  , kSrcOver    , kDst      , kHardLight  , kHardLight  , kSrcOver   , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kSoftLight  , kSoftLight  , kSoftLight  , kDst      , kSoftLight  , kSoftLight  , kSoftLight , kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kDifference , kDifference , kDifference , kDst      , kDifference , kDifference , kDifference, kDst      , kSrcOver  , _kAlphaSet, kDst      ),
  ROW(kExclusion  , kExclusion  , kExclusion  , kDst      , kExclusion  , kExclusion  , kExclusion , kDst      , kSrcOver  , _kAlphaSet, kDst      ),

  ROW(_kAlphaSet  , _kAlphaSet  , _kAlphaSet  , _kAlphaSet, _kAlphaSet  , _kAlphaSet  , _kAlphaSet , _kAlphaSet, _kAlphaSet, _kAlphaSet, _kAlphaSet),
  ROW(_kAlphaInv  , _kAlphaInv  , _kAlphaInv  , _kAlphaInv, _kAlphaInv  , _kAlphaInv  , _kAlphaInv , _kAlphaInv, _kAlphaInv, _kAlphaInv, _kAlphaInv)

  #undef ROW
};

B2D_END_SUB_NAMESPACE
