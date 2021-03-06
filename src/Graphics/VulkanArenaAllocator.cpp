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

#if 0
#define DBG_CHECK_POINTER(blockIndex, block)            \
  if (block) {                                          \
    if (block->prev == 0 && block->base != 197) {       \
      asm("int $3");                                    \
    }                                                   \
}
#else
#define DBG_CHECK_POINTER(b, b0)
#endif

VulkanArenaSlot VulkanArenaAllocator::allocate(uint32_t size) {
  uint32_t requiredBlocks = (uint32_t)glm::ceil(
    (float)size / (float)POOL_BLOCK_SIZE);

  uint16_t prevBlockIndex = 0;
  /* 
     We set mFirstFreeBlock as the previous block of the first because
     it's not actually a block
  */
  auto *prevBlock = &mFirstFreeBlock;

  /*
     We can allow ourselves to simply loop through like this to find the
     smallest possible group of blocks which could hold the the allocation
  */
  while (getBlock(prevBlock->next)->blockCount < requiredBlocks) {
    assert(getBlock(prevBlock->next)->next != INVALID_BLOCK_INDEX);

    prevBlockIndex = prevBlock->next;
    prevBlock = getBlock(prevBlockIndex);
  }

  uint16_t allocationStart = prevBlock->next; // aka. current block

  /* The slot which gets returned back to the calling function */
  VulkanArenaSlot slot(
    mGPUPool,
    POOL_BLOCK_SIZE * allocationStart,
    POOL_BLOCK_SIZE * requiredBlocks
  );

  /* This will no longer be a free block */
  auto *toOccupy = getBlock(allocationStart);

  assert(toOccupy->isFree);

  /* Holds information about the free block which previously occupied the slot */
  FreeBlock oldFreeInfo = *toOccupy;

  setRangeTo(
    false,
    allocationStart,
    allocationStart + requiredBlocks,
    allocationStart);

  toOccupy->base = allocationStart;
  toOccupy->blockCount = requiredBlocks;
  toOccupy->isFree = false;

  if (oldFreeInfo.blockCount == requiredBlocks) {
    /* 
       This block needs to disappear from the list of free blocks - we do this
       by setting the previous free block's next pointer to the old free block's
       next pointer
       We also need to set the next free block's prev pointer to the old free
       block's prev pointer
    */
    prevBlock->next = oldFreeInfo.next;

    /* 
       We need to make sure the next free block is valid before settings its 
       prev pointer
    */
    if (oldFreeInfo.next == INVALID_BLOCK_INDEX) {
      /* If the next pointer is invalid, this is the last free block */
      mLastFreeBlock = oldFreeInfo.prev;
    }
    else {
      /* Set the next block's prev pointer to the previous block of old block */
      getBlock(prevBlock->next)->prev = oldFreeInfo.prev;
    }

    DBG_CHECK_POINTER(prevBlockIndex, (prevBlock));
  }
  else {
    /* We need to create a new block */
    uint16_t newBlockIndex = allocationStart + requiredBlocks;
    FreeBlock *newBlock = getBlock(newBlockIndex);

    setRangeTo(
      true,
      allocationStart + requiredBlocks,
      allocationStart + oldFreeInfo.blockCount,
      allocationStart + requiredBlocks);

    /* Update the previous block */
    prevBlock->next = prevBlock->next + requiredBlocks;

    newBlock->next = oldFreeInfo.next;
    newBlock->prev = oldFreeInfo.prev;
    newBlock->isFree = true;
    newBlock->blockCount = oldFreeInfo.blockCount - requiredBlocks;

    /* Update the next block */
    if (newBlock->next == INVALID_BLOCK_INDEX) {
      mLastFreeBlock = prevBlock->next; // newBlockIndex
    }
    else {
      getBlock(newBlock->next)->prev = newBlockIndex;
    }

    DBG_CHECK_POINTER(prevBlockIndex, (prevBlock));
    DBG_CHECK_POINTER(newBlockIndex, (newBlock));

    sortFrom(newBlockIndex);
  }

  return slot;
}

void VulkanArenaAllocator::free(VulkanArenaSlot &slot) {
  uint32_t blockIndex = slot.mOffset / POOL_BLOCK_SIZE;
  FreeBlock *newFreeBlock = &mBlocks[blockIndex];

  assert(!newFreeBlock->isFree);

  bool createNewBlock = true;

  /* Is there a block before this new free one? */
  if (blockIndex > 0) {
    FreeBlock *prev = getBlock(blockIndex - 1);

    /* Now is this block free? */
    if (prev->isFree) {
      uint16_t newBaseIndex = prev->base;

      /* Need to merge with the adjacent block before this one */
      setRangeTo(
        true,
        blockIndex,
        blockIndex + newFreeBlock->blockCount,
        newBaseIndex);

      FreeBlock *newBase = getBlock(newBaseIndex);
      newBase->blockCount = newBase->blockCount + newFreeBlock->blockCount;

      createNewBlock = false;

      sortFrom(newBaseIndex);

      /* This is in case we need to merge with the free blocks after */
      newFreeBlock = newBase;
      blockIndex = newBaseIndex;
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

    DBG_CHECK_POINTER(mLastFreeBlock, (last));
    DBG_CHECK_POINTER(blockIndex, (newFreeBlock));

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

  uint32_t index = mFirstFreeBlock.next;
  FreeBlock *freeBlock = getBlock(mFirstFreeBlock.next);
  
  while (freeBlock) {
#if 0
    LOG_INFOV(
      "%d free blocks blocks at %p\n",
      freeBlock->blockCount, (void *)index);
#endif

    totalFreeBlockCount += freeBlock->blockCount;
    freeSectionsCount++;

    index = freeBlock->next;
    freeBlock = getBlock(freeBlock->next);
  }

#if 0
  LOG_INFOV(
    "There are %u free blocks left (%u bytes out of %u) in %u contiguous segments\n",
    totalFreeBlockCount,
    totalFreeBlockCount * POOL_BLOCK_SIZE,
    mAllocatedSize,
    freeSectionsCount);
#endif

#if 0
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
#endif
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

  FreeBlock afterCopy;
  FreeBlock beforeCopy;

  if (after) {
    afterCopy = *after;
    after->prev = aIndex;
  }
  if (before) {
    beforeCopy = *before;
    before->next = bIndex;
  }

  DBG_CHECK_POINTER(a->next, (after));
  DBG_CHECK_POINTER(b->prev, (before));
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
