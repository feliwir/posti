// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/math.h"
#include "../core/zone.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Zone - Statics]
// ============================================================================

// Zero size block used by `Zone` that doesn't have any memory allocated.
// Should be allocated in read-only memory and should never be modified.
const Zone::Block Zone::_zeroBlock = { nullptr, nullptr, 0 };

// ============================================================================
// [b2d::Zone - Init / Reset]
// ============================================================================

void Zone::_init(size_t blockSize, size_t blockAlignment, const Support::Temporary* temporary) noexcept {
  B2D_ASSERT(blockSize >= kMinBlockSize);
  B2D_ASSERT(blockSize <= kMaxBlockSize);
  B2D_ASSERT(blockAlignment <= 64);

  _assignZeroBlock();
  _blockSize = blockSize & Support::lsbMask<size_t>(Support::bitSizeOf<size_t>() - 4);
  _isTemporary = temporary != nullptr;
  _blockAlignmentShift = Support::ctz(blockAlignment) & 0x7;

  // Setup the first [temporary] block, if necessary.
  if (temporary) {
    Block* block = temporary->data<Block>();
    block->prev = nullptr;
    block->next = nullptr;

    B2D_ASSERT(temporary->size() >= kBlockSize);
    block->size = temporary->size() - kBlockSize;

    _assignBlock(block);
  }
}

void Zone::reset(uint32_t resetPolicy) noexcept {
  Block* cur = _block;

  // Can't be altered.
  if (cur == &_zeroBlock)
    return;

  if (resetPolicy == Globals::kResetHard) {
    Block* initial = const_cast<Zone::Block*>(&_zeroBlock);
    _ptr = initial->data();
    _end = initial->data();
    _block = initial;

    // Since cur can be in the middle of the double-linked list, we have to
    // traverse both directions (`prev` and `next`) separately to visit all.
    Block* next = cur->next;
    do {
      Block* prev = cur->prev;

      // If this is the first block and this ZoneTmp is temporary then the
      // first block is statically allocated. We cannot free it and it makes
      // sense to keep it even when this is hard reset.
      if (prev == nullptr && _isTemporary) {
        cur->prev = nullptr;
        cur->next = nullptr;
        _assignBlock(cur);
        break;
      }

      Allocator::systemRelease(cur);
      cur = prev;
    } while (cur);

    cur = next;
    while (cur) {
      next = cur->next;
      Allocator::systemRelease(cur);
      cur = next;
    }
  }
  else {
    while (cur->prev)
      cur = cur->prev;
    _assignBlock(cur);
  }
}

// ============================================================================
// [b2d::Zone - Alloc]
// ============================================================================

void* Zone::_alloc(size_t size, size_t alignment) noexcept {
  Block* curBlock = _block;
  Block* next = curBlock->next;

  size_t rawBlockAlignment = blockAlignment();
  size_t minimumAlignment = Math::max<size_t>(alignment, rawBlockAlignment);

  // If the `Zone` has been cleared the current block doesn't have to be the
  // last one. Check if there is a block that can be used instead of allocating
  // a new one. If there is a `next` block it's completely unused, we don't have
  // to check for remaining bytes in that case.
  if (next) {
    uint8_t* ptr = Support::alignUp(next->data(), minimumAlignment);
    uint8_t* end = Support::alignDown(next->data() + next->size, rawBlockAlignment);

    if (size <= (size_t)(end - ptr)) {
      _block = next;
      _ptr = ptr + size;
      _end = Support::alignDown(next->data() + next->size, rawBlockAlignment);
      return static_cast<void*>(ptr);
    }
  }

  size_t blockAlignmentOverhead = alignment - Math::min<size_t>(alignment, Globals::kMemAllocAlignment);
  size_t newSize = Math::max(blockSize(), size);

  // Prevent arithmetic overflow.
  if (B2D_UNLIKELY(newSize > std::numeric_limits<size_t>::max() - kBlockSize - blockAlignmentOverhead))
    return nullptr;

  // Allocate new block - we add alignment overhead to `newSize`, which becomes the
  // new block size, and we also add `kBlockOverhead` to the allocator as it includes
  // members of `Zone::Block` structure.
  newSize += blockAlignmentOverhead;
  Block* newBlock = static_cast<Block*>(Allocator::systemAlloc(newSize + kBlockSize));

  if (B2D_UNLIKELY(!newBlock))
    return nullptr;

  // Align the pointer to `minimumAlignment` and adjust the size of this block
  // accordingly. It's the same as using `minimumAlignment - Support::alignUpDiff()`,
  // just written differently.
  {
    newBlock->prev = nullptr;
    newBlock->next = nullptr;
    newBlock->size = newSize;

    if (curBlock != &_zeroBlock) {
      newBlock->prev = curBlock;
      curBlock->next = newBlock;

      // Does only happen if there is a next block, but the requested memory
      // can't fit into it. In this case a new buffer is allocated and inserted
      // between the current block and the next one.
      if (next) {
        newBlock->next = next;
        next->prev = newBlock;
      }
    }

    uint8_t* ptr = Support::alignUp(newBlock->data(), minimumAlignment);
    uint8_t* end = Support::alignDown(newBlock->data() + newSize, rawBlockAlignment);

    _ptr = ptr + size;
    _end = end;
    _block = newBlock;

    B2D_ASSERT(_ptr <= _end);
    return static_cast<void*>(ptr);
  }
}

void* Zone::allocZeroed(size_t size, size_t alignment) noexcept {
  void* p = alloc(size, alignment);
  if (B2D_UNLIKELY(!p))
    return p;
  return std::memset(p, 0, size);
}

B2D_END_NAMESPACE
