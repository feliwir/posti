// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_BUILD_H
#define _B2D_CORE_BUILD_H

// ============================================================================
// [b2d::Build - Globals - Dependencies]
// ============================================================================

// No deprecation warnings when building 'b2d' itself.
#if defined(B2D_EXPORTS) && defined(_MSC_VER)
  #if !defined(_CRT_SECURE_NO_DEPRECATE)
    #define _CRT_SECURE_NO_DEPRECATE
  #endif
  #if !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
  #endif
#endif

// Include <windows.h>
#if _WIN32
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
    #define B2D_UNDEF_WIN32_LEAN_AND_MEAN
  #endif
  #if !defined(NOMINMAX)
    #define NOMINMAX
    #define B2D_UNDEF_NOMINMAX
  #endif
  #include <windows.h>
  #if defined(B2D_UNDEF_NOMINMAX)
    #undef NOMINMAX
    #undef B2D_UNDEF_NOMINMAX
  #endif
  #if defined(B2D_UNDEF_WIN32_LEAN_AND_MEAN)
    #undef WIN32_LEAN_AND_MEAN
    #undef B2D_UNDEF_WIN32_LEAN_AND_MEAN
  #endif
#else
  // The FileSystem API works fully with 64-bit file sizes and offsets.
  #if B2D_OS_POSIX && !defined(_LARGEFILE64_SOURCE)
    #define _LARGEFILE64_SOURCE 1
  #endif

  // The FileSystem API supports extensions offered by Linux.
  #if B2D_OS_LINUX && !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
  #endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
  #include <algorithm>
  #include <atomic>
  #include <cmath>
  #include <cstdarg>
  #include <cstdio>
  #include <cstdlib>
  #include <cstring>
  #include <limits>
  #include <new>
  #include <type_traits>
  #include <utility>
#endif

// ============================================================================
// [b2d::Build - Globals - Version]
// ============================================================================

#define B2D_LIBRARY_VERSION 0x010000

// ============================================================================
// [b2d::Build - Globals - Static Build and Embedding]
// ============================================================================

// These definitions can be used to enable static library build. Embed is used
// when Blend2D's source code is embedded directly in another project, implies
// static build as well.
//
// #define B2D_BUILD_EMBED           // Blend2D is embedded (implies B2D_BUILD_STATIC).
// #define B2D_BUILD_STATIC          // Define to enable static-library build.

// ============================================================================
// [b2d::Build - Globals - Build Mode]
// ============================================================================

// These definitions control the build mode and tracing support. The build mode
// should be auto-detected at compile time, but it's possible to override it in
// case that the auto-detection fails.
//
// Tracing is a feature that is never compiled by default and it's only used to
// debug Blend2D itself.
//
// #define B2D_BUILD_DEBUG           // Define to enable debug-mode.
// #define B2D_BUILD_RELEASE         // Define to enable release-mode.

// Detect B2D_BUILD_DEBUG and B2D_BUILD_RELEASE if not defined.
#if !defined(B2D_BUILD_DEBUG) && !defined(B2D_BUILD_RELEASE)
  #if !defined(NDEBUG)
    #define B2D_BUILD_DEBUG
  #else
    #define B2D_BUILD_RELEASE
  #endif
#endif

// ============================================================================
// [b2d::Build - Globals - Tracing]
// ============================================================================

// Blend2D provides traces that can be enabled during development. Traces can
// help understanding how certain things work and can be used to track bugs

// #define B2D_TRACE_OT              // Trace all OpenType features.
// #define B2D_TRACE_OT_CFF          // Trace OpenType CFF|CFF2 implementations.
// #define B2D_TRACE_OT_KERN         // Trace OpenType KERN implementation.
// #define B2D_TRACE_OT_LAYOUT       // Trace OpenType layout implementation (GDEF, GPOS, GSUB).
// #define B2D_TRACE_OT_NAME         // Trace OpenType NAME implementation.

