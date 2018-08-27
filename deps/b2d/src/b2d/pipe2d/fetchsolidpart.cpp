// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./compoppart_p.h"
#include "./fetchdata_p.h"
#include "./fetchsolidpart_p.h"
#include "./pipecompiler_p.h"
#include "./tables_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

#define REL_SOLID(field) ASMJIT_OFFSET_OF(FetchData::Solid, field)

// ============================================================================
// [b2d::Pipe2D::FetchSolidPart - Construction / Destruction]
// ============================================================================

FetchSolidPart::FetchSolidPart(PipeCompiler* pc, uint32_t fetchType, uint32_t fetchExtra, uint32_t pixelFormat) noexcept
  : FetchPart(pc, fetchType, fetchExtra, pixelFormat) {

  _maxOptLevelSupported = kPipeOpt_X86_AVX;
  _maxPixels = kUnlimitedMaxPixels;

  _pixel.reset();
  _isTransparent = false;
}

// ============================================================================
// [b2d::Pipe2D::FetchSolidPart - Init / Fini]
// ============================================================================

void FetchSolidPart::_initPart(x86::Gp& x, x86::Gp& y) noexcept {
  B2D_UNUSED(x);
  B2D_UNUSED(y);
}

void FetchSolidPart::_finiPart() noexcept {
  // This injects a spill sequence at the end of the initialization code added
  // by a solid fetch type. This prevents spills in the middle of the generated
  // code in case there are no more registers.
  PipeScopedInject(cc, &_globalHook);

  // Packed and unpacked pixels are kept in registers, unpacked alpha values are
  // spilled.
  if (!_pixel.ua.empty()) cc->spill(_pixel.ua[0]);
  if (!_pixel.uia.empty()) cc->spill(_pixel.uia[0]);
}

// ============================================================================
// [b2d::Pipe2D::FetchSolidPart - InitSolidFlags]
// ============================================================================

void FetchSolidPart::initSolidFlags(uint32_t flags) noexcept {
  PipeScopedInject inject(cc, &_globalHook);
  PixelARGB& s = _pixel;

  if ((flags & PixelARGB::kAny) && s.pc.empty()) {
    s.pc.init(cc->newXmm("pixel.pc"));
    x86::Vec& pix = s.pc[0];

    if (!isTransparent()) {
      pc->vloadi32(pix, x86::dword_ptr(pc->_fetchData));
      pc->vswizi32(pix, pix, x86::Predicate::shuf(0, 0, 0, 0));
    }
    else {
      pc->vzeropi(pix);
    }
  }

  pc->xSatisfySolid(s, flags);
}

// ============================================================================
// [b2d::Pipe2D::FetchSolidPart - Fetch]
// ============================================================================

void FetchSolidPart::fetch1(PixelARGB& p, uint32_t flags) noexcept {
  if (flags & PixelARGB::kAny) {
    initSolidFlags(flags & PixelARGB::kAny);
    PixelARGB& s = _pixel;

    if (flags & PixelARGB::kImmutable) {
      if (flags & PixelARGB::kPC ) { p.pc.init(s.pc); }
      if (flags & PixelARGB::kUC ) { p.uc.init(s.uc); }
      if (flags & PixelARGB::kUA ) { p.ua.init(s.ua); }
      if (flags & PixelARGB::kUIA) { p.uia.init(s.uia); }
    }
    else {
      if (flags & PixelARGB::kPC) {
        p.pc.init(cc->newXmm("p.pc0"));
        pc->vmov(p.pc[0], s.pc[0]);
      }

      if (flags & PixelARGB::kUC) {
        p.uc.init(cc->newXmm("p.uc0"));
        pc->vmov(p.uc[0], s.uc[0]);
      }

      if (flags & PixelARGB::kUA) {
        p.ua.init(cc->newXmm("p.ua0"));
        pc->vmov(p.ua[0], s.ua[0]);
      }

      if (flags & PixelARGB::kUIA) {
        p.uia.init(cc->newXmm("p.uia0"));
        pc->vmov(p.uia[0], s.uia[0]);
      }
    }
  }

  pc->xSatisfyARGB32_1x(p, flags);
}

void FetchSolidPart::fetch4(PixelARGB& p, uint32_t flags) noexcept {
  if (flags & PixelARGB::kAny) {
    initSolidFlags(flags & PixelARGB::kAny);
    PixelARGB& s = _pixel;

    if (flags & PixelARGB::kImmutable) {
      if (flags & PixelARGB::kPC ) { p.pc.init(s.pc); }
      if (flags & PixelARGB::kUC ) { p.uc.init(s.uc); }
      if (flags & PixelARGB::kUA ) { p.ua.init(s.ua); }
      if (flags & PixelARGB::kUIA) { p.uia.init(s.uia); }
    }
    else {
      if (flags & PixelARGB::kPC) {
        pc->newXmmArray(p.pc, 1, "p.pc");
        pc->vmov(p.pc[0], s.pc[0]);
      }

      if (flags & PixelARGB::kUC) {
        pc->newXmmArray(p.uc, 2, "p.uc");
        pc->vmov(p.uc[0], s.uc[0]);
        pc->vmov(p.uc[1], s.uc[0]);
      }

      if (flags & PixelARGB::kUA) {
        pc->newXmmArray(p.ua, 2, "p.ua");
        pc->vmov(p.ua[0], s.ua[0]);
        pc->vmov(p.ua[1], p.ua[0]);
      }

      if (flags & PixelARGB::kUIA) {
        pc->newXmmArray(p.uia, 2, "p.uia");
        pc->vmov(p.uia[0], s.uia[0]);
        pc->vmov(p.uia[1], p.uia[0]);
      }
    }
  }

  pc->xSatisfyARGB32_Nx(p, flags);
}

B2D_END_SUB_NAMESPACE
