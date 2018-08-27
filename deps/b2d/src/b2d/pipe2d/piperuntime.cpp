// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./compoppart_p.h"
#include "./fetchpart_p.h"
#include "./fetchsolidpart_p.h"
#include "./fillpart_p.h"
#include "./pipecompiler_p.h"
#include "./piperuntime_p.h"
#include "./pipesignature_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

Wrap<PipeRuntime> PipeRuntime::_global;

// ============================================================================
// [b2d::Pipe2D::PipeErrorHandler]
// ============================================================================

//! \internal
//!
//! JIT error handler that implements `asmjit::ErrorHandler` interface.
class PipeErrorHandler : public asmjit::ErrorHandler {
public:
  PipeErrorHandler() noexcept : _err(asmjit::kErrorOk) {}
  virtual ~PipeErrorHandler() noexcept {}

  void handleError(Error err, const char* message, asmjit::BaseEmitter* origin) override {
    B2D_UNUSED(origin);

    _err = err;

    char buf[1024];
    std::snprintf(buf, 1024, "[b2d] Pipe2D failed: %s\n", message);
    DebugUtils::debugOutput(buf);
  }

  asmjit::Error _err;
};

// ============================================================================
// [b2d::Pipe2D::PipeRuntime - Construction / Destruction]
// ============================================================================

PipeRuntime::PipeRuntime() noexcept
  : _runtime(),
    _cache(),
    _pipelineCount(0),
    _optLevel(0),
    _maxPixels(0) {
  #ifndef ASMJIT_DISABLE_LOGGING
  const uint32_t kFormatFlags = asmjit::FormatOptions::kFlagRegCasts      |
                                asmjit::FormatOptions::kFlagAnnotations   |
                                asmjit::FormatOptions::kFlagMachineCode   ;
  _logger.setFile(stderr);
  _logger.addFlags(kFormatFlags);
  _enableLogger = false;
  #endif
}
PipeRuntime::~PipeRuntime() noexcept {}

// ============================================================================
// [b2d::Pipe2D::PipeRuntime - Accessors]
// ============================================================================

FillFunc PipeRuntime::_compileAndStore(uint32_t signature) noexcept {
  FillFunc func = _compileFunction(signature);
  if (B2D_UNLIKELY(!func))
    return nullptr;

  if (B2D_UNLIKELY(_cache.put(signature, (void*)func) != kErrorOk)) {
    _runtime.release(func);
    return nullptr;
  }

  _pipelineCount++;
  return func;
}

FillFunc PipeRuntime::_compileFunction(uint32_t signature) noexcept {
  using namespace asmjit;
  JitRuntime& runtime = _runtime;

  PipeErrorHandler eh;
  CodeHolder code;

  code.init(runtime.codeInfo());
  code.setErrorHandler(&eh);

  #ifndef ASMJIT_DISABLE_LOGGING
  if (_enableLogger)
    code.setLogger(&_logger);
  #endif

  x86::Compiler cc(&code);
  PipeSignature sig = { signature };

  #ifndef ASMJIT_DISABLE_LOGGING
  if (_enableLogger) {
    // Dump a function signature, useful when logging is enabled.
    cc.commentf("Signature 0x%08X Dst{Fmt=%d} Src{Fmt=%d} Op{Fill=%d CompOp=%d} Fetch{Type=%d Extra=%d}",
      sig.value(),
      sig.dstPixelFormat(),
      sig.srcPixelFormat(),
      sig.fillType(),
      sig.compOp(),
      sig.fetchType(),
      sig.fetchExtra());
  }
  #endif

  // Construct the pipeline and compile it.
  {
    PipeCompiler pc(&cc);

    if (optLevel() && optLevel() < pc.optLevel())
      pc.setOptLevel(optLevel());

    // TODO: Limit MaxPixels.
    FetchPart* dstPart = pc.newFetchPart(kFetchTypePixelPtr, 0, sig.dstPixelFormat());
    FetchPart* srcPart = pc.newFetchPart(sig.fetchType(), sig.fetchExtra(), sig.srcPixelFormat());

    // TODO: Move to Pipe2D.
    if (sig.compOp() == CompOp::kClear)
      srcPart->as<FetchSolidPart>()->setTransparent(true);

    CompOpPart* compOpPart = pc.newCompOpPart(sig.compOp(), dstPart, srcPart);
    FillPart* fillPart = pc.newFillPart(sig.fillType(), dstPart, compOpPart);

    pc.beginFunction();
    pc.initPipeline(fillPart);
    fillPart->compile();
    pc.endFunction();
  }

  if (eh._err)
    return nullptr;

  if (cc.finalize() != kErrorOk)
    return nullptr;

  #ifndef ASMJIT_DISABLE_LOGGING
  if (_enableLogger)
    _logger.logf("[Pipeline size: %u bytes]\n\n", uint32_t(code.codeSize()));
  #endif

  FillFunc func = nullptr;
  runtime.add(&func, &code);
  return func;
}

B2D_END_SUB_NAMESPACE

// ============================================================================
// [b2d::Pipe2D - Runtime Handlers]
// ============================================================================

B2D_BEGIN_NAMESPACE

static void Pipe2DOnShutdown(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);
  Pipe2D::PipeRuntime::_global.destroy();
}

void Pipe2DOnInit(Runtime::Context& rt) noexcept {
  Pipe2D::PipeRuntime::_global.init();
  rt.addShutdownHandler(Pipe2DOnShutdown);
}

B2D_END_NAMESPACE