// ============================================================================
// [b2d::Build - Globals - Optimizations]
// ============================================================================

// Build optimizations control compile-time optimizations to be built-in. These
// optimizations are not related to the code-generator optimizations (JIT) that
// are always auto-detected at runtime.
//
// #define B2D_BUILD_SSE2            // Build SSE2 support  (default  on X86 & X64).
// #define B2D_BUILD_SSE3            // Build SSE3 support  (optional on X86 & X64).
// #define B2D_BUILD_SSSE3           // Build SSSE3 support (optional on X86 & X64).

// ============================================================================
// [b2d::Build - Globals - Target Operating System]
// ============================================================================

#if defined(_WIN32)
  #define B2D_OS_WINDOWS   1
#else
  #define B2D_OS_WINDOWS   0
#endif

#if defined(__linux__) || defined(__ANDROID__)
  #define B2D_OS_LINUX     1
#else
  #define B2D_OS_LINUX     0
#endif

#if defined(__ANDROID__)
  #define B2D_OS_ANDROID   1
#else
  #define B2D_OS_ANDROID   0
#endif

#if defined(__APPLE__)
  #define B2D_OS_MAC       1
#else
  #define B2D_OS_MAC       0
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)    || \
    defined(__OpenBSD__) || defined(__DragonFly__) || \
    defined(__bsdi__)
  #define B2D_OS_BSD       1
#else
  #define B2D_OS_BSD       0
#endif

#define B2D_OS_POSIX       (!B2D_OS_WINDOWS)

// ============================================================================
// [b2d::Build - Globals - C++ Compiler and Features Detection]
// ============================================================================

#define B2D_CXX_CLANG      0
#define B2D_CXX_INTEL      0
#define B2D_CXX_GNU_ONLY   0
#define B2D_CXX_MSC_ONLY   0
#define B2D_CXX_VER(MAJOR, MINOR, PATCH) ((MAJOR) * 10000000 + (MINOR) * 100000 + (PATCH))

#if defined(__INTEL_COMPILER)
  // Intel compiler pretends to be GNU or MSC, so it must be checked first.
  //   https://software.intel.com/en-us/articles/c0x-features-supported-by-intel-c-compiler
  //   https://software.intel.com/en-us/articles/c14-features-supported-by-intel-c-compiler
  //   https://software.intel.com/en-us/articles/c17-features-supported-by-intel-c-compiler
  #undef B2D_CXX_INTEL
  #define B2D_CXX_INTEL B2D_CXX_VER(__INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) % 10, __INTEL_COMPILER % 10)
#elif defined(_MSC_VER) && defined(_MSC_FULL_VER)
  // MSC compiler:
  //   https://msdn.microsoft.com/en-us/library/hh567368.aspx
  //
  // Version List:
  //   16.00.0 == VS2010
  //   17.00.0 == VS2012
  //   18.00.0 == VS2013
  //   19.00.0 == VS2015
  //   19.10.0 == VS2017
  #undef B2D_CXX_MSC_ONLY
  #if _MSC_VER == _MSC_FULL_VER / 10000
    #define B2D_CXX_MSC_ONLY B2D_CXX_VER(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000)
  #else
    #define B2D_CXX_MSC_ONLY B2D_CXX_VER(_MSC_VER / 100, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000)
  #endif
#elif defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
  // Clang compiler:
  #undef B2D_CXX_CLANG
  #define B2D_CXX_CLANG B2D_CXX_VER(__clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
  // GNU compiler:
  //   https://gcc.gnu.org/projects/cxx-status.html
  #undef B2D_CXX_GNU_ONLY
  #define B2D_CXX_GNU_ONLY B2D_CXX_VER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#endif

// GNU [Compatibility] - GNU compiler or compatible.
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
  #define B2D_CXX_GNU B2D_CXX_VER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
  #define B2D_CXX_GNU B2D_CXX_GNU_ONLY
