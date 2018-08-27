// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_SIMDUTILS_P_H
#define _B2D_CORE_SIMDUTILS_P_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [B2D_SIMD_LOOP_32x4]
// ============================================================================

//! Define a blit that processes 4 (32-bit) pixels at a time in main loop.
#define B2D_SIMD_LOOP_32x4_INIT()                                             \
  size_t miniLoopCnt;                                                         \
  size_t mainLoopCnt;

#define B2D_SIMD_LOOP_32x4_MINI_BEGIN(LOOP, DST, COUNT)                       \
  miniLoopCnt = std::min(size_t((uintptr_t(0) - ((uintptr_t)(DST)/4)) & 0x3), \
                         size_t(COUNT));                                      \
  mainLoopCnt = size_t(COUNT) - miniLoopCnt;                                  \
  if (!miniLoopCnt) goto On##LOOP##_MiniSkip;                                 \
                                                                              \
On##LOOP##_MiniBegin:                                                         \
  do {

#define B2D_SIMD_LOOP_32x4_MINI_END(LOOP)                                     \
  } while (--miniLoopCnt);                                                    \
                                                                              \
On##LOOP##_MiniSkip:                                                          \
  miniLoopCnt = mainLoopCnt & 3;                                              \
  mainLoopCnt /= 4;                                                           \
  if (!mainLoopCnt) goto On##LOOP##_MainSkip;

#define B2D_SIMD_LOOP_32x4_MAIN_BEGIN(LOOP)                                   \
  do {

#define B2D_SIMD_LOOP_32x4_MAIN_END(LOOP)                                     \
  } while (--mainLoopCnt);                                                    \
                                                                              \
On##LOOP##_MainSkip:                                                          \
  if (miniLoopCnt) goto On##LOOP##_MiniBegin;                                 \
                                                                              \
On##LOOP##_End:                                                               \
  (void)0;

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_SIMDUTILS_P_H
