// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/lock.h"
#include "../core/math.h"
#include "../core/runtime.h"
#include "../core/wrap.h"
#include "../core/zeroallocator_p.h"
#include "../core/zone.h"
#include "../core/zonelist.h"
#include "../core/zonetree.h"

#ifdef B2D_BUILD_TEST
  #include <random>
#endif

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::ZeroAllocator - Implementation]
// ============================================================================

#ifdef B2D_BUILD_DEBUG
static void ZeroAllocator_checkReleasedMemory(uint8_t* ptr, size_t size) noexcept {
  // Must be aligned.
  B2D_ASSERT(Support::isAligned(ptr , sizeof(uintptr_t)));
  B2D_ASSERT(Support::isAligned(size, sizeof(uintptr_t)));

  uintptr_t* p = reinterpret_cast<uintptr_t*>(ptr);
  for (size_t i = 0; i < size / sizeof(uintptr_t); i++)
    B2D_ASSERT(p[i] == 0);
}
#endif

//! ZeroAllocator implementation, based on asmjit's JitAllocator, but simplified
//! a bit as we don't need some features that the original `JitAllocator` provided.
class ZeroAllocatorImpl {
public:
  B2D_NONCOPYABLE(ZeroAllocatorImpl)

  typedef Support::BitWord BitWord;

  static constexpr uint32_t kBlockAlignment = 64;
  static constexpr uint32_t kBlockGranularity = 1024;

  static constexpr uint32_t kMinBlockSize = 1048576; // 1MB.
  static constexpr uint32_t kMaxBlockSize = 8388608; // 8MB.

  static constexpr size_t bitWordCountFromAreaSize(uint32_t areaSize) noexcept {
    using Support::kBitWordSizeInBits;
    return Support::alignUp<size_t>(areaSize, kBitWordSizeInBits) / kBitWordSizeInBits;
  }

  class Block : public ZoneTreeNodeT<Block>,
                public ZoneListNode<Block> {
  public:
    B2D_NONCOPYABLE(Block)

    enum Flags : uint32_t {
      kFlagStatic = 0x00000001u,         //!< This is a statically allocated block.
      kFlagDirty  = 0x80000000u          //!< Block is dirty (some members needs update).
    };

    B2D_INLINE Block(uint8_t* buffer, size_t blockSize, uint32_t areaSize) noexcept
      : ZoneTreeNodeT(),
        _buffer(buffer),
        _bufferAligned(Support::alignUp(buffer, kBlockAlignment)),
        _blockSize(blockSize),
        _flags(0),
        _areaSize(areaSize),
        _areaUsed(0),
        _largestUnusedArea(areaSize),
        _searchStart(0),
        _searchEnd(areaSize) {}

    B2D_INLINE uint8_t* bufferAligned() const noexcept { return _bufferAligned; }
    B2D_INLINE size_t blockSize() const noexcept { return _blockSize; }
    B2D_INLINE size_t overheadSize() const noexcept { return sizeof(Block) - sizeof(BitWord) + Support::numGranularized(areaSize(), Support::kBitWordSizeInBits); }

    B2D_INLINE uint32_t flags() const noexcept { return _flags; }
    B2D_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }
    B2D_INLINE void addFlags(uint32_t flags) noexcept { _flags |= flags; }
    B2D_INLINE void clearFlags(uint32_t flags) noexcept { _flags &= ~flags; }

    B2D_INLINE uint32_t areaSize() const noexcept { return _areaSize; }
    B2D_INLINE uint32_t areaUsed() const noexcept { return _areaUsed; }
    B2D_INLINE uint32_t areaAvailable() const noexcept { return areaSize() - areaUsed(); }
    B2D_INLINE uint32_t largestUnusedArea() const noexcept { return _largestUnusedArea; }

    B2D_INLINE void resetBitVector() noexcept {
      std::memset(_bitVector, 0, Support::numGranularized(_areaSize, Support::kBitWordSizeInBits) * sizeof(BitWord));
    }

    // RBTree default CMP uses '<' and '>' operators.
    B2D_INLINE bool operator<(const Block& other) const noexcept { return bufferAligned() < other.bufferAligned(); }
    B2D_INLINE bool operator>(const Block& other) const noexcept { return bufferAligned() > other.bufferAligned(); }

