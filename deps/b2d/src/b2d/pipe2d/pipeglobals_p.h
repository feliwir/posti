// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPEGLOBALS_P_H
#define _B2D_PIPE2D_PIPEGLOBALS_P_H

// [Dependencies]
#include "../core/compop.h"
#include "../core/gradient.h"
#include "../core/math.h"
#include "../core/matrix2d.h"
#include "../core/pattern.h"
#include "../core/pixelformat.h"
#include "../core/runtime.h"
#include "../core/support.h"
#include "../core/wrap.h"

#if B2D_ARCH_X86
  #include <asmjit/x86.h>
#endif

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class PipePart;
class PipeCompiler;

class CompOpPart;

class FetchPart;
class FetchGradientPart;
class FetchPixelPtrPart;
class FetchSolidPart;
class FetchTexturePart;

class FillPart;
class FillBoxPart;
class FillAnalyticPart;

// ============================================================================
// [b2d::Pipe2D::FillFunc]
// ============================================================================

typedef Error (B2D_CDECL* FillFunc)(void* ctxData, void* fillData, const void* fetchData);

// ============================================================================
// [b2d::Pipe2D::PtrInfo]
// ============================================================================

//! Host pointer size and shift, used for addressing, LEA, etc...
enum PtrInfo {
  kPtrSize = int(sizeof(void*)),         //!< Host pointer size.
  kPtrShift = (kPtrSize == 4) ? 2 : 3    //!< Shift needed to scale an index to the host pointer size.
};

// ============================================================================
// [b2d::Pipe2D::Limits]
// ============================================================================

enum Limits {
  kNumVirtGroups = asmjit::BaseReg::kGroupVirt
};

// ============================================================================
// [b2d::Pipe2D::PipeCMaskLoopType]
// ============================================================================

//! Pipeline loop type, used by span fillers & compositors.
enum PipeCMaskLoopType {
  kPipeCMaskLoopNone       = 0,          //!< Not in a loop-mode.
  kPipeCMaskLoopOpq        = 1,          //!< CMask opaque loop (alpha is 1.0).
  kPipeCMaskLoopMsk        = 2           //!< CMask masked loop (alpha is not 1.0).
};

// ============================================================================
// [b2d::Pipe2D::PipeOptLevel]
// ============================================================================

//! Pipeline optimization level.
enum PipeOptLevel {
  kPipeOpt_None            = 0,          //!< Safest optimization level.

  kPipeOpt_X86_SSE2        = 1,          //!< SSE2+ optimization level (minimum on X86).
  kPipeOpt_X86_SSE3        = 2,          //!< SSE3+ optimization level.
  kPipeOpt_X86_SSSE3       = 3,          //!< SSSE3+ optimization level.
  kPipeOpt_X86_SSE4_1      = 4,          //!< SSE4.1+ optimization level.
  kPipeOpt_X86_SSE4_2      = 5,          //!< SSE4.2+ optimization level.
  kPipeOpt_X86_AVX         = 6,          //!< AVX+ optimization level.
  kPipeOpt_X86_AVX2        = 7           //!< AVX2+ optimization level.
};

// ============================================================================
// [b2d::Pipe2D::FillType]
// ============================================================================

//! Fill type.
enum FillType {
  kFillTypeNone            = 0,          //!< Nothing to fill, should not be used.
  kFillTypeBoxAA           = 1,          //!< Fill axis-aligned box.
  kFillTypeBoxAU           = 2,          //!< Fill axis-unaligned box.
  kFillTypeAnalytic        = 3,          //!< Fill analytic non-zero/even-odd.

  _kFillTypeBoxFirst       = 1,
  _kFillTypeBoxLast        = 2
};

// ============================================================================
// [b2d::Pipe2D::FetchType]
// ============================================================================

//! Fetch-type - a unique id describing what `FetchPart` does.
//!
//! NOTE: RoR means Repeat or Reflect. It's an universal fetcher for both extends.
enum FetchType {
  kFetchTypeSolid = 0,                   //!< Solid fetch.

  kFetchTypeGradientLinearPad,           //!< Linear gradient (pad)                   [ALWAYS].
  kFetchTypeGradientLinearRoR,           //!< Linear gradient (repeat|reflect)        [ALWAYS].

  kFetchTypeGradientRadialPad,           //!< Radial gradient (pad)                   [ALWAYS].
  kFetchTypeGradientRadialRepeat,        //!< Radial gradient (repeat)                [ALWAYS].
  kFetchTypeGradientRadialReflect,       //!< Radial gradient (reflect)               [ALWAYS].

  kFetchTypeGradientConical,             //!< Conical gradient (any)                  [ALWAYS].

  kFetchTypeTextureAABlit,               //!< Texture blit {aligned} (blit)           [ALWAYS].
  kFetchTypeTextureAAPadX,               //!< Texture blit {aligned} (pad-x)          [ALWAYS].
  kFetchTypeTextureAARepeatLargeX,       //!< Texture blit {aligned} (repeat-large-x) [OPTIMIZED].
  kFetchTypeTextureAARoRX,               //!< Texture blit {aligned} (ror-x)          [ALWAYS].

