// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./pipecompiler_p.h"
#include "./pipepart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::PipePart - Construction / Destruction]
// ============================================================================

PipePart::PipePart(PipeCompiler* pc, uint32_t partType) noexcept
  : pc(pc),
    cc(pc->cc),
    _partType(partType),
    _childrenCount(0),
    _maxOptLevelSupported(kPipeOpt_None),
    _flags(0),
    _persistentRegs(),
    _spillableRegs(),
    _temporaryRegs(),
    _globalHook(nullptr) {

  std::memset(_hasLowRegs, 0, sizeof(_hasLowRegs));
  _children[0] = nullptr;
  _children[1] = nullptr;
}

// ============================================================================
// [b2d::Pipe2D::PipePart - Prepare]
// ============================================================================

void PipePart::preparePart() noexcept {
  prepareChildren();
}

void PipePart::prepareChildren() noexcept {
  size_t count = childrenCount();
  for (size_t i = 0; i < count; i++) {
    PipePart* childPart = _children[i];
    if (!(childPart->flags() & kFlagPrepareDone))
      childPart->preparePart();
  }
}

B2D_END_SUB_NAMESPACE