    // Special implementation for querying blocks by `key`, which must be in `[buffer, buffer + blockSize)` range.
    B2D_INLINE bool operator<(const uint8_t* key) const noexcept { return bufferAligned() + blockSize() <= key; }
    B2D_INLINE bool operator>(const uint8_t* key) const noexcept { return bufferAligned() > key; }

    uint8_t* _buffer;                    //!< Zeroed buffer managed by this block.
    uint8_t* _bufferAligned;             //!< Aligned `_buffer` to kBlockAlignment.
    size_t _blockSize;                   //!< Size of `buffer` in bytes.

    uint32_t _flags;                     //!< Block flags.
    uint32_t _areaSize;                  //!< Size of the whole block area (bit-vector size).
    uint32_t _areaUsed;                  //!< Used area (number of bits in bit-vector used).
    uint32_t _largestUnusedArea;         //!< The largest unused continuous area in the bit-vector (or `_areaSize` to initiate rescan).
    uint32_t _searchStart;               //!< Start of a search range (for unused bits).
    uint32_t _searchEnd;                 //!< End of a search range (for unused bits).
    BitWord _bitVector[1];               //!< Bit vector representing all used areas (0 = unused, 1 = used).
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE ZeroAllocatorImpl(Block* baseBlock) noexcept
    : _tree(),
      _blocks(),
      _blockCount(0),
      _baseAreaSize(0),
      _totalAreaSize(0),
      _totalAreaUsed(0),
      _cleanupThreshold(0),
      _overheadSize(0) {

    baseBlock->addFlags(Block::kFlagStatic);
    insertBlock(baseBlock);

    _baseAreaSize = _totalAreaSize;
    _cleanupThreshold = _totalAreaSize;
  }

  B2D_INLINE ~ZeroAllocatorImpl() noexcept {
    _cleanupInternal();
  }

  // --------------------------------------------------------------------------
  // [Block Management]
  // --------------------------------------------------------------------------

  // Allocate a new `ZeroAllocator::Block` for the given `blockSize`.
  Block* newBlock(size_t blockSize) noexcept {
    using Support::BitWord;
    using Support::kBitWordSizeInBits;

    uint32_t areaSize = uint32_t((blockSize + kBlockGranularity - 1) / kBlockGranularity);
    uint32_t numBitWords = (areaSize + kBitWordSizeInBits - 1u) / kBitWordSizeInBits;

    size_t blockStructSize = sizeof(Block) + size_t(numBitWords - 1) * sizeof(BitWord);
    Block* block = static_cast<Block*>(Allocator::systemAlloc(blockStructSize));
    uint8_t* buffer = static_cast<uint8_t*>(std::calloc(1, blockSize + kBlockAlignment));

    // Out of memory.
    if (B2D_UNLIKELY(!block || !buffer)) {
      if (buffer)
        std::free(buffer);

      if (block)
        Allocator::systemRelease(block);

      return nullptr;
    }

    block = new(block) Block(buffer, blockSize, areaSize);
    block->resetBitVector();
    return block;
  }

  void deleteBlock(Block* block) noexcept {
    B2D_ASSERT(!(block->hasFlag(Block::kFlagStatic)));

    std::free(block->_buffer);
    Allocator::systemRelease(block);
  }

  void insertBlock(Block* block) noexcept {
    // Add to RBTree and List.
    _tree.insert(block);
    _blocks.append(block);

    // Update statistics.
    _blockCount++;
    _totalAreaSize += block->areaSize();
    _overheadSize += block->overheadSize();
  }

  void removeBlock(Block* block) noexcept {
    // Remove from RBTree and List.
    _tree.remove(block);
    _blocks.unlink(block);

    // Update statistics.
    _blockCount--;
    _totalAreaSize -= block->areaSize();
    _overheadSize -= block->overheadSize();
  }

  B2D_INLINE size_t calculateIdealBlockSize(size_t allocationSize) noexcept {
    uint32_t kMaxSizeShift = Support::ctz(kMaxBlockSize) -
                             Support::ctz(kMinBlockSize) ;

    size_t blockSize = size_t(kMinBlockSize) << Math::min<uint32_t>(kMaxSizeShift, _blockCount);
    if (blockSize < allocationSize)
      blockSize = Support::alignUp(allocationSize, blockSize);
    return blockSize;
  }

  B2D_INLINE size_t calculateCleanupThreshold() const noexcept {
    if (_blockCount <= 6)
      return 0;

    size_t area = _totalAreaSize - _baseAreaSize;
    size_t threshold = area / 5u;
    return _baseAreaSize + threshold;
  }