#endif

// MSC [Compatibility] - MSC compiler or compatible.
#if !B2D_CXX_MSC_ONLY && defined(_MSC_VER)
  #define B2D_CXX_MSC B2D_CXX_VER(_MSC_VER / 100, _MSC_VER % 100, 0)
#else
  #define B2D_CXX_MSC B2D_CXX_MSC_ONLY
#endif

// Compiler features detection macros.
#if B2D_CXX_CLANG && defined(__has_builtin)
  #define B2D_CXX_HAS_BUILTIN(NAME, CHECK) (__has_builtin(NAME))
#else
  #define B2D_CXX_HAS_BUILTIN(NAME, CHECK) (!(!(CHECK)))
#endif

#if B2D_CXX_CLANG && defined(__has_extension)
  #define B2D_CXX_HAS_FEATURE(NAME, CHECK) (__has_extension(NAME))
#elif B2D_CXX_CLANG && defined(__has_feature)
  #define B2D_CXX_HAS_FEATURE(NAME, CHECK) (__has_feature(NAME))
#else
  #define B2D_CXX_HAS_FEATURE(NAME, CHECK) (!(!(CHECK)))
#endif

#if B2D_CXX_CLANG && defined(__has_attribute)
  #define B2D_CXX_HAS_ATTRIBUTE(NAME, CHECK) (__has_attribute(NAME))
#else
  #define B2D_CXX_HAS_ATTRIBUTE(NAME, CHECK) (!(!(CHECK)))
#endif

#if B2D_CXX_CLANG && defined(__has_cpp_attribute)
  #define B2D_CXX_HAS_CPP_ATTRIBUTE(NAME, CHECK) (__has_cpp_attribute(NAME))
#else
  #define B2D_CXX_HAS_CPP_ATTRIBUTE(NAME, CHECK) (!(!(CHECK)))
#endif

// Compiler features by vendor.
#if defined(B2D_CXX_MSC_ONLY) && !defined(_NATIVE_WCHAR_T_DEFINED)
  #define B2D_CXX_HAS_NATIVE_WCHAR_T 0
#else
  #define B2D_CXX_HAS_NATIVE_WCHAR_T 1
#endif

#define B2D_CXX_HAS_UNICODE_LITERALS \
  B2D_CXX_HAS_FEATURE(__cxx_unicode_literals__, ( \
                      (B2D_CXX_INTEL    >= B2D_CXX_VER(14, 0, 0)) || \
                      (B2D_CXX_MSC_ONLY >= B2D_CXX_VER(19, 0, 0)) || \
                      (B2D_CXX_GNU_ONLY >= B2D_CXX_VER(4 , 5, 0) && __cplusplus >= 201103L) ))

// ============================================================================
// [b2d::Build - Globals - Target Architecture]
// ============================================================================

#if defined(_M_X64) || defined(__amd64) || defined(__x86_64) || defined(__x86_64__)
  #define B2D_ARCH_X86     64
#elif defined(_M_IX86) || defined(__i386) || defined(__i386__)
  #define B2D_ARCH_X86     32
#else
  #define B2D_ARCH_X86     0
#endif

#if defined(__ARM64__) || defined(__aarch64__)
  #define B2D_ARCH_ARM     64
#elif (defined(_M_ARM  ) || defined(__arm    ) || defined(__thumb__ ) || \
       defined(_M_ARMT ) || defined(__arm__  ) || defined(__thumb2__))
  #define B2D_ARCH_ARM     32
#else
  #define B2D_ARCH_ARM     0
#endif

#if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
  #define B2D_ARCH_MIPS    64
#elif defined(_MIPS_ARCH_MIPS32) || defined(_M_MRX000) || defined(__mips) || defined(__mips__)
  #define B2D_ARCH_MIPS    32
#else
  #define B2D_ARCH_MIPS    0
#endif

