// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FUNCTIONCACHE_P_H
#define _B2D_PIPE2D_FUNCTIONCACHE_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FunctionCache]
// ============================================================================

//! Function cache.
//!
//! The cache is currently implemented as an AVL tree. The reason to not using
//! RedBlack tree is that we prefer better balance by sacrifising insertion
//! cost. Also functions that were already generated won't be deleted from the
//! cache, thus the implementation is relatively simple.
//!
//! NOTE: No locking is preformed implicitly, it's user's responsibility to
//! ensure only one thread is accessing `FunctionCache` at the sime time.
class FunctionCache {
public:
  enum { kHeightLimit = 64 };

  struct Node {
    uint32_t _signature;                 //!< Function signature.
    uint32_t _level;                     //!< Horizontal level used for tree balancing.
    void* _func;                         //!< Function pointer.
    Node* _link[2];                      //!< Left and right nodes.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FunctionCache() noexcept;
  ~FunctionCache() noexcept;

  // --------------------------------------------------------------------------
  // [Get / Put]
  // --------------------------------------------------------------------------

  inline void* get(uint32_t signature) const noexcept {
    Node* node = _root;
    while (node) {
      uint32_t nodeSignature = node->_signature;
      if (nodeSignature == signature)
        return node->_func;
      node = node->_link[nodeSignature < signature];
    }
    return nullptr;
  }

  Error put(uint32_t signature, void* func) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Node* _root;
  asmjit::Zone _zone;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FUNCTIONCACHE_P_H
