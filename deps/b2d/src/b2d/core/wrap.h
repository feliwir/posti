// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_WRAP_H
#define _B2D_CORE_WRAP_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Wrap<T>
// ============================================================================

//! Wrapper to control construction & destruction of `T`.
template<typename T>
struct alignas(alignof(T)) Wrap {
  // --------------------------------------------------------------------------
  // [Init / Destroy]
  // --------------------------------------------------------------------------

  //! Placement new constructor.
  inline T* init() noexcept { return new(static_cast<void*>(_data)) T; }

  //! Placement new constructor with arguments.
  template<typename... ArgsT>
  inline T* init(ArgsT&&... args) noexcept { return new(static_cast<void*>(_data)) T(std::forward<ArgsT>(args)...); }

  //! Placement delete destructor.
  inline void destroy() noexcept { p()->~T(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline T* p() noexcept { return reinterpret_cast<T*>(this); }
  inline const T* p() const noexcept { return reinterpret_cast<const T*>(this); }

  inline operator T&() noexcept { return *p(); }
  inline operator const T&() const noexcept { return *p(); }

  inline T& operator()() noexcept { return *p(); }
  inline const T& operator()() const noexcept { return *p(); }

  inline T* operator&() noexcept { return p(); }
  inline const T* operator&() const noexcept { return p(); }

  inline T* operator->() noexcept { return p(); }
  inline T const* operator->() const noexcept { return p(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Storage required to instantiate `T`.
  uint8_t _data[sizeof(T)];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_WRAP_H