#define B2D_ARCH_BITS (B2D_ARCH_X86 | B2D_ARCH_ARM | B2D_ARCH_MIPS)
#if B2D_ARCH_BITS == 0
  #undef B2D_ARCH_BITS
  #if defined (__LP64__) || defined(_LP64)
    #define B2D_ARCH_BITS  64
  #else
    #define B2D_ARCH_BITS  32
  #endif
#endif

#if (defined(__ARMEB__)) || \
    (defined(__MIPSEB__)) || \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
  #define B2D_ARCH_LE      0
  #define B2D_ARCH_BE      1
#else
  #define B2D_ARCH_LE      1
  #define B2D_ARCH_BE      0
#endif

#ifdef __AVX2__
  #define B2D_ARCH_AVX2    1
#else
  #define B2D_ARCH_AVX2    0
#endif

#ifdef __AVX__
  #define B2D_ARCH_AVX     1
#else
  #define B2D_ARCH_AVX     B2D_ARCH_AVX2
#endif

#ifdef __SSE4_2__
  #define B2D_ARCH_SSE4_2  1
#else
  #define B2D_ARCH_SSE4_2  B2D_ARCH_AVX
#endif

#ifdef __SSE4_1__
  #define B2D_ARCH_SSE4_1  1
#else
  #define B2D_ARCH_SSE4_1  B2D_ARCH_AVX
#endif

#ifdef __SSSE3__
  #define B2D_ARCH_SSSE3   1
#else
  #define B2D_ARCH_SSSE3   B2D_ARCH_SSE4_1
#endif

#ifdef __SSE3__
  #define B2D_ARCH_SSE3    1
#else
  #define B2D_ARCH_SSE3    B2D_ARCH_SSSE3
#endif

#if !defined(B2D_ARCH_SSE2) && (B2D_ARCH_X86 == 64 || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
  #define B2D_ARCH_SSE2    1
#else
  #define B2D_ARCH_SSE2    B2D_ARCH_SSE3
#endif

#if !defined(B2D_ARCH_SSE) && (B2D_ARCH_X86 == 64 || defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))
  #define B2D_ARCH_SSE     1
#else
  #define B2D_ARCH_SSE     B2D_ARCH_SSE2
#endif

#if B2D_ARCH_ARM && defined(__ARM_NEON__)
  #define B2D_ARCH_NEON    1
#else
  #define B2D_ARCH_NEON    0
#endif

#ifdef B2D_BUILD_AVX2
  #define B2D_MAYBE_AVX2   1
#else
  #define B2D_MAYBE_AVX2   0
#endif

#ifdef B2D_BUILD_AVX
  #define B2D_MAYBE_AVX    1
#else
  #define B2D_MAYBE_AVX    B2D_MAYBE_AVX2
#endif

#ifdef B2D_BUILD_SSE4_2
  #define B2D_MAYBE_SSE4_2 1
#else
  #define B2D_MAYBE_SSE4_2 B2D_MAYBE_AVX
#endif

#ifdef B2D_BUILD_SSE4_1
  #define B2D_MAYBE_SSE4_1 1
#else
  #define B2D_MAYBE_SSE4_1 B2D_MAYBE_SSE4_2
#endif

#ifdef B2D_BUILD_SSSE3
  #define B2D_MAYBE_SSSE3  1
#else
  #define B2D_MAYBE_SSSE3  B2D_MAYBE_SSE4_1
#endif

#ifdef B2D_BUILD_SSE3
  #define B2D_MAYBE_SSE3   1
#else
  #define B2D_MAYBE_SSE3   B2D_MAYBE_SSSE3
#endif

#ifdef B2D_BUILD_SSE2
  #define B2D_MAYBE_SSE2   1
#else
  #define B2D_MAYBE_SSE2   B2D_MAYBE_SSE3
#endif

#if B2D_CXX_MSC
  #include <intrin.h>
#endif

#if B2D_ARCH_SSE
  #include <xmmintrin.h>
