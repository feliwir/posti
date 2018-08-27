// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPESIGNATURE_P_H
#define _B2D_PIPE2D_PIPESIGNATURE_P_H

// [Dependencies]
#include "./pipepart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::PipeSignature]
// ============================================================================

//! \internal
//!
//! Pipe2D signature stored as `uint32_t`.
struct PipeSignature {
  B2D_INLINE PipeSignature() noexcept = default;
  constexpr PipeSignature(uint32_t value) : _value(value) {}

  //! Signature masks & shifts.
  enum Bits : uint32_t {
    kDstFormatShift   = 0,
    kDstFormatBits    = 0x0007,
    kDstFormatMask    = kDstFormatBits << kDstFormatShift,

    kSrcFormatShift   = 3,
    kSrcFormatBits    = 0x0007,
    kSrcFormatMask    = kSrcFormatBits << kSrcFormatShift,

    kFillTypeShift   = 6,
    kFillTypeBits    = 0x0003,
    kFillTypeMask    = kFillTypeBits << kFillTypeShift,

    kClipModeShift   = 8,
    kClipModeBits    = 0x0001,
    kClipModeMask    = kClipModeBits << kClipModeShift,

    kCompOpShift     = 9,
    kCompOpBits      = 0x001F,
    kCompOpMask      = kCompOpBits << kCompOpShift,

    kFetchTypeShift  = 14,
    kFetchTypeBits   = 0x001F,
    kFetchTypeMask   = kFetchTypeBits << kFetchTypeShift,

    kFetchExtraShift = 19,
    kFetchExtraBits  = 0x03FF,
    kFetchExtraMask  = kFetchExtraBits << kFetchExtraShift
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Reset all values to zero.
  inline void reset() noexcept { _value = 0; }

  //! Get the signature packed into a 32-bit integer.
  inline uint32_t value() const noexcept { return _value; }
  //! Set the signature from a packed 32-bit integer.
  inline void setValue(uint32_t value) noexcept { _value = value; }
  //! Set the signature from another `PipeSignature`.
  inline void setValue(const PipeSignature& other) noexcept { _value = other._value; }

  //! Get destination pixel format.
  inline uint32_t dstPixelFormat() const noexcept { return (_value >> kDstFormatShift) & kDstFormatBits; }
  //! Get source pixel format.
  inline uint32_t srcPixelFormat() const noexcept { return (_value >> kSrcFormatShift) & kSrcFormatBits; }
  //! Get sweep type.
  inline uint32_t fillType() const noexcept { return (_value >> kFillTypeShift) & kFillTypeBits; }
  //! Get clip mode.
  inline uint32_t clipMode() const noexcept { return (_value >> kClipModeShift) & kClipModeBits; }
  //! Get compositing operator.
  inline uint32_t compOp() const noexcept { return (_value >> kCompOpShift) & kCompOpBits; }
  //! Get fetch type.
  inline uint32_t fetchType() const noexcept { return (_value >> kFetchTypeShift) & kFetchTypeBits; }
  //! Get fetch data.
  inline uint32_t fetchExtra() const noexcept { return (_value >> kFetchExtraShift) & kFetchExtraBits; }

  // The following methods are used to build the signature. They use '|' operator
  // which doesn't clear the previous value, each function is expected to be called
  // only once after `reset()`.

  //! Combine with an another signature.
  inline void add(uint32_t value) noexcept { _value |= value; }
  //! Combine with an another signature.
  inline void add(const PipeSignature& other) noexcept { _value |= other._value; }

  //! Add destination pixel format.
  inline void addDstPixelFormat(uint32_t v) noexcept { add(v << kDstFormatShift); }
  //! Add source pixel format.
  inline void addSrcPixelFormat(uint32_t v) noexcept { add(v << kSrcFormatShift); }
  //! Add sweep type.
  inline void addFillType(uint32_t v) noexcept { add(v << kFillTypeShift); }
  //! Add clip mode.
  inline void addClipMode(uint32_t v) noexcept { add(v << kClipModeShift); }
  //! Add compositing operator.
  inline void addCompOp(uint32_t v) noexcept { add(v << kCompOpShift); }
  //! Add fetch type.
  inline void addFetchType(uint32_t v) noexcept { add(v << kFetchTypeShift); }
  //! Add fetch data.
  inline void addFetchExtra(uint32_t v) noexcept { add(v << kFetchExtraShift); }

  inline void resetDstPixelFormat() noexcept { _value &= ~kDstFormatMask; }
  inline void resetSrcPixelFormat() noexcept { _value &= ~kSrcFormatMask; }
  inline void resetFillType() noexcept { _value &= ~kFillTypeMask; }
  inline void resetClipMode() noexcept { _value &= ~kClipModeMask; }
  inline void resetCompOp() noexcept { _value &= ~kCompOpMask; }
  inline void resetFetchType() noexcept { _value &= ~kFetchTypeMask; }
  inline void resetFetchExtra() noexcept { _value &= ~kFetchExtraMask; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _value;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPESIGNATURE_P_H
