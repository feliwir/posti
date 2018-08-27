// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./fetchutils_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::IndexExtractorU16]
// ============================================================================

void IndexExtractorU16::begin(const x86::Xmm& vec) noexcept {
  _vec = vec;
  _stack.reset();

  if (_strategy == kStrategyStack) {
    _stack = _pc->tmpStack(16);
    _pc->vstorei128a(_stack, _vec);
  }
}

void IndexExtractorU16::extract(x86::Gp& dst, uint32_t index) noexcept {
  B2D_ASSERT(index <= 7);
  x86::Gp dst32 = { dst.r32() };

  switch (_strategy) {
    case kStrategyExtractSSE2: {
      _pc->vextractu16(dst32, _vec, index);
      break;
    }

    case kStrategyStack: {
      x86::Mem src16 = _stack;
      src16.setSize(2);
      src16.addOffset(int(index * 2));
      _pc->cc->movzx(dst32, src16);
      break;
    }
  }
}

// ============================================================================
// [b2d::Pipe2D::IndexExtractorU32]
// ============================================================================

void IndexExtractorU32::begin(const x86::Xmm& vec) noexcept {
  _vec = vec;
  _stack.reset();

  if (_strategy == kStrategyStack) {
    _stack = _pc->tmpStack(16);
    _pc->vstorei128a(_stack, _vec);
  }
}

void IndexExtractorU32::extract(x86::Gp& dst, uint32_t index) noexcept {
  B2D_ASSERT(index <= 3);
  x86::Gp dst32 = { dst.r32() };

  switch (_strategy) {
    case kStrategyExtractSSE4_1: {
      if (index == 0)
        _pc->vmovsi32(dst32, _vec);
      else
        _pc->vextractu32_(dst32, _vec, index);
      break;
    }

    case kStrategyStack: {
      x86::Mem src32 = _stack;
      src32.setSize(4);
      src32.addOffset(int(index * 4));
      _pc->cc->mov(dst32, src32);
      break;
    }
  }
}

// ============================================================================
// [b2d::Pipe2D::Fetch4Context]
// ============================================================================

void Fetch4Context::_init() noexcept {
  x86::Compiler* cc = pc->cc;

  // We need at least one temporary if the CPU doesn't support `SSE4.1`.
  if (!pc->hasSSE4_1()) {
    pARGB32Tmp0 = cc->newXmm("ARGB32Tmp0");
    pARGB32Tmp1 = cc->newXmm("ARGB32Tmp1");
  }

  if (fetchFlags & PixelARGB::kPC)
    pc->newXmmArray(p->pc, 1, "ARGB32");
  else
    pc->newXmmArray(p->uc, 2, "ARGB32");
}

void Fetch4Context::fetchARGB32(const x86::Mem& src) noexcept {
  B2D_ASSERT(fetchIndex <= 3);

  x86::Vec& p0 = (fetchFlags & PixelARGB::kPC) ? p->pc[0] : p->uc[0];

  if (!pc->hasSSE4_1()) {
    switch (fetchIndex) {
      case 0:
        pc->vloadi32(p0, src);
        break;

      case 1:
        pc->vloadi32(pARGB32Tmp0, src);
        break;

      case 2:
        pc->vunpackli32(p0, p0, pARGB32Tmp0);
        if (fetchFlags & PixelARGB::kPC)
          pc->vloadi32(pARGB32Tmp0, src);
        else
          pc->vloadi32(p->uc[1], src);
        break;

      case 3:
        pc->vloadi32(pARGB32Tmp1, src);
        break;
    }
  }
  else {
    switch (fetchIndex) {
      case 0:
        pc->vloadi32(p0, src);
        break;

      case 1:
        pc->vinsertu32_(p0, p0, src, 1);
        break;

      case 2:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p->pc[0], p->pc[0], src, 2);
        else
          pc->vloadi32(p->uc[1], src);
        break;

      case 3:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p->pc[0], p->pc[0], src, 3);
        else
          pc->vinsertu32_(p->uc[1], p->uc[1], src, 1);
        break;
    }
  }

  fetchIndex++;
}

void Fetch4Context::end() noexcept {
  B2D_ASSERT(fetchIndex == 4);

  if (!pc->hasSSE4_1()) {
    if (fetchFlags & PixelARGB::kPC) {
      pc->vunpackli32(pARGB32Tmp0, pARGB32Tmp0, pARGB32Tmp1);
      pc->vunpackli64(p->pc[0], p->pc[0], pARGB32Tmp0);
    }
    else {
      pc->vunpackli32(p->uc[1], p->uc[1], pARGB32Tmp1);
    }
  }

  if (fetchFlags & PixelARGB::kPC) {
    // Nothing...
  }
  else {
    pc->vmovu8u16(p->uc, p->uc);
  }
}

