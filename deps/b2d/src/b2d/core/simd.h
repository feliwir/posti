// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

#ifndef _B2D_CORE_SIMD_H
#define _B2D_CORE_SIMD_H

#include "../core/globals.h"

#if B2D_ARCH_SSE || B2D_ARCH_SSE2 || B2D_ARCH_AVX || B2D_ARCH_AVX2
  #include "../core/simd_x86.h"
#endif

#if !defined(B2D_SIMD_I)
  #define B2D_SIMD_I 0
#endif

#if !defined(B2D_SIMD_F)
  #define B2D_SIMD_F 0
#endif

#if !defined(B2D_SIMD_D)
  #define B2D_SIMD_D 0
#endif

#endif // _B2D_CORE_SIMD_H
