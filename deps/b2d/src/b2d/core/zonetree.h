// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_ZONETREE_H
#define _B2D_CORE_ZONETREE_H

// [Dependencies]
#include "../core/support.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::ZoneTreeNode]
// ============================================================================

//! RB-Tree node.
//!
//! The color is stored in a least significant bit of the `left` node.
//!
//! WARNING: Always use accessors to access left and right children.
class ZoneTreeNode {
public:
  B2D_NONCOPYABLE(ZoneTreeNode)

  static constexpr uintptr_t kRedMask = 0x1;
  static constexpr uintptr_t kPtrMask = ~kRedMask;

  B2D_INLINE ZoneTreeNode() noexcept
    : _rbNodeData { 0, 0 } {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool hasChild(size_t i) const noexcept { return _rbNodeData[i] > kRedMask; }
  B2D_INLINE bool hasLeft() const noexcept { return _rbNodeData[0] > kRedMask; }
  B2D_INLINE bool hasRight() const noexcept { return _rbNodeData[1] != 0; }

  B2D_INLINE ZoneTreeNode* _getChild(size_t i) const noexcept { return (ZoneTreeNode*)(_rbNodeData[i] & kPtrMask); }
  B2D_INLINE ZoneTreeNode* _getLeft() const noexcept { return (ZoneTreeNode*)(_rbNodeData[0] & kPtrMask); }
  B2D_INLINE ZoneTreeNode* _getRight() const noexcept { return (ZoneTreeNode*)(_rbNodeData[1]); }

  B2D_INLINE void _setChild(size_t i, ZoneTreeNode* node) noexcept { _rbNodeData[i] = (_rbNodeData[i] & kRedMask) | (uintptr_t)node; }
  B2D_INLINE void _setLeft(ZoneTreeNode* node) noexcept { _rbNodeData[0] = (_rbNodeData[0] & kRedMask) | (uintptr_t)node; }
  B2D_INLINE void _setRight(ZoneTreeNode* node) noexcept { _rbNodeData[1] = (uintptr_t)node; }

  template<typename T = ZoneTreeNode>
  B2D_INLINE T* child(size_t i) const noexcept { return static_cast<T*>(_getChild(i)); }
  template<typename T = ZoneTreeNode>
  B2D_INLINE T* left() const noexcept { return static_cast<T*>(_getLeft()); }
  template<typename T = ZoneTreeNode>
  B2D_INLINE T* right() const noexcept { return static_cast<T*>(_getRight()); }

  B2D_INLINE bool isRed() const noexcept { return static_cast<bool>(_rbNodeData[0] & kRedMask); }
  B2D_INLINE void _makeRed() noexcept { _rbNodeData[0] |= kRedMask; }
  B2D_INLINE void _makeBlack() noexcept { _rbNodeData[0] &= kPtrMask; }

  //! Get whether the node is RED (RED node must be non-null and must have RED flag set).
  static B2D_INLINE bool _isValidRed(ZoneTreeNode* node) noexcept { return node && node->isRed(); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uintptr_t _rbNodeData[Globals::kLinkCount];
};

template<typename NodeT>
class ZoneTreeNodeT : public ZoneTreeNode {
public:
  B2D_NONCOPYABLE(ZoneTreeNodeT)

  B2D_INLINE ZoneTreeNodeT() noexcept
    : ZoneTreeNode() {}

  B2D_INLINE NodeT* child(size_t i) const noexcept { return static_cast<NodeT*>(_getChild(i)); }
  B2D_INLINE NodeT* left() const noexcept { return static_cast<NodeT*>(_getLeft()); }
  B2D_INLINE NodeT* right() const noexcept { return static_cast<NodeT*>(_getRight()); }
};

// ============================================================================
// [b2d::ZoneTree]
// ============================================================================

template<typename NodeT>
class ZoneTree {
public:
  B2D_NONCOPYABLE(ZoneTree)

  typedef NodeT Node;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ZoneTree() noexcept
    : _root(nullptr) {}

  B2D_INLINE ZoneTree(ZoneTree&& other) noexcept
    : _root(other._root) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool empty() const noexcept { return _root == nullptr; }
  B2D_INLINE NodeT* root() const noexcept { return static_cast<NodeT*>(_root); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept { _root = nullptr; }

  // --------------------------------------------------------------------------
  // [Operations]
  // --------------------------------------------------------------------------

  template<typename CompareT = Support::Compare<Support::kSortAscending>>
  void insert(NodeT* node, const CompareT& cmp = CompareT()) noexcept {
    // Node to insert must not contain garbage.
    B2D_ASSERT(!node->hasLeft());
    B2D_ASSERT(!node->hasRight());
    B2D_ASSERT(!node->isRed());

    if (!_root) {
      _root = node;
      return;
    }

    ZoneTreeNode head;         // False root node,
    head._setRight(_root);     // having root on the right.

    ZoneTreeNode* g = nullptr; // Grandparent.
    ZoneTreeNode* p = nullptr; // Parent.
    ZoneTreeNode* t = &head;   // Iterator.
    ZoneTreeNode* q = _root;   // Query.

    size_t dir = 0;            // Direction for accessing child nodes.
    size_t last = 0;           // Not needed to initialize, but makes some tools happy.

    node->_makeRed();          // New nodes are always red and violations fixed appropriately.

    // Search down the tree.
    for (;;) {
      if (!q) {
        // Insert new node at the bottom.
        q = node;
        p->_setChild(dir, node);
      }
      else if (_isValidRed(q->_getLeft()) && _isValidRed(q->_getRight())) {
        // Color flip.
        q->_makeRed();
        q->_getLeft()->_makeBlack();
        q->_getRight()->_makeBlack();
      }

      // Fix red violation.
      if (_isValidRed(q) && _isValidRed(p))
        t->_setChild(t->_getRight() == g,
                     q == p->_getChild(last) ? _singleRotate(g, !last) : _doubleRotate(g, !last));

      // Stop if found.
      if (q == node)
        break;

      last = dir;
      dir = cmp(*static_cast<NodeT*>(q), *static_cast<NodeT*>(node)) < 0;

      // Update helpers.
      if (g) t = g;

      g = p;
      p = q;
      q = q->_getChild(dir);
    }

    // Update root and make it black.
    _root = static_cast<NodeT*>(head._getRight());
    _root->_makeBlack();
  }

  //! Remove node from RBTree.
  template<typename CompareT = Support::Compare<Support::kSortAscending>>
  void remove(ZoneTreeNode* node, const CompareT& cmp = CompareT()) noexcept {
    ZoneTreeNode head;          // False root node,
    head._setRight(_root);      // having root on the right.

    ZoneTreeNode* g = nullptr;  // Grandparent.
    ZoneTreeNode* p = nullptr;  // Parent.
    ZoneTreeNode* q = &head;    // Query.

    ZoneTreeNode* f  = nullptr; // Found item.
    ZoneTreeNode* gf = nullptr; // Found grandparent.
    size_t dir = 1;             // Direction (0 or 1).

    // Search and push a red down.
    while (q->hasChild(dir)) {
      size_t last = dir;

      // Update helpers.
      g = p;
      p = q;
      q = q->_getChild(dir);
      dir = cmp(*static_cast<NodeT*>(q), *static_cast<NodeT*>(node)) < 0;

      // Save found node.
      if (q == node) {
        f = q;
        gf = g;
      }

      // Push the red node down.
      if (!_isValidRed(q) && !_isValidRed(q->_getChild(dir))) {
        if (_isValidRed(q->_getChild(!dir))) {
          ZoneTreeNode* child = _singleRotate(q, dir);
          p->_setChild(last, child);
          p = child;
        }
        else if (!_isValidRed(q->_getChild(!dir)) && p->_getChild(!last)) {
          ZoneTreeNode* s = p->_getChild(!last);
          if (!_isValidRed(s->_getChild(!last)) && !_isValidRed(s->_getChild(last))) {
            // Color flip.
            p->_makeBlack();
            s->_makeRed();
            q->_makeRed();
          }
          else {
            size_t dir2 = g->_getRight() == p;
            ZoneTreeNode* child = g->_getChild(dir2);

            if (_isValidRed(s->_getChild(last))) {
              child = _doubleRotate(p, last);
              g->_setChild(dir2, child);
            }
            else if (_isValidRed(s->_getChild(!last))) {
              child = _singleRotate(p, last);
              g->_setChild(dir2, child);
            }

            // Ensure correct coloring.
            q->_makeRed();
            child->_makeRed();
            child->_getLeft()->_makeBlack();
            child->_getRight()->_makeBlack();
          }
        }
      }
    }

    // Replace and remove.
    B2D_ASSERT(f != nullptr);
    B2D_ASSERT(f != &head);
    B2D_ASSERT(q != &head);

    p->_setChild(p->_getRight() == q,
                 q->_getChild(q->_getLeft() == nullptr));

    // NOTE: The original algorithm used a trick to just copy 'key/value' to
    // `f` and mark `q` for deletion. But this is unacceptable here as we
    // really want to destroy the passed `node`. So, we really have to make
    // sure that we really removed `f` and not `q` from the tree.
    if (f != q) {
      B2D_ASSERT(f != &head);
      B2D_ASSERT(f != gf);

      ZoneTreeNode* n = gf ? gf : &head;
      dir = (n == &head) ? 1  : cmp(*static_cast<NodeT*>(n), *static_cast<NodeT*>(node)) < 0;

      for (;;) {
        if (n->_getChild(dir) == f) {
          n->_setChild(dir, q);
          // RAW copy, including the color.
          q->_rbNodeData[0] = f->_rbNodeData[0];
          q->_rbNodeData[1] = f->_rbNodeData[1];
          break;
        }

        n = n->_getChild(dir);

        // Cannot be true as we know that it must reach `f` in few iterations.
        B2D_ASSERT(n != nullptr);
        dir = cmp(*static_cast<NodeT*>(n), *static_cast<NodeT*>(node)) < 0;
      }
    }

    // Update root and make it black.
    _root = static_cast<NodeT*>(head._getRight());
    if (_root) _root->_makeBlack();
  }

  template<typename KeyT, typename CompareT = Support::Compare<Support::kSortAscending>>
  B2D_INLINE NodeT* get(const KeyT& key, const CompareT& cmp = CompareT()) const noexcept {
    ZoneTreeNode* node = _root;
    while (node) {
      auto result = cmp(*static_cast<const NodeT*>(node), key);
      if (result == 0) break;

      // Go left or right depending on the `result`.
      node = node->_getChild(result < 0);
    }
    return static_cast<NodeT*>(node);
  }

  // --------------------------------------------------------------------------
  // [Swap]
  // --------------------------------------------------------------------------

  B2D_INLINE void swapWith(ZoneTree& other) noexcept {
    std::swap(_root, other._root);
  }

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

  static B2D_INLINE bool _isValidRed(ZoneTreeNode* node) noexcept { return ZoneTreeNode::_isValidRed(node); }

  //! Single rotation.
  static B2D_INLINE ZoneTreeNode* _singleRotate(ZoneTreeNode* root, size_t dir) noexcept {
    ZoneTreeNode* save = root->_getChild(!dir);
    root->_setChild(!dir, save->_getChild(dir));
    save->_setChild( dir, root);
    root->_makeRed();
    save->_makeBlack();
    return save;
  }

  //! Double rotation.
  static B2D_INLINE ZoneTreeNode* _doubleRotate(ZoneTreeNode* root, size_t dir) noexcept {
    root->_setChild(!dir, _singleRotate(root->_getChild(!dir), !dir));
    return _singleRotate(root, dir);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  NodeT* _root;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_ZONETREE_H
