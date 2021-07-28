#include "VulkanContext.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::Graphics {

void VulkanArenaAllocator::init(
  uint32_t maxBlockCount,
  VulkanBufferFlagBits bufferFlags,
  VulkanContext &graphicsContext) {
  mBlocks.init(maxBlockCount);
  mAllocatedSize = maxBlockCount * POOL_BLOCK_SIZE;
  mGPUPool.init(graphicsContext.device(), mAllocatedSize, bufferFlags);
  mBlocks.init(maxBlockCount);
  mBlocks[0].next = INVALID_BLOCK_INDEX;
  mBlocks[0].prev = INVALID_BLOCK_INDEX;
  mBlocks[0].blockCount = maxBlockCount;
  mBlocks[0].isFree = 1;
  mFirstFreeBlock.next = 0;
  mFirstFreeBlock.prev = INVALID_BLOCK_INDEX;
  mFirstFreeBlock.blockCount = 0;
  mFirstFreeBlock.base = 0;
  mFirstFreeBlock.isFree = 1;
  mLastFreeBlock = 0;
  setRangeTo(true, 0, maxBlockCount, 0);
}

VulkanArenaSlot VulkanArenaAllocator::allocate(uint32_t size) {
  uint32_t requiredBlocks = (uint32_t)glm::ceil(
    (float)size / (float)POOL_BLOCK_SIZE);

  uint16_t prevBlockIndex = 0;
  auto *prevBlock = &mFirstFreeBlock;

  while (getBlock(prevBlock->next)->blockCount < requiredBlocks) {
    assert(getBlock(prevBlock->next)->next != INVALID_BLOCK_INDEX);

    prevBlockIndex = prevBlock->next;
    prevBlock = getBlock(prevBlockIndex);
  }

  VulkanArenaSlot slot(
    mGPUPool,
    POOL_BLOCK_SIZE * prevBlock->next,
    POOL_BLOCK_SIZE * requiredBlocks
  );

  // This will no longer be a free block
  auto *toOccupy = getBlock(prevBlock->next);
  FreeBlock copy = *toOccupy;

  setRangeTo(
    false,
    prevBlock->next,
    prevBlock->next + requiredBlocks,
    prevBlock->next);

  toOccupy->base = prevBlock->next;
  toOccupy->blockCount = requiredBlocks;
  toOccupy->isFree = false;

  if (copy.blockCount == requiredBlocks) {
    // This block needs to disappear from the list of free blocks
    prevBlock->next = copy.next;

    if (prevBlock->next == INVALID_BLOCK_INDEX) {
      mLastFreeBlock = prevBlockIndex;
    }
    else {
      getBlock(prevBlock->next)->prev = prevBlockIndex;
    }
  }
  else {
    // We need to create a new block
    uint16_t newBlockIndex = prevBlock->next + requiredBlocks;
    FreeBlock *newBlock = getBlock(newBlockIndex);

    setRangeTo(
      true,
      prevBlock->next + requiredBlocks,
      prevBlock->next + copy.blockCount,
      prevBlock->next + requiredBlocks);

    prevBlock->next = prevBlock->next + requiredBlocks;

    newBlock->next = copy.next;
    newBlock->prev = copy.prev;
    newBlock->isFree = true;
    newBlock->blockCount = copy.blockCount - requiredBlocks;

    if (newBlock->next == INVALID_BLOCK_INDEX) {
      mLastFreeBlock = prevBlock->next;
    }

    sortFrom(newBlockIndex);
  }

  return slot;
}

void VulkanArenaAllocator::free(VulkanArenaSlot &slot) {
  uint32_t blockIndex = slot.mOffset / POOL_BLOCK_SIZE;
  FreeBlock *newFreeBlock = &mBlocks[blockIndex];
  uint32_t newFreeBlockCount = newFreeBlock->blockCount;

  bool createNewBlock = true;

  if (blockIndex > 0) {
    // Check if the previous block adjacent is free
    FreeBlock *prev = getBlock(blockIndex - 1);
    if (prev->isFree) {
      // Need to merge with this block
      setRangeTo(
        true,
        blockIndex,
        blockIndex + newFreeBlock->blockCount,
        prev->base);

      uint16_t newBaseIndex = prev->base;
      FreeBlock *newBase = getBlock(prev->base);
      newBase->blockCount = newBase->blockCount + newFreeBlockCount;

      createNewBlock = false;

      sortFrom(newBaseIndex);
    }
  }

  if (createNewBlock) {
    setRangeTo(
      true,
      blockIndex,
      blockIndex + newFreeBlock->blockCount,
      blockIndex);

    // Need to add a new free block
    FreeBlock *last = getBlock(mLastFreeBlock);
    last->next = blockIndex;
    newFreeBlock->prev = mLastFreeBlock;
    newFreeBlock->next = 0xFFFF;
    newFreeBlock->isFree = true;
    newFreeBlock->base = blockIndex;
    newFreeBlock->blockCount = newFreeBlockCount;

    mLastFreeBlock = blockIndex;

    sortFrom(blockIndex);
  }

  slot.mSize = 0;
  slot.mOffset = 0;
  slot.mBuffer = nullptr;
}

