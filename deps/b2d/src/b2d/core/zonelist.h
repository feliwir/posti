// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZONELIST_H
#define _B2D_CORE_ZONELIST_H

// [Dependencies]
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ZoneListNode]
// ============================================================================

template<typename NodeT>
class ZoneListNode {
public:
  B2D_NONCOPYABLE(ZoneListNode)

  B2D_INLINE ZoneListNode() noexcept
    : _listNodes { nullptr, nullptr } {}

  B2D_INLINE ZoneListNode(ZoneListNode&& other) noexcept
    : _listNodes { other._listNodes[0], other._listNodes[1] } {}

  B2D_INLINE bool hasPrev() const noexcept { return _listNodes[Globals::kLinkPrev] != nullptr; }
  B2D_INLINE bool hasNext() const noexcept { return _listNodes[Globals::kLinkNext] != nullptr; }

  B2D_INLINE NodeT* prev() const noexcept { return _listNodes[Globals::kLinkPrev]; }
  B2D_INLINE NodeT* next() const noexcept { return _listNodes[Globals::kLinkNext]; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  NodeT* _listNodes[Globals::kLinkCount];
};

// ============================================================================
// [b2d::ZoneList<T>]
// ============================================================================

template <typename NodeT>
class ZoneList {
public:
  B2D_NONCOPYABLE(ZoneList)

  B2D_INLINE ZoneList() noexcept
    : _bounds { nullptr, nullptr } {}

  B2D_INLINE ZoneList(ZoneList&& other) noexcept
    : _bounds { other._bounds[0], other._bounds[1] } {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _bounds[0] = nullptr;
    _bounds[1] = nullptr;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _bounds[0] == nullptr; }
  B2D_INLINE NodeT* first() const noexcept { return _bounds[Globals::kLinkFirst]; }
  B2D_INLINE NodeT* last() const noexcept { return _bounds[Globals::kLinkLast]; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  // Can be used to both prepend and append.
  B2D_INLINE void _addNode(NodeT* node, size_t dir) noexcept {
    NodeT* prev = _bounds[dir];

    node->_listNodes[!dir] = prev;
    _bounds[dir] = node;
    if (prev)
      prev->_listNodes[dir] = node;
    else
      _bounds[!dir] = node;
  }

  // Can be used to both prepend and append.
  B2D_INLINE void _insertNode(NodeT* ref, NodeT* node, size_t dir) noexcept {
    B2D_ASSERT(ref != nullptr);

    NodeT* prev = ref;
    NodeT* next = ref->_listNodes[dir];

    prev->_listNodes[dir] = node;
    if (next)
      next->_listNodes[!dir] = node;
    else
      _bounds[dir] = node;

    node->_listNodes[!dir] = prev;
    node->_listNodes[ dir] = next;
  }

  B2D_INLINE void append(NodeT* node) noexcept { _addNode(node, Globals::kLinkLast); }
  B2D_INLINE void prepend(NodeT* node) noexcept { _addNode(node, Globals::kLinkFirst); }

  B2D_INLINE void insertAfter(NodeT* ref, NodeT* node) noexcept { _insertNode(ref, node, Globals::kLinkNext); }
  B2D_INLINE void insertBefore(NodeT* ref, NodeT* node) noexcept { _insertNode(ref, node, Globals::kLinkPrev); }

  B2D_INLINE NodeT* unlink(NodeT* node) noexcept {
    NodeT* prev = node->prev();
    NodeT* next = node->next();

    if (prev) { prev->_listNodes[Globals::kLinkNext] = next; node->_listNodes[0] = nullptr; } else { _bounds[Globals::kLinkFirst] = next; }
    if (next) { next->_listNodes[Globals::kLinkPrev] = prev; node->_listNodes[1] = nullptr; } else { _bounds[Globals::kLinkLast ] = prev; }

    node->_listNodes[0] = nullptr;
    node->_listNodes[1] = nullptr;

    return node;
  }

  B2D_INLINE NodeT* popFirst() noexcept {
    NodeT* node = _bounds[Globals::kLinkFirst];
    B2D_ASSERT(node != nullptr);

    NodeT* next = node->next();
    _bounds[Globals::kLinkFirst] = next;

    if (next) {
      next->_listNodes[Globals::kLinkPrev] = nullptr;
      node->_listNodes[Globals::kLinkNext] = nullptr;
    }
    else {
      _bounds[Globals::kLinkLast] = nullptr;
    }

    return node;
  }

  B2D_INLINE NodeT* pop() noexcept {
    NodeT* node = _bounds[Globals::kLinkLast];
    B2D_ASSERT(node != nullptr);

    NodeT* prev = node->prev();
    _bounds[Globals::kLinkLast] = prev;

    if (prev) {
      prev->_listNodes[Globals::kLinkNext] = nullptr;
      node->_listNodes[Globals::kLinkPrev] = nullptr;
    }
    else {
      _bounds[Globals::kLinkFirst] = nullptr;
    }

    return node;
  }

  // --------------------------------------------------------------------------
  // [Swap]
  // --------------------------------------------------------------------------

  B2D_INLINE void swapWith(ZoneList& other) noexcept {
    std::swap(_bounds[0], other._bounds[0]);
    std::swap(_bounds[1], other._bounds[1]);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  NodeT* _bounds[Globals::kLinkCount];
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZONELIST_H