  // --------------------------------------------------------------------------
  // [Cleanup]
  // --------------------------------------------------------------------------

  void _cleanupInternal(size_t n = std::numeric_limits<size_t>::max()) noexcept {
    Block* block = _blocks.last();

    while (block && n) {
      Block* prev = block->prev();
      if (block->areaUsed() == 0 && !(block->hasFlag(Block::kFlagStatic))) {
        removeBlock(block);
        deleteBlock(block);
        n--;
      }
      block = prev;
    }

    _cleanupThreshold = calculateCleanupThreshold();
  }

  B2D_INLINE void cleanup() noexcept {
    ScopedLock locked(_lock);
    _cleanupInternal();
  }

  // --------------------------------------------------------------------------
  // [Memory Info]
  // --------------------------------------------------------------------------

  B2D_INLINE void populateMemoryInfo(Runtime::MemoryInfo& memoryInfo) noexcept {
    ScopedLock locked(_lock);

    memoryInfo._zeroedMemoryUsed = _totalAreaUsed * kBlockGranularity;
    memoryInfo._zeroedMemoryReserved = _totalAreaSize * kBlockGranularity;
    memoryInfo._zeroedMemoryOverhead = _overheadSize;
    memoryInfo._zeroedMemoryBlockCount = _blockCount;
  }

  // --------------------------------------------------------------------------
  // [Alloc / Release]
  // --------------------------------------------------------------------------

  void* _allocInternal(size_t size, size_t& allocatedSize) noexcept {
    constexpr uint32_t kNoIndex = std::numeric_limits<uint32_t>::max();

    // Align to minimum granularity by default.
    size = Support::alignUp<size_t>(size, kBlockGranularity);
    allocatedSize = 0;

    if (B2D_UNLIKELY(size == 0 || size > std::numeric_limits<uint32_t>::max() / 2))
      return nullptr;

    Block* block = _blocks.first();
    uint32_t areaIndex = kNoIndex;
    uint32_t areaSize = uint32_t(Support::numGranularized(size, kBlockGranularity));

    // Try to find the requested memory area in existing blocks.
    if (block) {
      Block* initial = block;
      do {
        Block* next = block->hasNext() ? block->next() : _blocks.first();
        if (block->areaAvailable() >= areaSize) {
          if (block->hasFlag(Block::kFlagDirty) || block->largestUnusedArea() >= areaSize) {
            uint32_t blockAreaSize = block->areaSize();
            uint32_t searchStart = block->_searchStart;
            uint32_t searchEnd = block->_searchEnd;

            Support::BitVectorFlipIterator<BitWord> it(
              block->_bitVector, Support::numGranularized(searchEnd, Support::kBitWordSizeInBits), searchStart, Support::allOnes<BitWord>());

            // If there is unused area available then there has to be at least one match.
            B2D_ASSERT(it.hasNext());

            uint32_t bestArea = blockAreaSize;
            uint32_t largestArea = 0;

            uint32_t holeIndex = uint32_t(it.peekNext());
            uint32_t holeEnd = holeIndex;

            searchStart = holeIndex;
            do {
              holeIndex = uint32_t(it.nextAndFlip());
              if (holeIndex >= searchEnd) break;

              holeEnd = it.hasNext() ? Math::min(searchEnd, uint32_t(it.nextAndFlip())) : searchEnd;
              uint32_t holeSize = holeEnd - holeIndex;

              if (holeSize >= areaSize && bestArea >= holeSize) {
                largestArea = Math::max(largestArea, bestArea);
                bestArea = holeSize;
                areaIndex = holeIndex;
              }
              else {
                largestArea = Math::max(largestArea, holeSize);
              }
            } while (it.hasNext());
            searchEnd = holeEnd;

            // Because we have traversed the entire block, we can now mark the
            // largest unused area that can be used to cache the next traversal.
            block->_searchStart = searchStart;
            block->_searchEnd = searchEnd;
            block->_largestUnusedArea = largestArea;
            block->clearFlags(Block::kFlagDirty);

            if (areaIndex != kNoIndex) {
              if (searchStart == areaIndex)
                block->_searchStart += areaSize;
              break;
            }
          }
        }

        block = next;
      } while (block != initial);
    }

    // Allocate a new block if there is no region of a required width.
    if (areaIndex == kNoIndex) {
      size_t blockSize = calculateIdealBlockSize(size);
      block = newBlock(blockSize);

      if (B2D_UNLIKELY(!block))
        return nullptr;

      insertBlock(block);
      _cleanupThreshold = calculateCleanupThreshold();

      areaIndex = 0;
      block->_searchStart = areaSize;
      block->_largestUnusedArea = block->areaSize() - areaSize;
    }

    // Update statistics.
    _totalAreaUsed += areaSize;
    block->_areaUsed += areaSize;

    // Handle special cases.
    if (block->areaAvailable() == 0) {
      // The whole block is filled.
      block->_searchStart = block->areaSize();
      block->_searchEnd = 0;
      block->_largestUnusedArea = 0;
      block->clearFlags(Block::kFlagDirty);
    }

    // Mark the newly allocated space as occupied and also the sentinel.
    Support::bitVectorFill(block->_bitVector, areaIndex, areaSize);

    // Return a pointer to allocated memory.
    uint8_t* result = block->bufferAligned() + areaIndex * kBlockGranularity;
    B2D_ASSERT(result >= block->bufferAligned());
    B2D_ASSERT(result <= block->bufferAligned() + block->blockSize() - size);

    allocatedSize = size;
    return result;
  }

