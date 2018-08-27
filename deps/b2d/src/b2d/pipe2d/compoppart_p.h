// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_COMPOPPART_P_H
#define _B2D_PIPE2D_COMPOPPART_P_H

// [Dependencies]
#include "./compopinfo_p.h"
#include "./fetchpart_p.h"
#include "./pipehelpers_p.h"
#include "./pipepart_p.h"
#include "./pipepixel_p.h"
#include "./tables_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::CompOpPart]
// ============================================================================

//! Pipeline combine part.
class CompOpPart : public PipePart {
public:
  B2D_NONCOPYABLE(CompOpPart)

  enum : uint32_t {
    kIndexDstPart = 0,
    kIndexSrcPart = 1
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  CompOpPart(PipeCompiler* pc, uint32_t compOp, FetchPart* dstPart, FetchPart* srcPart) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline FetchPart* dstPart() const noexcept { return reinterpret_cast<FetchPart*>(_children[kIndexDstPart]); }
  inline FetchPart* srcPart() const noexcept { return reinterpret_cast<FetchPart*>(_children[kIndexSrcPart]); }

  //! Get operator id, see \ref CompOp::Id.
  inline uint32_t compOp() const noexcept { return _compOp; }
  //! Get information associated with the operator.
  inline const CompOpInfo& compOpInfo() const noexcept { return CompOpInfo::table[_compOp]; }

  //! Get whether the destination pixel format has an alpha channel.
  inline bool hasDa() const noexcept { return _hasSa != 0; }
  //! Get whether the source pixel format has an alpha channel.
  inline bool hasSa() const noexcept { return _hasDa != 0; }

  //! Get the current loop mode.
  inline uint32_t cMaskLoopType() const noexcept { return _cMaskLoopType; }
  //! Get whether the current loop is fully opaque (no mask).
  inline bool isLoopOpaque() const noexcept { return _cMaskLoopType == kPipeCMaskLoopOpq; }
  //! Get whether the current loop is `CMask` (constant mask).
  inline bool isLoopCMask() const noexcept { return _cMaskLoopType == kPipeCMaskLoopMsk; }

  //! Get the maximum pixels the composite part can handle at a time.
  //!
  //! NOTE: This value is configured in a way that it's always one if the fetch
  //! part doesn't support more. This makes it easy to use in loop compilers.
  //! In other words, the value doesn't describe the real implementation of the
  //! composite part.
  inline uint32_t maxPixels() const noexcept { return _maxPixels; }
  //! Get the maximum pixels the children of this part can handle.
  inline uint32_t maxPixelsOfChildren() const noexcept { return Math::min(dstPart()->maxPixels(), srcPart()->maxPixels()); }

  //! Get pixel granularity passed to `init()`, otherwise the result should be zero.
  inline uint32_t pixelGranularity() const noexcept { return _pixelGranularity; }
  //! Get the minimum destination alignment required to the maximum number of pixels `_maxPixels`.
  inline uint32_t minAlignment() const noexcept { return _minAlignment; }

  inline bool isUsingSolidPre() const noexcept { return !_solidPre.pc.empty() || !_solidPre.uc.empty(); }

  // --------------------------------------------------------------------------
  // [Init / Fini]
  // --------------------------------------------------------------------------

  void init(x86::Gp& x, x86::Gp& y, uint32_t pixelGranularity) noexcept;
  void fini() noexcept;

  // --------------------------------------------------------------------------
  // [Decision Making]
  // --------------------------------------------------------------------------

  //! Get whether the opaque fill should be optimized and placed into a separate
  //! loop.
  bool shouldOptimizeOpaqueFill() const noexcept;

  //! Get whether the compositor should emit a specialized loop that contains
  //! an inlined version of `memcpy()` or `memset()`.
  bool shouldMemcpyOrMemsetOpaqueFill() const noexcept;

  // --------------------------------------------------------------------------
  // [Advance]
  // --------------------------------------------------------------------------

  void startAtX(x86::Gp& x) noexcept;
  void advanceX(x86::Gp& x, x86::Gp& diff) noexcept;
  void advanceY() noexcept;

  // --------------------------------------------------------------------------
  // [Prefetch / Postfetch]
  // --------------------------------------------------------------------------