#endif

#if B2D_ARCH_SSE2
  #include <emmintrin.h>
#endif

#if B2D_ARCH_SSE3 && !B2D_CXX_MSC
  #include <pmmintrin.h>
#endif

#if B2D_ARCH_SSSE3
  #include <tmmintrin.h>
#endif

#if B2D_ARCH_SSE4_1
  #include <smmintrin.h>
#endif

#if B2D_ARCH_SSE4_2
  #include <nmmintrin.h>
#endif

#if B2D_ARCH_AVX || B2D_ARCH_AVX2
  #include <immintrin.h>
#endif

#if B2D_ARCH_NEON
  #include <arm_neon.h>
#endif

// ============================================================================
// [b2d::Build - Globals - API Decorators & Language Extensions]
// ============================================================================

// B2D_BUILD_EMBED implies B2D_BUILD_STATIC.
#if defined(B2D_BUILD_EMBED) && !defined(B2D_BUILD_STATIC)
  #define B2D_BUILD_STATIC
#endif

// API (Export / Import).
#if !defined(B2D_API) && !defined(B2D_BUILD_STATIC)
  #if B2D_OS_WINDOWS && (B2D_CXX_MSC || defined(__MINGW32__))
    #if defined(B2D_EXPORTS)
      #define B2D_API __declspec(dllexport)
    #else
      #define B2D_API __declspec(dllimport)
    #endif
  #elif B2D_OS_WINDOWS && B2D_CXX_GNU
    #if defined(B2D_EXPORTS)
      #define B2D_API __attribute__((__dllexport__))
    #else
      #define B2D_API __attribute__((__dllimport__))
    #endif
  #elif B2D_CXX_GNU >= B2D_CXX_VER(4, 0, 0)
    #define B2D_API __attribute__((__visibility__("default")))
  #endif
#endif

#if !defined(B2D_API)
  #define B2D_API
#endif

#if !defined(B2D_VARAPI)
  #define B2D_VARAPI extern B2D_API
#endif

// This is basically a workaround. When using MSVC and marking class as DLL
// export everything gets exported, which is unwanted in most projects. MSVC
// automatically exports typeinfo and vtable if at least one symbol of the
// class is exported. However, GCC has some strange behavior that even if
// one or more symbol is exported it doesn't export typeinfo unless the
// class itself is decorated with "visibility(default)" (i.e. B2D_API).
#if B2D_CXX_GNU && !B2D_OS_WINDOWS
  #define B2D_VIRTAPI B2D_API
#else
  #define B2D_VIRTAPI
#endif

// Function attributes.
#if (B2D_CXX_GNU >= B2D_CXX_VER(4, 4, 0) && !defined(__MINGW32__))
  #define B2D_INLINE inline __attribute__((__always_inline__))
#elif B2D_CXX_MSC
  #define B2D_INLINE __forceinline
#else
  #define B2D_INLINE inline
#endif

#if B2D_CXX_GNU
  #define B2D_NOINLINE __attribute__((__noinline__))
  #define B2D_NORETURN __attribute__((__noreturn__))
#elif B2D_CXX_MSC
  #define B2D_NOINLINE __declspec(noinline)
  #define B2D_NORETURN __declspec(noreturn)
#else
  #define B2D_NOINLINE
  #define B2D_NORETURN
#endif

// Calling conventions.
#if B2D_ARCH_X86 == 32 && B2D_CXX_GNU
  #define B2D_CDECL __attribute__((__cdecl__))
#elif B2D_ARCH_X86 == 32 && B2D_CXX_MSC
  #define B2D_CDECL __cdecl
#else
  #define B2D_CDECL
#endif

