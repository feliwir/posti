// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_RUNTIME_H
#define _B2D_CORE_RUNTIME_H

// [Dependencies]
#include "../core/globals.h"
#include "../core/wrap.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Runtime]
// ============================================================================

namespace Runtime {
  // --------------------------------------------------------------------------
  // [Initialization & Shutdown]
  // --------------------------------------------------------------------------

  //! Initialize Blend2D library.
  //!
  //! Blend2D calls `Runtime::init()` automatically when the library is loaded so
  //! there should be no need to call it manually. Initialization uses a counter
  //! so consecutive calls to `Runtime::init()` won't do anything if the Runtime
  //! has been already initialized.
  B2D_API void init() noexcept;

  //! Shut down Blend2D library.
  //!
  //! Blend2D calls `shutdown()` automatically when the library is unloaded so
  //! there should be no need to call it explicitly.
  B2D_API void shutdown() noexcept;

  #ifdef B2D_BUILD_STATIC
  // Hack to initialize the library statically (static builds only).
  B2D_API void initStatic() noexcept;
  #endif

  // --------------------------------------------------------------------------
  // [Cleanup]
  // --------------------------------------------------------------------------

  enum CleanupFlags : uint32_t {
    kCleanupZeroedMemory  = 0x00000001u, //!< Cleanup zeroed memory.
    kCleanupAll           = 0xFFFFFFFFu  //!< Cleanup everything.
  };

  B2D_API void cleanup(uint32_t cleanupFlags = kCleanupAll) noexcept;

  // --------------------------------------------------------------------------
  // [Build Info]
  // --------------------------------------------------------------------------

  //! Blend2D build type.
  enum BuildType : uint32_t {
    kBuildTypeDebug = 0,                 //!< Debug build.
    kBuildTypeRelease = 1                //!< Release build.
  };

  //! Blend2D build information.
  struct BuildInfo {
    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    //! Get Blend2D build type, see \ref BuildType.
    B2D_INLINE uint32_t buildType() const noexcept { return _buildType; }
    //! Get if Blend2D was built as debug.
    B2D_INLINE bool isDebugBuild() const noexcept { return _buildType != kBuildTypeRelease; }
    //! Get if Blend2D was built as release.
    B2D_INLINE bool isReleaseBuild() const noexcept { return _buildType == kBuildTypeRelease; }

    //! Get Blend2D major version.
    B2D_INLINE uint32_t majorVersion() const noexcept { return _majorVersion; }
    //! Get Blend2D minor version.
    B2D_INLINE uint32_t minorVersion() const noexcept { return _minorVersion; }
    //! Get Blend2D patch version.
    B2D_INLINE uint32_t patchVersion() const noexcept { return _patchVersion; }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    uint32_t _buildType;                 //!< Blend2D build type.
    uint16_t _majorVersion;              //!< Blend2D major version.
    uint8_t _minorVersion;               //!< Blend2D minor version.
    uint8_t _patchVersion;               //!< Blend2D patch version.
  };

  B2D_VARAPI const BuildInfo _buildInfo;
  static B2D_INLINE BuildInfo buildInfo() noexcept { return _buildInfo; }

  // --------------------------------------------------------------------------
  // [HWInfo]
  // --------------------------------------------------------------------------

  //! Blend2D optimization level.
  enum OptLevel {
    kOptLevel_None         = 0,          //!< Baseline optimizations.
    kOptLevel_X86_SSE2     = 1,          //!< SSE2+ optimization level (minimum on X86).
    kOptLevel_X86_SSE3     = 2,          //!< SSE3+ optimization level.
    kOptLevel_X86_SSSE3    = 3,          //!< SSSE3+ optimization level.
    kOptLevel_X86_SSE4_1   = 4,          //!< SSE4.1+ optimization level.
    kOptLevel_X86_SSE4_2   = 5,          //!< SSE4.2+ optimization level.
    kOptLevel_X86_AVX      = 6,          //!< AVX+ optimization level.
    kOptLevel_X86_AVX2     = 7           //!< AVX2+ optimization level.
  };

  //! Hardware information.
  struct HWInfo {
    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    //! Get number of hardware threads.
    B2D_INLINE uint32_t hwThreadCount() const noexcept { return _hwThreadCount; }

    //! Get the optimization level.
    B2D_INLINE uint32_t optLevel() const noexcept { return _optLevel; }

