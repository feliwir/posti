// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "./functioncache_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

// ============================================================================
// [b2d::Pipe2D::FunctionCache - Statics]
// ============================================================================

//! \internal
//!
//! Remove left horizontal links.
static inline FunctionCache::Node* FunctionCache_skewNode(FunctionCache::Node* node) noexcept {
  FunctionCache::Node* link = node->_link[0];
  uint32_t level = node->_level;

  if (level != 0 && link && link->_level == level) {
    node->_link[0] = link->_link[1];
    link->_link[1] = node;

    node = link;
  }

  return node;
}

//! \internal
//!
//! Remove consecutive horizontal links.
static inline FunctionCache::Node* FunctionCache_splitNode(FunctionCache::Node* node) noexcept {
  FunctionCache::Node* link = node->_link[1];
  uint32_t level = node->_level;

  if (level != 0 && link && link->_link[1] && link->_link[1]->_level == level) {
    node->_link[1] = link->_link[0];
    link->_link[0] = node;

    node = link;
    node->_level++;
  }

  return node;
}

// ============================================================================
// [b2d::Pipe2D::FunctionCache - Construction / Destruction]
// ============================================================================

FunctionCache::FunctionCache() noexcept
  : _root(nullptr),
    _zone(4096 - asmjit::Zone::kBlockOverhead) {}
FunctionCache::~FunctionCache() noexcept {}

// ============================================================================
// [b2d::Pipe2D::FunctionCache - Get / Put]
// ============================================================================

Error FunctionCache::put(uint32_t signature, void* func) noexcept {
  Node* node = _root;
  Node* stack[kHeightLimit];

  Node* newNode = static_cast<Node*>(_zone.alloc(sizeof(Node)));
  newNode->_signature = signature;
  newNode->_level = 0;
  newNode->_func = func;
  newNode->_link[0] = nullptr;
  newNode->_link[1] = nullptr;

  if (B2D_UNLIKELY(!newNode))
    return DebugUtils::errored(kErrorNoMemory);

  if (node == nullptr) {
    _root = newNode;
    return kErrorOk;
  }

  unsigned int top = 0;
  unsigned int dir;

  // Find a spot and save the stack.
  for (;;) {
    stack[top++] = node;
    dir = node->_signature < signature;
    if (node->_link[dir] == nullptr)
      break;
    node = node->_link[dir];
  }

  // Link and rebalance.
  node->_link[dir] = newNode;

  while (top > 0) {
    // Which child?
    node = stack[--top];

    if (top != 0)
      dir = stack[top - 1]->_link[1] == node;

    node = FunctionCache_skewNode(node);
    node = FunctionCache_splitNode(node);

    // Fix the parent.
    if (top != 0)
      stack[top - 1]->_link[dir] = node;
    else
      _root = node;
  }

  return kErrorOk;
}

B2D_END_SUB_NAMESPACE