  void _releaseInternal(void* ptr, size_t size) noexcept {
    B2D_ASSERT(ptr != nullptr);
    B2D_ASSERT(size != 0);

    Block* block = _tree.get(static_cast<uint8_t*>(ptr));
    B2D_ASSERT(block != nullptr);

    #ifdef B2D_BUILD_DEBUG
    ZeroAllocator_checkReleasedMemory(static_cast<uint8_t*>(ptr), size);
    #endif

    // Offset relative to the start of the block.
    size_t byteOffset = (size_t)((uint8_t*)ptr - block->bufferAligned());

    // The first bit representing the allocated area and its size.
    uint32_t areaIndex = uint32_t(byteOffset / kBlockGranularity);
    uint32_t areaSize = uint32_t(Support::numGranularized(size, kBlockGranularity));

    // Update the search region and statistics.
    block->_searchStart = Math::min(block->_searchStart, areaIndex);
    block->_searchEnd = Math::max(block->_searchEnd, areaIndex + areaSize);
    block->addFlags(Block::kFlagDirty);

    block->_areaUsed -= areaSize;
    _totalAreaUsed -= areaSize;

    // Clear bits used to mark this area as occupied.
    Support::bitVectorClear(block->_bitVector, areaIndex, areaSize);

    if (_totalAreaUsed < _cleanupThreshold)
      _cleanupInternal(1);
  }

  B2D_INLINE void* alloc(size_t size, size_t& allocatedSize) noexcept {
    ScopedLock locked(_lock);
    return _allocInternal(size, allocatedSize);
  }

  B2D_INLINE void* realloc(void* oldPtr, size_t oldSize, size_t newSize, size_t& allocatedSize) noexcept {
    ScopedLock locked(_lock);
    if (oldPtr != nullptr)
      _releaseInternal(oldPtr, oldSize);
    return _allocInternal(newSize, allocatedSize);
  }

  B2D_INLINE void release(void* ptr, size_t size) noexcept {
    ScopedLock locked(_lock);
    _releaseInternal(ptr, size);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  mutable Lock _lock;                    //!< Mutex for thread safety.
  ZoneTree<Block> _tree;                 //!< RB-Tree that contains all blocks.
  ZoneList<Block> _blocks;               //!< Double linked list of blocks.
  uint32_t _blockCount;                  //!< Allocated block count.
  size_t _baseAreaSize;                  //!< Area size of base block.
  size_t _totalAreaSize;                 //!< Number of bits reserved across all blocks.
  size_t _totalAreaUsed;                 //!< Number of bits used across all blocks.
  size_t _cleanupThreshold;              //!< A threshold to trigger auto-cleanup.
  size_t _overheadSize;                  //!< Memory overhead required to manage blocks.
};

static Wrap<ZeroAllocatorImpl> gZeroAllocatorImpl;

// ============================================================================
// [b2d::ZeroAllocator - Public API]
// ============================================================================

void* ZeroAllocator::alloc(size_t size, size_t& allocatedSize) noexcept {
  return gZeroAllocatorImpl->alloc(size, allocatedSize);
}

void* ZeroAllocator::realloc(void* oldPtr, size_t oldSize, size_t newSize, size_t& allocatedSize) noexcept {
  return gZeroAllocatorImpl->realloc(oldPtr, oldSize, newSize, allocatedSize);
}

void ZeroAllocator::release(void* ptr, size_t size) noexcept {
  gZeroAllocatorImpl->release(ptr, size);
}

// ============================================================================
// [b2d::ZeroAllocator - Static Buffer]
// ============================================================================

// Base memory is a zeroed memory allocated by the linker. By default we use
// 1MB of memory that we will use as a base before obtaining more from the
// system if that's not enough.

struct ZeroAllocatorStaticBlock {
  enum : uint32_t {
    kBlockSize = 1024u * 1024u,
    kAreaSize = Support::numGranularized(kBlockSize, ZeroAllocatorImpl::kBlockGranularity),
    kBitWordCount = Support::numGranularized(kAreaSize, Support::kBitWordSizeInBits)
  };

