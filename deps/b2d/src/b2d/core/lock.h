// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_LOCK_H
#define _B2D_CORE_LOCK_H

// [Dependencies]
#include "../core/globals.h"

#if B2D_OS_POSIX
#include <pthread.h>
#endif // B2D_OS_POSIX

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Lock]
// ============================================================================

//! Lock.
class Lock {
public:
  B2D_NONCOPYABLE(Lock)

  // --------------------------------------------------------------------------
  // [Windows]
  // --------------------------------------------------------------------------

  #if B2D_OS_WINDOWS
  typedef CRITICAL_SECTION Native;
  Native _native;

  B2D_INLINE Lock() noexcept { ::InitializeCriticalSection(&_native); }
  B2D_INLINE ~Lock() noexcept { ::DeleteCriticalSection(&_native); }

  B2D_INLINE void lock() noexcept { ::EnterCriticalSection(&_native); }
  B2D_INLINE bool tryLock() noexcept { return ::TryEnterCriticalSection(&_native) != 0; }
  B2D_INLINE void unlock() noexcept { ::LeaveCriticalSection(&_native); }
  #endif

  // --------------------------------------------------------------------------
  // [Posix]
  // --------------------------------------------------------------------------

  #if B2D_OS_POSIX
  typedef pthread_mutex_t Native;

  #if defined(PTHREAD_MUTEX_INITIALIZER)
  Native _native = PTHREAD_MUTEX_INITIALIZER;
  B2D_INLINE Lock() noexcept = default;
  B2D_INLINE ~Lock() noexcept { ::pthread_mutex_destroy(&_native); }
  #else
  Native _native;
  B2D_INLINE Lock() noexcept { ::pthread_mutex_init(&_native, nullptr); }
  B2D_INLINE ~Lock() noexcept { ::pthread_mutex_destroy(&_native); }
  #endif

  B2D_INLINE void lock() noexcept { ::pthread_mutex_lock(&_native); }
  B2D_INLINE bool tryLock() noexcept { return ::pthread_mutex_trylock(&_native) == 0; }
  B2D_INLINE void unlock() noexcept { ::pthread_mutex_unlock(&_native); }
  #endif
};

// ============================================================================
// [b2d::ScopedLock]
// ============================================================================

//! Scoped lock.
struct ScopedLock {
public:
  B2D_NONCOPYABLE(ScopedLock)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Creates an instance of `ScopedLock` and locks the given `lock`.
  B2D_INLINE ScopedLock(Lock& lock) noexcept {
    _lock = &lock;
    _lock->lock();
  }

  //! Creates an instance of `ScopedLock` and locks a the given `lock`.
  B2D_INLINE ScopedLock(Lock* lock) noexcept {
    _lock = lock;
    _lock->lock();
  }

  //! Destructor that unlocks locked mutex.
  B2D_INLINE ~ScopedLock() noexcept {
    _lock->unlock();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Lock* _lock;
};


//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_LOCK_H
