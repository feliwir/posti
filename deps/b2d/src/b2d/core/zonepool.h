// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZONEPOOL_H
#define _B2D_CORE_ZONEPOOL_H

// [Dependencies]
#include "../core/zone.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ZonePool]
// ============================================================================

template<typename T, size_t SizeOfT = sizeof(T)>
class ZonePool {
public:
  B2D_NONCOPYABLE(ZonePool)

  // --------------------------------------------------------------------------
  // [Link]
  // --------------------------------------------------------------------------

  struct Link {
    Link* next;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ZonePool(Zone* zone) noexcept
    : _zone(zone),
      _pool(nullptr) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the zone pool.
  //!
  //! Reset must be called after the associated `Zone` has been reset, otherwise
  //! the existing pool will collide with possible allocations made on the `Zone`
  //! object after the reset.
  B2D_INLINE void reset() noexcept { _pool = nullptr; }

  // --------------------------------------------------------------------------
  // [Alloc / Free]
  // --------------------------------------------------------------------------

  //! Ensure that there is at least one object in the pool.
  B2D_INLINE bool ensure() noexcept {
    if (_pool) return true;

    Link* p = static_cast<Link*>(_zone->alloc(SizeOfT));
    if (p == nullptr) return false;

    p->next = nullptr;
    _pool = p;
    return true;
  }

  //! Alloc a memory (or reuse the existing allocation) of `size` (in byts).
  B2D_INLINE T* alloc() noexcept {
    Link* p = _pool;
    if (B2D_UNLIKELY(p == nullptr))
      return static_cast<T*>(_zone->alloc(SizeOfT));
    _pool = p->next;
    return static_cast<T*>(static_cast<void*>(p));
  }

  //! Like `alloc()`, but can be only called after `ensure()` returned `true`.
  B2D_INLINE T* allocEnsured() noexcept {
    Link* p = _pool;
    B2D_ASSERT(p != nullptr);

    _pool = p->next;
    return static_cast<T*>(static_cast<void*>(p));
  }

  //! Pool the previously allocated memory.
  B2D_INLINE void free(T* _p) noexcept {
    B2D_ASSERT(_p != nullptr);
    Link* p = reinterpret_cast<Link*>(_p);

    p->next = _pool;
    _pool = p;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone* _zone;
  Link* _pool;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZONEPOOL_H