    //! Get whether the host CPU supports SSE2 (X86|X64 only).
    B2D_INLINE bool hasSSE2() const noexcept { return (B2D_ARCH_SSE2 != 0) || _optLevel >= kOptLevel_X86_SSE2; }
    //! Get whether the host CPU supports SSE3 (X86|X64 only).
    B2D_INLINE bool hasSSE3() const noexcept { return (B2D_ARCH_SSE3 != 0) || _optLevel >= kOptLevel_X86_SSE3; }
    //! Get whether the host CPU supports SSSE3 (X86|X64 only).
    B2D_INLINE bool hasSSSE3() const noexcept { return (B2D_ARCH_SSSE3 != 0) || _optLevel >= kOptLevel_X86_SSSE3; }
    //! Get whether the host CPU supports SSE4.1 (X86|X64 only).
    B2D_INLINE bool hasSSE4_1() const noexcept { return (B2D_ARCH_SSE4_1 != 0) || _optLevel >= kOptLevel_X86_SSE4_1; }
    //! Get whether the host CPU supports SSE4.2 (X86|X64 only).
    B2D_INLINE bool hasSSE4_2() const noexcept { return (B2D_ARCH_SSE4_2 != 0) || _optLevel >= kOptLevel_X86_SSE4_2; }
    //! Get whether the host CPU supports AVX (X86|X64 only).
    B2D_INLINE bool hasAVX() const noexcept { return (B2D_ARCH_AVX != 0) || _optLevel >= kOptLevel_X86_AVX; }
    //! Get whether the host CPU supports AVX2 (X86|X64 only).
    B2D_INLINE bool hasAVX2() const noexcept { return (B2D_ARCH_AVX2 != 0) || _optLevel >= kOptLevel_X86_AVX2; }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    uint32_t _hwThreadCount;             //!< Number of HW threads.
    uint32_t _optLevel;                  //!< Optimization level.
  };

  B2D_VARAPI HWInfo _hwInfo;
  static B2D_INLINE const HWInfo& hwInfo() noexcept { return _hwInfo; }

  // --------------------------------------------------------------------------
  // [MemoryInfo]
  // --------------------------------------------------------------------------

  struct MemoryInfo {
    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    B2D_INLINE size_t zeroedMemoryUsed() const noexcept { return _zeroedMemoryUsed; }
    B2D_INLINE size_t zeroedMemoryReserved() const noexcept { return _zeroedMemoryReserved; }
    B2D_INLINE size_t zeroedMemoryOverhead() const noexcept { return _zeroedMemoryOverhead; }
    B2D_INLINE size_t zeroedMemoryBlockCount() const noexcept { return _zeroedMemoryBlockCount; }

    B2D_INLINE size_t virtualMemoryUsed() const noexcept { return _virtualMemoryUsed; }
    B2D_INLINE size_t virtualMemoryReserved() const noexcept { return _virtualMemoryReserved; }
    B2D_INLINE size_t virtualMemoryOverhead() const noexcept { return _virtualMemoryOverhead; }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    size_t _zeroedMemoryUsed;
    size_t _zeroedMemoryReserved;
    size_t _zeroedMemoryOverhead;
    size_t _zeroedMemoryBlockCount;

    size_t _virtualMemoryUsed;
    size_t _virtualMemoryReserved;
    size_t _virtualMemoryOverhead;

    size_t _pipelineCount;
  };

  B2D_API MemoryInfo memoryInfo() noexcept;

  // --------------------------------------------------------------------------
  // [Context]
  // --------------------------------------------------------------------------

  // This functionality is internal to Blend2D. The context provides functionality
  // to manage shutdown and cleanup handlers and also provides basic information
  // about the current runtime that can be simply obtained by `Runtime::info()`.

  #ifdef B2D_EXPORTS
  //! \internal
  //!
  //! Runtime context used as an internal Runtime instance (singleton).
  class Context {
  public:
    //! Shutdown handler.
    typedef void (B2D_CDECL* ShutdownFunc)(Context& rt);
    //! Cleanup handler.
    typedef void (B2D_CDECL* CleanupFunc)(Context& rt, uint32_t cleanupFlags);
    //! MemoryInfo handler.
    typedef void (B2D_CDECL* MemoryInfoFunc)(Context& rt, MemoryInfo& memoryInfo);

    //! Fixed array used by handlers, all content should be zero initialized by
    //! linker as it's used only within a statically allocated `Context` instance.
    template<typename FuncT, size_t kCapacity>
    struct FixedArray {
      B2D_INLINE void reset() noexcept { size = 0; }

      B2D_INLINE void add(FuncT func) noexcept {
        B2D_ASSERT(size < kCapacity);
        data[size++] = func;
      }

      size_t size;
      FuncT data[kCapacity];
    };

    // NOTE: Cleanup and shutdown handlers are internal. There is only a limited
    // number of handlers that can be added to the context. The reason we do it
    // this way is that for builds of Blend2D that have conditionally disabled
    // some features it's easier to have only `OnInit()` handlers and let them
    // register cleanup/shutdown handlers when needed.

    B2D_INLINE void addShutdownHandler(ShutdownFunc func) noexcept { _shutdownHandlers.add(func); }
    B2D_INLINE void addCleanupHandler(CleanupFunc func) noexcept { _cleanupHandlers.add(func); }
    B2D_INLINE void addMemoryInfoHandler(MemoryInfoFunc func) noexcept { _memoryInfoHandlers.add(func); }

    //! Shutdown handlers (always traversed from last to first).
    FixedArray<ShutdownFunc, 8> _shutdownHandlers;
    //! Cleanup handlers (always executed from first to last).
    FixedArray<CleanupFunc, 8> _cleanupHandlers;
    //! MemoryInfo handlers (always traversed from first to last).
    FixedArray<MemoryInfoFunc, 8> _memoryInfoHandlers;
  };
  #endif
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_RUNTIME_H
