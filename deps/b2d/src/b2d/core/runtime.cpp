// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/runtime.h"

#include <asmjit/asmjit.h>

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Runtime - Globals]
// ============================================================================

//! \internal
//!
//! Get how many times `Runtime::init()` has been called.
//!
//! Returns the current initialization count, which is incremented every
//! time a `Runtime::init()` is called and decremented every time a
//! `Runtime::shutdown()` is called.
//!
//! When this counter is incremented from 0 to 1 the library is initialized,
//! when it's decremented to zero it will free all resources and it will no
//! longer be safe to use.
static std::atomic<size_t> gRuntimeInitCount;

//! \internal
//!
//! Runtime global context.
static Runtime::Context gRuntimeContext;

// ============================================================================
// [b2d::Runtime - Initialization & Shutdown]
// ============================================================================

static B2D_INLINE void RuntimeOnInitHWInfo(Runtime::Context& rt) noexcept;

void AllocatorOnInit(Runtime::Context& rt) noexcept;
void ZeroAllocatorOnInit(Runtime::Context& rt) noexcept;
void ArrayOnInit(Runtime::Context& rt) noexcept;
void BufferOnInit(Runtime::Context& rt) noexcept;
void Matrix2DOnInit(Runtime::Context& rt) noexcept;
void RegionOnInit(Runtime::Context& rt) noexcept;
void Path2DOnInit(Runtime::Context& rt) noexcept;
void ImageOnInit(Runtime::Context& rt) noexcept;
void ImageScalerOnInit(Runtime::Context& rt) noexcept;
void ImageCodecOnInit(Runtime::Context& rt) noexcept;
void PatternOnInit(Runtime::Context& rt) noexcept;
void GradientOnInit(Runtime::Context& rt) noexcept;

void FontDataOnInit(Runtime::Context& rt) noexcept;
void FontLoaderOnInit(Runtime::Context& rt) noexcept;
void FontMatcherOnInit(Runtime::Context& rt) noexcept;
void FontCollectionOnInit(Runtime::Context& rt) noexcept;
void FontFaceOnInit(Runtime::Context& rt) noexcept;
void FontOnInit(Runtime::Context& rt) noexcept;

void Pipe2DOnInit(Runtime::Context& rt) noexcept;
void Context2DOnInit(Runtime::Context& rt) noexcept;

void Runtime::init() noexcept {
  Context& rt = gRuntimeContext;
  if (gRuntimeInitCount.fetch_add(1, std::memory_order_relaxed) != 0)
    return;

  // Setup HW information first so we can call other initializers.
  RuntimeOnInitHWInfo(rt);

  // Initialization handlers.
  AllocatorOnInit(rt);
  ZeroAllocatorOnInit(rt);
  ArrayOnInit(rt);
  BufferOnInit(rt);
  Matrix2DOnInit(rt);
  RegionOnInit(rt);
  Path2DOnInit(rt);
  ImageOnInit(rt);
  ImageScalerOnInit(rt);
  ImageCodecOnInit(rt);
  GradientOnInit(rt);
  PatternOnInit(rt);

  FontDataOnInit(rt);
  FontLoaderOnInit(rt);
  FontFaceOnInit(rt);
  FontMatcherOnInit(rt);
  FontCollectionOnInit(rt);
  FontOnInit(rt);

  Pipe2DOnInit(rt);
  Context2DOnInit(rt);
}

void Runtime::shutdown() noexcept {
  Context& rt = gRuntimeContext;
  if (gRuntimeInitCount.fetch_sub(1, std::memory_order_acq_rel) != 1)
    return;

  size_t i = rt._shutdownHandlers.size;
  while (i) {
    Context::ShutdownFunc func = rt._shutdownHandlers.data[--i];
    func(rt);
  }
  rt._shutdownHandlers.reset();
}

// Static instance of class that will call Runtime::init() and Runtime::shutdown().
class RuntimeAutoInit {
public:
  RuntimeAutoInit() noexcept { Runtime::init(); }
  ~RuntimeAutoInit() noexcept { Runtime::shutdown(); }
};
static RuntimeAutoInit gRuntimeAutoInit;

#ifdef B2D_BUILD_STATIC
void Runtime::initStatic() noexcept {}
#endif

// ============================================================================
// [b2d::Runtime - Cleanup]
// ============================================================================

void Runtime::cleanup(uint32_t cleanupFlags) noexcept {
  Context& rt = gRuntimeContext;
  size_t count = rt._cleanupHandlers.size;

  for (size_t i = 0; i < count; i++) {
    Context::CleanupFunc func = rt._cleanupHandlers.data[i];
    func(rt, cleanupFlags);
  }
}

// ============================================================================
// [b2d::Runtime - Build Info]
// ============================================================================

const Runtime::BuildInfo Runtime::_buildInfo = {
  // Blend2D build type [Debug|Release].
  #ifdef B2D_BUILD_DEBUG
  Runtime::kBuildTypeDebug,
  #else
  Runtime::kBuildTypeRelease,
  #endif

  // Blend2D version [Major.Minor.Patch].
  uint16_t(B2D_LIBRARY_VERSION >> 16),
  uint8_t((B2D_LIBRARY_VERSION >>  8) & 0xFFu),
  uint8_t((B2D_LIBRARY_VERSION >>  0) & 0xFFu)
};

// ============================================================================
// [b2d::Runtime - Hardware Info]
// ============================================================================

Runtime::HWInfo Runtime::_hwInfo;

static B2D_INLINE uint32_t Runtime_detectOptLevel(const asmjit::CpuInfo& cpuInfo) noexcept {
  #if B2D_ARCH_X86
  if (cpuInfo.hasFeature(asmjit::x86::Features::kAVX2  )) return Runtime::kOptLevel_X86_AVX2;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kAVX   )) return Runtime::kOptLevel_X86_AVX;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kSSE4_2)) return Runtime::kOptLevel_X86_SSE4_2;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kSSE4_1)) return Runtime::kOptLevel_X86_SSE4_1;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kSSSE3 )) return Runtime::kOptLevel_X86_SSSE3;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kSSE3  )) return Runtime::kOptLevel_X86_SSE3;
  if (cpuInfo.hasFeature(asmjit::x86::Features::kSSE2  )) return Runtime::kOptLevel_X86_SSE2;
  #endif

  return Runtime::kOptLevel_None;
}

static B2D_INLINE void RuntimeOnInitHWInfo(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);

  Runtime::HWInfo& hwInfo = Runtime::_hwInfo;
  const asmjit::CpuInfo& cpuInfo = asmjit::CpuInfo::host();

  hwInfo._hwThreadCount = cpuInfo.hwThreadCount();
  hwInfo._optLevel = Runtime_detectOptLevel(cpuInfo);
}

// ============================================================================
// [b2d::Runtime - Memory Info]
// ============================================================================

Runtime::MemoryInfo Runtime::memoryInfo() noexcept {
  Context& rt = gRuntimeContext;
  size_t count = rt._cleanupHandlers.size;

  MemoryInfo memoryInfo {};
  for (size_t i = 0; i < count; i++) {
    Context::MemoryInfoFunc func = rt._memoryInfoHandlers.data[i];
    func(rt, memoryInfo);
  }
  return memoryInfo;
}

B2D_END_NAMESPACE
