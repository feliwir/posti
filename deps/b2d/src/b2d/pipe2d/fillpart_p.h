// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FILLPART_P_H
#define _B2D_PIPE2D_FILLPART_P_H

// [Dependencies]
#include "./pipepart_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FillPart]
// ============================================================================

//! Pipeline fill part.
class FillPart : public PipePart {
public:
  B2D_NONCOPYABLE(FillPart)

  enum : uint32_t {
    kIndexDstPart = 0,
    kIndexCompOpPart = 1
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FillPart(PipeCompiler* pc, uint32_t fillType, FetchPixelPtrPart* dstPart, CompOpPart* compOpPart) noexcept;

  // --------------------------------------------------------------------------
  // [Children]
  // --------------------------------------------------------------------------

  inline FetchPixelPtrPart* dstPart() const noexcept {
    return reinterpret_cast<FetchPixelPtrPart*>(_children[kIndexDstPart]);
  }

  inline void setDstPart(FetchPixelPtrPart* part) noexcept {
    _children[kIndexDstPart] = reinterpret_cast<PipePart*>(part);
  }

  inline CompOpPart* compOpPart() const noexcept {
    return reinterpret_cast<CompOpPart*>(_children[kIndexCompOpPart]);
  }

  inline void setCompOpPart(FetchPixelPtrPart* part) noexcept {
    _children[kIndexCompOpPart] = reinterpret_cast<PipePart*>(part);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get fill type, see \ref FillType.
  inline uint32_t fillType() const noexcept { return _fillType; }
  //! Get whether the fill type matches `fillType`.
  inline bool isFillType(uint32_t fillType) const noexcept { return _fillType == fillType; }

  //! Get whether fill-type is a pure rectangular fill (aligned or fractional).
  //!
  //! Rectangle fills have some properties that can be exploited by other parts.
  inline bool isRectFill() const noexcept { return _isRectFill; }
  inline bool isAnalyticFill() const noexcept { return _fillType == kFillTypeAnalytic; }

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  //! Compile the fill part.
  virtual void compile() noexcept = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _fillType;                     //!< Fill type.
  bool _isRectFill;                      //!< True if this a pure rectangle fill (either axis-aligned or fractional).
};

// ============================================================================
// [b2d::Pipe2D::FillBoxAAPart]
// ============================================================================

class FillBoxAAPart final : public FillPart {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FillBoxAAPart(PipeCompiler* pc, uint32_t fillType, FetchPixelPtrPart* dstPart, CompOpPart* compOpPart) noexcept;

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  void compile() noexcept override;
};

// ============================================================================
// [b2d::Pipe2D::FillBoxAUPart]
// ============================================================================

class FillBoxAUPart final : public FillPart {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FillBoxAUPart(PipeCompiler* pc, uint32_t fillType, FetchPixelPtrPart* dstPart, CompOpPart* compOpPart) noexcept;

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  void compile() noexcept override;
};

// ============================================================================
// [b2d::Pipe2D::FillAnalyticPart]
// ============================================================================

class FillAnalyticPart final : public FillPart {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FillAnalyticPart(PipeCompiler* pc, uint32_t fillType, FetchPixelPtrPart* dstPart, CompOpPart* compOpPart) noexcept;

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  void compile() noexcept override;

  //! Adds covers held by `val` to the accumulator `acc`.
  void accumulateCovers(const x86::Vec& acc, const x86::Vec& val) noexcept;

  //! Calculates masks for 4 pixels - this works for both NonZero and EvenOdd fill
  //! rules. The first VAND operation that uses `fillRuleMask` makes sure that we
  //! only keep the important part in EvenOdd case and the last VMINI16 makes sure
  //! we clamp it in NonZero case.
  void calcMasksFromCovers(const x86::Vec& dst, const x86::Vec& src, const x86::Vec& fillRuleMask, const x86::Vec& globalAlpha, bool unpack) noexcept;

  //! Emits the following:
  //!
  //! ```
  //! dstPtr -= x * dstBpp;
  //! cellPtr -= x * 4;
  //! ```
  void disadvanceDstPtrAndCellPtr(const x86::Gp& dstPtr, const x86::Gp& cellPtr, const x86::Gp& x, int dstBpp) noexcept;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FILLPART_P_H