  Wrap<ZeroAllocatorImpl::Block> block;
  Support::BitWord bitWords[kBitWordCount];
};

struct alignas(64) ZeroAllocatorStaticBuffer {
  uint8_t buffer[ZeroAllocatorStaticBlock::kBlockSize];
};

static ZeroAllocatorStaticBlock gZeroAllocatorStaticBlock;
static ZeroAllocatorStaticBuffer gZeroAllocatorStaticBuffer;

// ============================================================================
// [b2d::ZeroAllocator - Runtime Handlers]
// ============================================================================

static void B2D_CDECL ZeroAllocatorOnShutdown(Runtime::Context& rt) noexcept {
  B2D_UNUSED(rt);
  gZeroAllocatorImpl.destroy();
}

static void B2D_CDECL ZeroAllocatorOnCleanup(Runtime::Context& rt, uint32_t cleanupFlags) noexcept {
  B2D_UNUSED(rt);
  if (cleanupFlags & Runtime::kCleanupZeroedMemory)
    gZeroAllocatorImpl->cleanup();
}

static void B2D_CDECL ZeroAllocatorOnMemoryInfo(Runtime::Context& rt, Runtime::MemoryInfo& memoryInfo) noexcept {
  B2D_UNUSED(rt);
  gZeroAllocatorImpl->populateMemoryInfo(memoryInfo);
}

void ZeroAllocatorOnInit(Runtime::Context& rt) noexcept {
  ZeroAllocatorImpl::Block* block =
    gZeroAllocatorStaticBlock.block.init(
      gZeroAllocatorStaticBuffer.buffer,
      ZeroAllocatorStaticBlock::kBlockSize,
      ZeroAllocatorStaticBlock::kAreaSize);

  gZeroAllocatorImpl.init(block);

  rt.addShutdownHandler(ZeroAllocatorOnShutdown);
  rt.addCleanupHandler(ZeroAllocatorOnCleanup);
  rt.addMemoryInfoHandler(ZeroAllocatorOnMemoryInfo);
}

// ============================================================================
// [b2d::ZeroAllocator - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
// Helper class to verify that ZeroAllocator doesn't return addresses that overlap.
class ZeroAllocatorWrapper {
public:
  inline ZeroAllocatorWrapper() noexcept {}

  // Address to a memory region of a given size.
  class Range {
  public:
    inline Range(uint8_t* addr, size_t size) noexcept
      : addr(addr),
        size(size) {}
    uint8_t* addr;
    size_t size;
  };

  // Based on ZeroAllocator::Block, serves our purpose well...
  class Record : public ZoneTreeNodeT<Record>,
                 public Range {
  public:
    inline Record(uint8_t* addr, size_t size)
      : ZoneTreeNodeT<Record>(),
        Range(addr, size) {}

    inline bool operator<(const Record& other) const noexcept { return addr < other.addr; }
    inline bool operator>(const Record& other) const noexcept { return addr > other.addr; }

    inline bool operator<(const uint8_t* key) const noexcept { return addr + size <= key; }
    inline bool operator>(const uint8_t* key) const noexcept { return addr > key; }
  };

  void _insert(void* p_, size_t size) noexcept {
    uint8_t* p = static_cast<uint8_t*>(p_);
    uint8_t* pEnd = p + size - 1;

    Record* record = _records.get(p);
    if (record)
      EXPECT(record == nullptr,
             "Address [%p:%p] collides with a newly allocated [%p:%p]\n", record->addr, record->addr + record->size, p, p + size);

    record = _records.get(pEnd);
    if (record)
      EXPECT(record == nullptr,
             "Address [%p:%p] collides with a newly allocated [%p:%p]\n", record->addr, record->addr + record->size, p, p + size);

    record = new(std::nothrow) Record(p, size);
    EXPECT(record != nullptr,
           "Out of memory, cannot allocate 'Record'");

    _records.insert(record);
  }

