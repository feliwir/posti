// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./fetchpixelptrpart_p.h"
#include "./pipecompiler_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::FetchPixelPtrPart - Construction / Destruction]
// ============================================================================

FetchPixelPtrPart::FetchPixelPtrPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept
  : FetchPart(pc, fetchType, fetchExtra, pixelFormat) {

  _maxOptLevelSupported = kPipeOpt_X86_AVX;
  _maxPixels = kUnlimitedMaxPixels;
  _ptrAlignment = 0;
}

// ============================================================================
// [b2d::Pipe2D::FetchPixelPtrPart - Fetch]
// ============================================================================

void FetchPixelPtrPart::fetch1(PixelARGB& p, uint32_t flags) noexcept {
  pc->xFetchARGB32_1x(p, flags, x86::ptr(_ptr), 4);
  pc->xSatisfyARGB32_1x(p, flags);
}

void FetchPixelPtrPart::fetch4(PixelARGB& p, uint32_t flags) noexcept {
  pc->xFetchARGB32_4x(p, flags, x86::ptr(_ptr), _ptrAlignment);
  pc->xSatisfyARGB32_Nx(p, flags);
}

void FetchPixelPtrPart::fetch8(PixelARGB& p, uint32_t flags) noexcept {
  pc->xFetchARGB32_8x(p, flags, x86::ptr(_ptr), _ptrAlignment);
  pc->xSatisfyARGB32_Nx(p, flags);
}

B2D_END_SUB_NAMESPACE