  // These are just wrappers that call these on both source & destination parts.

  void prefetch1() noexcept;
  void enterN() noexcept;
  void leaveN() noexcept;
  void prefetchN() noexcept;
  void postfetchN() noexcept;

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  void dstFetch32(PixelARGB& out, uint32_t flags, uint32_t n) noexcept;
  void srcFetch32(PixelARGB& out, uint32_t flags, uint32_t n) noexcept;

  // --------------------------------------------------------------------------
  // [PartialFetch]
  // --------------------------------------------------------------------------

  inline bool isInPartialMode() const noexcept {
    return !_pixPartial.pc.empty();
  }

  void enterPartialMode(uint32_t partialFlags = 0) noexcept;
  void exitPartialMode() noexcept;
  void nextPartialPixel() noexcept;

  // --------------------------------------------------------------------------
  // [CMask (Any)]
  // --------------------------------------------------------------------------

  void cMaskInit(x86::Gp& m) noexcept;
  void cMaskInit(const x86::Mem& pMsk) noexcept;
  void cMaskFini() noexcept;

  void cMaskGenericLoop(x86::Gp& i) noexcept;
  void cMaskGranularLoop(x86::Gp& i) noexcept;
  void cMaskMemcpyOrMemsetLoop(x86::Gp& i) noexcept;

  void _cMaskLoopInit(uint32_t loopType) noexcept;
  void _cMaskLoopFini() noexcept;

  // --------------------------------------------------------------------------
  // [CMask (XMM)]
  // --------------------------------------------------------------------------

  void cMaskInitXmm(x86::Vec& m) noexcept;
  void cMaskFiniXmm() noexcept;

  void cMaskGenericLoopXmm(x86::Gp& i) noexcept;
  void cMaskGranularLoopXmm(x86::Gp& i) noexcept;

  void cMaskProc32Xmm1(PixelARGB& out, uint32_t flags) noexcept;
  void cMaskProc32Xmm4(PixelARGB& out, uint32_t flags) noexcept;
  void cMaskProc32Xmm8(PixelARGB& out, uint32_t flags) noexcept;
  void cMaskProc32XmmV(PixelARGB& out, uint32_t flags, uint32_t n) noexcept;

  // --------------------------------------------------------------------------
  // [VMask (Any)]
  // --------------------------------------------------------------------------

  void vMaskProc(PixelARGB& out, uint32_t flags, x86::Gp& m) noexcept;

  // --------------------------------------------------------------------------
  // [VMask (XMM)]
  // --------------------------------------------------------------------------

  void vMaskProc32Xmm1(PixelARGB& out, uint32_t flags, VecArray& mv, bool mImmutable) noexcept;
  void vMaskProc32Xmm4(PixelARGB& out, uint32_t flags, VecArray& mv, bool mImmutable) noexcept;
  void vMaskProc32XmmV(PixelARGB& out, uint32_t flags, VecArray& mv, uint32_t n, bool mImmutable) noexcept;

  void vMaskProc32InvertMask(VecArray& mi, VecArray& mv) noexcept;
  void vMaskProc32InvertDone(VecArray& mi, bool mImmutable) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _compOp;                      //!< Composition operator.
  uint8_t _cMaskLoopType;                //!< The current span mode.
  uint8_t _maxPixels;                    //!< Maximum pixels the compositor can handle at a time.
  uint8_t _pixelGranularity;             //!< Pixel granularity.
  uint8_t _minAlignment;                 //!< Minimum alignment required to process `_maxPixels`.

  uint8_t _hasDa : 1;                    //!< Whether the destination format has an alpha channel.
  uint8_t _hasSa : 1;                    //!< Whether the source format has an alpha channel.

  asmjit::BaseNode* _cMaskLoopHook;      //!< A hook that is used by the current loop.
  SolidPixelARGB _solidOpt;              //!< Optimized solid pixel for operators that allow it.
  PixelARGB _solidPre;                   //!< Pre-processed solid pixel for TypeA operators that always use `vMaskProc?()`.
  PixelARGB _pixPartial;                 //!< Partial fetch that happened at the end of the scanline (border case).
  Wrap<PipeCMask> _mask;                 //!< Const mask.
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_COMPOPPART_P_H