  void _remove(void* p) noexcept {
    Record* record = _records.get(static_cast<uint8_t*>(p));
    EXPECT(record != nullptr,
           "Address [%p] doesn't exist\n", p);

    _records.remove(record);
    delete record;
  }

  void* alloc(size_t size) noexcept {
    size_t allocatedSize = 0;
    void* p = ZeroAllocator::alloc(size, allocatedSize);
    EXPECT(p != nullptr,
           "ZeroAllocator failed to allocate '%u' bytes\n", unsigned(size));

    for (size_t i = 0; i < allocatedSize; i++) {
      EXPECT(static_cast<const uint8_t*>(p)[i] == 0,
            "The returned pointer doesn't point to a zeroed memory %p[%u]\n", p, int(size));
    }

    _insert(p, allocatedSize);
    return p;
  }

  size_t getSizeOfPtr(void* p) noexcept {
    Record* record = _records.get(static_cast<uint8_t*>(p));
    return record ? record->size : size_t(0);
  }

  void release(void* p) noexcept {
    size_t size = getSizeOfPtr(p);
    _remove(p);
    ZeroAllocator::release(p, size);
  }

  ZoneTree<Record> _records;
};

static void ZeroAllocatorTest_shuffle(void** ptrArray, size_t count, std::mt19937& prng) noexcept {
  for (size_t i = 0; i < count; ++i)
    std::swap(ptrArray[i], ptrArray[size_t(prng() % count)]);
}

static void ZeroAllocatorTest_usage() noexcept {
  Runtime::MemoryInfo mi = Runtime::memoryInfo();

  INFO("NumBlocks: %9llu"         , (unsigned long long)(mi.zeroedMemoryBlockCount()));
  INFO("UsedSize : %9llu [Bytes]" , (unsigned long long)(mi.zeroedMemoryUsed()));
  INFO("Reserved : %9llu [Bytes]" , (unsigned long long)(mi.zeroedMemoryReserved()));
  INFO("Overhead : %9llu [Bytes]" , (unsigned long long)(mi.zeroedMemoryOverhead()));
}

UNIT(b2d_core_zero_allocator) {
  ZeroAllocatorWrapper wrapper;
  std::mt19937 prng(100);

  size_t i;
  size_t kCount = 50000;

  INFO("Memory alloc/release test - %d allocations", kCount);

  void** ptrArray = (void**)Allocator::systemAlloc(sizeof(void*) * size_t(kCount));
  EXPECT(ptrArray != nullptr,
        "Couldn't allocate '%u' bytes for pointer-array", unsigned(sizeof(void*) * size_t(kCount)));

  INFO("Allocating zeroed memory...");
  for (i = 0; i < kCount; i++)
    ptrArray[i] = wrapper.alloc((prng() % 8000) + 128);
  ZeroAllocatorTest_usage();

  INFO("Releasing zeroed memory...");
  for (i = 0; i < kCount; i++)
    wrapper.release(ptrArray[i]);
  ZeroAllocatorTest_usage();

  INFO("Submitting manual cleanup...");
  Runtime::cleanup(Runtime::kCleanupZeroedMemory);
  ZeroAllocatorTest_usage();

  INFO("Allocating zeroed memory...", kCount);
  for (i = 0; i < kCount; i++)
    ptrArray[i] = wrapper.alloc((prng() % 8000) + 128);
  ZeroAllocatorTest_usage();

  INFO("Shuffling...");
  ZeroAllocatorTest_shuffle(ptrArray, unsigned(kCount), prng);

  INFO("Releasing 50% blocks...");
  for (i = 0; i < kCount / 2; i++)
    wrapper.release(ptrArray[i]);
  ZeroAllocatorTest_usage();

  INFO("Allocating 50% blocks again...");
  for (i = 0; i < kCount / 2; i++)
    ptrArray[i] = wrapper.alloc((prng() % 8000) + 128);
  ZeroAllocatorTest_usage();

  INFO("Releasing zeroed memory...");
  for (i = 0; i < kCount; i++)
    wrapper.release(ptrArray[i]);
  ZeroAllocatorTest_usage();

  Allocator::systemRelease(ptrArray);
}
#endif

B2D_END_NAMESPACE