// ============================================================================
// [b2d::Pipe2D::Fetch8Context]
// ============================================================================

void Fetch8Context::_init() noexcept {
  x86::Compiler* cc = pc->cc;

  // We need at least one temporary if the CPU doesn't support `SSE4.1`.
  if (!pc->hasSSE4_1()) {
    pARGB32Tmp0 = cc->newXmm("ARGB32Tmp0");
    pARGB32Tmp1 = cc->newXmm("ARGB32Tmp1");
  }

  if (fetchFlags & PixelARGB::kPC)
    pc->newXmmArray(p->pc, 2, "ARGB32");
  else
    pc->newXmmArray(p->uc, 4, "ARGB32");
}

void Fetch8Context::fetchARGB32(const x86::Mem& src) noexcept {
  B2D_ASSERT(fetchIndex <= 7);

  x86::Vec& p0 = (fetchFlags & PixelARGB::kPC) ? p->pc[0] : p->uc[0];
  x86::Vec& p1 = (fetchFlags & PixelARGB::kPC) ? p->pc[1] : p->uc[2];

  if (!pc->hasSSE4_1()) {
    switch (fetchIndex) {
      case 0:
        pc->vloadi32(p0, src);
        break;

      case 1:
        pc->vloadi32(pARGB32Tmp0, src);
        break;

      case 2:
        pc->vunpackli32(p0, p0, pARGB32Tmp0);
        if (fetchFlags & PixelARGB::kPC)
          pc->vloadi32(pARGB32Tmp0, src);
        else
          pc->vloadi32(p->uc[1], src);
        break;

      case 3:
        pc->vloadi32(pARGB32Tmp1, src);
        break;

      case 4:
        if (fetchFlags & PixelARGB::kPC) {
          pc->vunpackli32(pARGB32Tmp0, pARGB32Tmp0, pARGB32Tmp1);
          pc->vunpackli64(p0, p0, pARGB32Tmp0);
        }
        else {
          pc->vunpackli32(p->uc[1], p->uc[1], pARGB32Tmp1);
        }

        pc->vloadi32(p1, src);
        break;

      case 5:
        pc->vloadi32(pARGB32Tmp0, src);
        break;

      case 6:
        pc->vunpackli32(p1, p1, pARGB32Tmp0);
        if (fetchFlags & PixelARGB::kPC)
          pc->vloadi32(pARGB32Tmp0, src);
        else
          pc->vloadi32(p->uc[3], src);
        break;

      case 7:
        pc->vloadi32(pARGB32Tmp1, src);
        break;
    }
  }
  else {
    switch (fetchIndex) {
      case 0:
        pc->vloadi32(p0, src);
        break;

      case 1:
        pc->vinsertu32_(p0, p0, src, 1);
        break;

      case 2:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p0, p0, src, 2);
        else
          pc->vloadi32(p->uc[1], src);
        break;

      case 3:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p0, p0, src, 3);
        else
          pc->vinsertu32_(p->uc[1], p->uc[1], src, 1);
        break;

      case 4:
        pc->vloadi32(p1, src);
        break;

      case 5:
        pc->vinsertu32_(p1, p1, src, 1);
        break;

      case 6:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p1, p1, src, 2);
        else
          pc->vloadi32(p->uc[3], src);
        break;

      case 7:
        if (fetchFlags & PixelARGB::kPC)
          pc->vinsertu32_(p1, p1, src, 3);
        else
          pc->vinsertu32_(p->uc[3], p->uc[3], src, 1);
        break;
    }
  }

  fetchIndex++;
}

void Fetch8Context::end() noexcept {
  B2D_ASSERT(fetchIndex == 8);

  if (!pc->hasSSE4_1()) {
    if (fetchFlags & PixelARGB::kPC) {
      pc->vunpackli32(pARGB32Tmp0, pARGB32Tmp0, pARGB32Tmp1);
      pc->vunpackli64(p->pc[1], p->pc[1], pARGB32Tmp0);
    }
    else {
      pc->vunpackli32(p->uc[3], p->uc[3], pARGB32Tmp1);
    }
  }

  if (fetchFlags & PixelARGB::kPC) {
    // Nothing...
  }
  else {
    pc->vmovu8u16(p->uc, p->uc);
  }
}

B2D_END_SUB_NAMESPACE
