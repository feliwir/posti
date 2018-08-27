// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPEREGUSAGE_P_H
#define _B2D_PIPE2D_PIPEREGUSAGE_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::PipeRegUsage]
// ============================================================================

//! Registers that are used/reserved by a Pipe2D::Part.
struct PipeRegUsage {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept {
    for (uint32_t i = 0; i < kNumVirtGroups; i++)
      _data[i] = 0;
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  inline uint32_t& operator[](uint32_t kind) noexcept {
    ASMJIT_ASSERT(kind < kNumVirtGroups);
    return _data[kind];
  }

  inline const uint32_t& operator[](uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumVirtGroups);
    return _data[kind];
  }

  inline void set(const PipeRegUsage& other) noexcept {
    for (uint32_t i = 0; i < kNumVirtGroups; i++)
      _data[i] = other._data[i];
  }

  inline void add(const PipeRegUsage& other) noexcept {
    for (uint32_t i = 0; i < kNumVirtGroups; i++)
      _data[i] += other._data[i];
  }

  inline void max(const PipeRegUsage& other) noexcept {
    for (uint32_t i = 0; i < kNumVirtGroups; i++)
      _data[i] = Math::max(_data[i], other._data[i]);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _data[kNumVirtGroups];
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPEREGUSAGE_P_H