// Internal macros that are only used when building 'b2d' itself.
#if defined(B2D_EXPORTS)
  #if !defined(B2D_BUILD_DEBUG) && B2D_CXX_HAS_ATTRIBUTE(__optimize__, B2D_CXX_GNU >= B2D_CXX_VER(4, 4, 0))
    #define B2D_FAVOR_SIZE  __attribute__((__optimize__("Os")))
    #define B2D_FAVOR_SPEED __attribute__((__optimize__("O3")))
  #else
    #define B2D_FAVOR_SIZE
    #define B2D_FAVOR_SPEED
  #endif
#endif

// Type alignment (not allowed by C++11 'alignas' keyword).
#if B2D_CXX_GNU
  #define B2D_ALIGN_TYPE(TYPE, N) __attribute__((__aligned__(N))) TYPE
#elif B2D_CXX_MSC
  #define B2D_ALIGN_TYPE(TYPE, N) __declspec(align(N)) TYPE
#else
  #define B2D_ALIGN_TYPE(TYPE, N) TYPE
#endif

// Annotations.
#if B2D_CXX_CLANG || B2D_CXX_GNU
  #define B2D_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
  #define B2D_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
  #define B2D_LIKELY(...) (__VA_ARGS__)
  #define B2D_UNLIKELY(...) (__VA_ARGS__)
#endif

#if B2D_CXX_CLANG && __cplusplus >= 201103L
  #define B2D_FALLTHROUGH [[clang::fallthrough]]
#elif B2D_CXX_GNU_ONLY >= B2D_CXX_VER(7, 0, 0)
  #define B2D_FALLTHROUGH __attribute__((__fallthrough__))
#else
  #define B2D_FALLTHROUGH ((void)0) /* fallthrough */
#endif

#define B2D_UNUSED(X) (void)(X)

// Utilities.
#define B2D_OFFSET_OF(STRUCT, MEMBER) ((int)(intptr_t)((const char*)&((const STRUCT*)0x1)->MEMBER) - 1)
#define B2D_ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))

#if B2D_CXX_GNU
  #define B2D_MACRO_BEGIN ({
  #define B2D_MACRO_END })
#else
  #define B2D_MACRO_BEGIN do {
  #define B2D_MACRO_END } while (0)
#endif

#if B2D_CXX_MSC
  #define B2D_ASSUME(...) __assume(__VA_ARGS__)
#elif B2D_CXX_HAS_BUILTIN(__builtin_assume, 0)
  #define B2D_ASSUME(...) __builtin_assume(__VA_ARGS__)
#elif B2D_CXX_HAS_BUILTIN(__builtin_unreachable, B2D_CXX_GNU >= B2D_CXX_VER(4, 5, 0))
  #define B2D_ASSUME(...) do { if (!(__VA_ARGS__)) __builtin_unreachable(); } while (0)
#else
  #define B2D_ASSUME(...) ((void)0)
#endif

// ============================================================================
// [b2d::Build - Globals - Begin-Namespace / End-Namespace]
// ============================================================================

#if B2D_CXX_GNU_ONLY
  #define B2D_BEGIN_NAMESPACE                                                 \
  namespace b2d {                                                             \
    _Pragma("GCC diagnostic push")                                            \
    _Pragma("GCC diagnostic ignored \"-Wenum-compare\"")                      \
    _Pragma("GCC diagnostic warning \"-Winline\"")
  #define B2D_END_NAMESPACE                                                   \
    _Pragma("GCC diagnostic pop")                                             \
  }
#elif B2D_CXX_MSC_ONLY
  #define B2D_BEGIN_NAMESPACE                                                 \
  namespace b2d {                                                             \
    __pragma(warning(push))                                                   \
    __pragma(warning(disable: 4102)) /* unreferenced label                 */ \
    __pragma(warning(disable: 4127)) /* conditional expression is constant */ \
    __pragma(warning(disable: 4201)) /* nameless struct/union              */ \
    __pragma(warning(disable: 4127)) /* conditional expression is constant */ \
    __pragma(warning(disable: 4251)) /* struct needs to have dll-interface */ \
    __pragma(warning(disable: 4275)) /* non dll-interface struct ... used  */ \
    __pragma(warning(disable: 4355)) /* this used in base member-init list */ \
    __pragma(warning(disable: 4480)) /* specifying underlying type for enum*/ \
    __pragma(warning(disable: 4800)) /* forcing value to bool true or false*/
  #define B2D_END_NAMESPACE                                                   \
    __pragma(warning(pop))                                                    \
  }
