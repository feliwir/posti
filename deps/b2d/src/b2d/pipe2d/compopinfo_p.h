// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_COMPOPINFO_P_H
#define _B2D_PIPE2D_COMPOPINFO_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

struct CompOpInfo {
  // --------------------------------------------------------------------------
  // [Limits]
  // --------------------------------------------------------------------------

  enum Limits : uint32_t {
    kMaxComponentOptions = PixelFlags::kComponentMask + 1u
  };

  // --------------------------------------------------------------------------
  // [Flags]
  // --------------------------------------------------------------------------

  //! Composition flags, used by compositor and pipeline generator.
  enum Flags : uint32_t {
    kFlagDc               = 0x00000001u, //!< Uses `Dc` (destination color).
    kFlagDa               = 0x00000002u, //!< Uses `Da` (destination alpha).
    kFlagDcDa             = 0x00000003u, //!< Uses both `Dc` and `Da`.

    kFlagSc               = 0x00000004u, //!< Uses `Sc` (source color).
    kFlagSa               = 0x00000008u, //!< Uses `Sa` (source alpha).
    kFlagScSa             = 0x0000000Cu, //!< Uses both `Sc` and `Sa`,

    kFlagTypeA            = 0x00000010u, //!< TypeA operator - "D*(1-M) + Op(D, S)*M" == "Op(D, S * M)".
    kFlagTypeB            = 0x00000020u, //!< TypeB operator - "D*(1-M) + Op(D, S)*M" == "Op(D, S*M) + D*(1-M)".
    kFlagTypeC            = 0x00000040u, //!< TypeC operator - cannot be simplified.

    kFlagNonSeparable     = 0x00000100u, //!< Non-separable operator.

    kFlagNop              = 0x00000800u, //!< Destination is never changed (NOP).
    kFlagNopDa0           = 0x00001000u, //!< Destination is changed only if `Da != 0`.
    kFlagNopDa1           = 0x00002000u, //!< Destination is changed only if `Da != 1`.
    kFlagNopSa0           = 0x00004000u, //!< Destination is changed only if `Sa != 0`.
    kFlagNopSa1           = 0x00008000u  //!< Destination is changed only if `Sa != 1`.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline uint32_t flags() const noexcept { return _flags; }

  inline bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }
  inline bool hasDst() const noexcept { return hasFlag(kFlagDcDa); }
  inline bool hasSrc() const noexcept { return hasFlag(kFlagScSa); }

  inline bool isTypeA() const noexcept { return hasFlag(kFlagTypeA); }
  inline bool isTypeB() const noexcept { return hasFlag(kFlagTypeB); }
  inline bool isTypeC() const noexcept { return hasFlag(kFlagTypeC); }

  //! Simplify a composition operator based on both destination and source pixel
  //! formats (their components to be exact). The result is a simplified operator
  //! that would produce the same result but may be faster.
  //!
  //! If no simplification is possible it returns the current operator.
  inline uint32_t simplify(uint32_t dstComponents, uint32_t srcComponents) const noexcept {
    B2D_ASSERT((dstComponents & ~PixelFlags::kComponentMask) == 0);
    B2D_ASSERT((srcComponents & ~PixelFlags::kComponentMask) == 0);

    return _simplify[dstComponents * 4u + srcComponents];
  }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  static const CompOpInfo table[CompOp::_kInternalCount];

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Composition operator flags.
  uint32_t _flags;

  //! Simplification matrix, uses \ref PixelFormat::Components to simplify the
  //! composition of `Dst OP Src` based on properties of both pixel formats.
  //! This is very useful in cases where either one or both pixel formats are
  //! not `ARGB`.
  uint8_t _simplify[kMaxComponentOptions * kMaxComponentOptions];
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_COMPOPINFO_P_H