void VulkanArenaAllocator::debugLogState() {
  uint32_t totalFreeBlockCount = 0;
  uint32_t freeSectionsCount = 0;

  FreeBlock *freeBlock = getBlock(mFirstFreeBlock.next);
  
  while (freeBlock) {
    totalFreeBlockCount += freeBlock->blockCount;
    freeSectionsCount++;
    freeBlock = getBlock(freeBlock->next);
  }

  LOG_INFOV(
    "There are %u free blocks left (%u bytes out of %u) in %u contiguous segments\n",
    totalFreeBlockCount,
    totalFreeBlockCount * POOL_BLOCK_SIZE,
    mAllocatedSize,
    freeSectionsCount);

  /*
  printf("\n");
  LOG_INFO("--- LOGGING ARENA ALLOCATOR STATE ---\n");
  LOG_INFOV(
    "First free block at %p\n",
    (void *)(uint64_t)(mFirstFreeBlock.next * POOL_BLOCK_SIZE));
  LOG_INFOV(
    "Last free block at %p\n",
    (void *)(uint64_t)(mLastFreeBlock * POOL_BLOCK_SIZE));
  printf("\n");

  for (int i = 0; i < mBlocks.capacity; ++i) {
    mBlocks[i].log(i * POOL_BLOCK_SIZE);
    printf("\n");
  }

  // Print the list of block counts
  FreeBlock *block = getBlock(mFirstFreeBlock.next);
  while (block) {
    printf("%d -> ", block->blockCount);
    block = getBlock(block->next);
  }
  */
}

void VulkanArenaAllocator::setRangeTo(
  bool isFree, uint16_t start, uint16_t end, uint16_t base) {
  for (int i = start; i < end; ++i) {
    mBlocks[i].isFree = isFree;
    mBlocks[i].base = base;
  }
}

VulkanArenaAllocator::FreeBlock *VulkanArenaAllocator::getBlock(uint32_t index) {
  if (index == INVALID_BLOCK_INDEX) {
    return nullptr;
  }
  else {
    return &mBlocks[index];
  }
}

// a's next block needs to be b
void VulkanArenaAllocator::swapBlockOrder(uint16_t aIndex, uint16_t bIndex) {
  FreeBlock *a = getBlock(aIndex);
  FreeBlock *b = getBlock(bIndex);

  FreeBlock aCopy = *a;
  FreeBlock bCopy = *b;

  b->prev = a->prev;
  a->next = b->next;

  b->next = aIndex;
  a->prev = bIndex;

  FreeBlock *after = getBlock(a->next);
  FreeBlock *before = getBlock(b->prev);

  if (after) {
    after->prev = aIndex;
  }
  if (before) {
    before->next = bIndex;
  }
}

void VulkanArenaAllocator::sortFrom(uint16_t blockIndex) {
  uint16_t originalBlockIndex = blockIndex;
  FreeBlock *original = getBlock(blockIndex);
  FreeBlock *current = original;
  /* 
     This will run if the new block isn't already sorted in one direction
  */
  while (blockIndex != INVALID_BLOCK_INDEX) {
    if (current->prev == INVALID_BLOCK_INDEX) {
      break;
    }

    uint16_t prevIndex = current->prev;
    FreeBlock *prev = getBlock(current->prev);
    if (prev->blockCount > current->blockCount) {
      swapBlockOrder(current->prev, blockIndex);

      if (current->next == INVALID_BLOCK_INDEX) {
        mLastFreeBlock = blockIndex;
      }
      else if (prev->next == INVALID_BLOCK_INDEX) {
        mLastFreeBlock = prevIndex;
      }

      if (current->prev == INVALID_BLOCK_INDEX) {
        mFirstFreeBlock.next = blockIndex;
      }
      else if (prev->prev == INVALID_BLOCK_INDEX) {
        mFirstFreeBlock.next = prevIndex;
      }
    }
    else {
      break;
    }
  }

  current = original;
  blockIndex = originalBlockIndex;
  /* 
     This is will run if the new block is sorted in the other direction
     but maybe not in the other
  */
  while (blockIndex != INVALID_BLOCK_INDEX) {
    if (current->next == INVALID_BLOCK_INDEX) {
      break;
    }

    uint16_t nextIndex = current->next;
    FreeBlock *next = getBlock(current->next);
    if (next->blockCount < current->blockCount) {
      swapBlockOrder(blockIndex, current->next);

      if (current->next == INVALID_BLOCK_INDEX) {
        mLastFreeBlock = blockIndex;
      }
      else if (next->next == INVALID_BLOCK_INDEX) {
        mLastFreeBlock = nextIndex;
      }

      if (current->prev == INVALID_BLOCK_INDEX) {
        mFirstFreeBlock.next = blockIndex;
      }
      else if (next->prev == INVALID_BLOCK_INDEX) {
        mFirstFreeBlock.next = nextIndex;
      }
    }
    else {
      break;
    }
  }
}

}