#elif B2D_CXX_CLANG
  #define B2D_BEGIN_NAMESPACE                                                 \
  namespace b2d {                                                             \
    _Pragma("clang diagnostic push")                                          \
    _Pragma("clang diagnostic ignored \"-Wconstant-logical-operand\"")        \
    _Pragma("clang diagnostic ignored \"-Wunnamed-type-template-args\"")
  #define B2D_END_NAMESPACE                                                   \
    _Pragma("clang diagnostic pop")                                           \
  }
#else
  #define B2D_BEGIN_NAMESPACE namespace b2d {
  #define B2D_END_NAMESPACE }
#endif

#define B2D_BEGIN_SUB_NAMESPACE(NAMESPACE)                                    \
  B2D_BEGIN_NAMESPACE                                                         \
  namespace NAMESPACE {

#define B2D_END_SUB_NAMESPACE                                                 \
  }                                                                           \
  B2D_END_NAMESPACE

// TODO: Deprecated, remove...
#define B2D_STATIC_INLINE static B2D_INLINE

#define B2D_SIZEOF_ALIGNED(x, y) ((sizeof(x) + y - 1) & ~(size_t)(y - 1))

// ============================================================================
// [b2d::Build - Globals - Utilities]
// ============================================================================

#define B2D_NONCOPYABLE(...)                                                  \
private:                                                                      \
  __VA_ARGS__(const __VA_ARGS__& other) = delete;                             \
  __VA_ARGS__& operator=(const __VA_ARGS__& other) = delete;                  \
public:

// ============================================================================
// [b2d::Build - Globals - Build-Only]
// ============================================================================

// Internal macros that are only used when building 'b2d' itself.
#if defined(B2D_EXPORTS)
  #if !defined(B2D_BUILD_DEBUG) && B2D_CXX_HAS_ATTRIBUTE(__optimize__, B2D_CXX_GNU >= B2D_CXX_VER(4, 4, 0))
    #define B2D_FAVOR_SIZE  __attribute__((__optimize__("Os")))
    #define B2D_FAVOR_SPEED __attribute__((__optimize__("O3")))
  #else
    #define B2D_FAVOR_SIZE
    #define B2D_FAVOR_SPEED
  #endif

  // Only turn-off these warnings when building 'b2d' itself.
  #if defined(_MSC_VER)
    #if !defined(_CRT_SECURE_NO_DEPRECATE)
      #define _CRT_SECURE_NO_DEPRECATE
    #endif
    #if !defined(_CRT_SECURE_NO_WARNINGS)
      #define _CRT_SECURE_NO_WARNINGS
    #endif
  #endif

  // The FileSystem API works fully with 64-bit file sizes and offsets.
  #if B2D_OS_POSIX && !defined(_LARGEFILE64_SOURCE)
    #define _LARGEFILE64_SOURCE 1
  #endif

  // The FileSystem API supports extensions offered by Linux.
  #if B2D_OS_LINUX && !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
  #endif
#endif

// ============================================================================
// [b2d::Build - Globals - Unit Testing Boilerplate]
// ============================================================================

// IDE: Make sure '#ifdef'ed unit tests are not disabled by IDE.
#if defined(__INTELLISENSE__) && !defined(B2D_BUILD_TEST)
  #define B2D_BUILD_TEST
#endif

// Include a unit testing package if this is a `b2d_test_unit` build.
#if defined(B2D_BUILD_TEST)
  #include "../../../test/broken.h"
#endif

// [Guard]
#endif // _B2D_CORE_BUILD_H