  kFetchTypeTextureFxPadX,               //!< Texture blit {frac-x}  pattern (pad-x)  [OPTIMIZED].
  kFetchTypeTextureFxRoRX,               //!< Texture blit {frac-x}  pattern (ror-x)  [OPTIMIZED].
  kFetchTypeTextureFyPadX,               //!< Texture blit {frac-y}  pattern (pad-x)  [OPTIMIZED].
  kFetchTypeTextureFyRoRX,               //!< Texture blit {frac-x}  pattern (ror-x)  [OPTIMIZED].
  kFetchTypeTextureFxFyPadX,             //!< Texture blit {frac-xy} pattern (pad-x)  [ALWAYS].
  kFetchTypeTextureFxFyRoRX,             //!< Texture blit {frac-xy} pattern (ror-x)  [ALWAYS].

  kFetchTypeTextureAffineNnAny,          //!< Texture blit {affine} {nearest}  (any)  [ALWAYS].
  kFetchTypeTextureAffineNnOpt,          //!< Texture blit {affine} {nearest}  (any)  [ALWAYS].
  kFetchTypeTextureAffineBiAny,          //!< Texture blit {affine} {bilinear} (any)  [ALWAYS].
  kFetchTypeTextureAffineBiOpt,          //!< Texture blit {affine} {bilinear} (any)  [ALWAYS].

  kFetchTypeCount,                       //!< Number of fetch types.
  kFetchTypePixelPtr = 0xFF,             //!< Pixel pointer, not a valid fetch type.

  _kFetchTypeGradientFirst        = kFetchTypeGradientLinearPad,
  _kFetchTypeGradientLast         = kFetchTypeGradientConical,

  _kFetchTypeGradientLinearFirst  = kFetchTypeGradientLinearPad,
  _kFetchTypeGradientLinearLast   = kFetchTypeGradientLinearRoR,

  _kFetchTypeGradientRadialFirst  = kFetchTypeGradientRadialPad,
  _kFetchTypeGradientRadialLast   = kFetchTypeGradientRadialReflect,

  _kFetchTypeGradientConicalFirst = kFetchTypeGradientConical,
  _kFetchTypeGradientConicalLast  = kFetchTypeGradientConical,

  _kFetchTypeTextureFirst         = kFetchTypeTextureAABlit,
  _kFetchTypeTextureLast          = kFetchTypeTextureAffineBiOpt,

  _kFetchTypeTextureAAFirst       = kFetchTypeTextureAABlit,
  _kFetchTypeTextureAALast        = kFetchTypeTextureAARoRX,

  _kFetchTypeTextureAUFirst       = kFetchTypeTextureFxPadX,
  _kFetchTypeTextureAULast        = kFetchTypeTextureFxFyRoRX,

  _kFetchTypeTextureFxFirst       = kFetchTypeTextureFxPadX,
  _kFetchTypeTextureFxLast        = kFetchTypeTextureFxRoRX,

  _kFetchTypeTextureFyFirst       = kFetchTypeTextureFyPadX,
  _kFetchTypeTextureFyLast        = kFetchTypeTextureFyRoRX,

  _kFetchTypeTextureFxFyFirst     = kFetchTypeTextureFxFyPadX,
  _kFetchTypeTextureFxFyLast      = kFetchTypeTextureFxFyRoRX,

  _kFetchTypeTextureSimpleFirst   = kFetchTypeTextureAABlit,
  _kFetchTypeTextureSimpleLast    = kFetchTypeTextureFxFyRoRX,

  _kFetchTypeTextureAffineFirst   = kFetchTypeTextureAffineNnAny,
  _kFetchTypeTextureAffineLast    = kFetchTypeTextureAffineBiOpt
};

// ============================================================================
// [b2d::Pipe2D::Value32]
// ============================================================================

union Value32 {
  uint32_t u;
  int32_t i;
  float f;
};

// ============================================================================
// [b2d::Pipe2D::Value64]
// ============================================================================

union Value64 {
  inline void expandLoToHi() noexcept { u32Hi = u32Lo; }

  uint64_t u64;
  int64_t i64;
  double d;

  int32_t i32[2];
  uint32_t u32[2];

  int16_t i16[4];
  uint16_t u16[4];

#if B2D_ARCH_LE
  struct { uint32_t i32Lo, i32Hi; };
  struct { uint32_t u32Lo, u32Hi; };
#else
  struct { uint32_t i32Hi, i32Lo; };
  struct { uint32_t u32Hi, u32Lo; };
#endif
};

// ============================================================================
// [b2d::Pipe2D::PixelData]
// ============================================================================

struct PixelData {
  inline void init(uint8_t* pixels, intptr_t stride, uint32_t w, uint32_t h) noexcept {
    this->pixels = pixels;
    this->stride = stride;
    this->width = w;
    this->height = h;
  }
  inline void reset() noexcept { init(nullptr, 0, 0, 0); }

  uint8_t* pixels;                       //!< Pixels pointer (points to the first scanline).
  intptr_t stride;                       //!< Scanline stride (can be negative).
  uint32_t width;                        //!< Width.
  uint32_t height;                       //!< Height.
};

// ============================================================================
// [b2d::Pipe2D::ContextData]
// ============================================================================

struct ContextData {
  inline void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  PixelData dst;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPEGLOBALS_P_H
