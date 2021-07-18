#include "VulkanContext.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::Graphics {

void VulkanArenaAllocator::init(
  uint32_t maxBlockCount,
  VulkanBufferFlagBits bufferFlags,
  VulkanContext &graphicsContext) {
  mFreeBlocks.init(maxBlockCount);
  mAllocatedSize = maxBlockCount * POOL_BLOCK_SIZE;
  mGPUPool.init(graphicsContext.device(), mAllocatedSize, bufferFlags);
  mFreeBlocks.init(maxBlockCount);
  mFreeBlocks[0].nextFreeBlock = 0xFFFF;
  mFreeBlocks[0].blockCount = maxBlockCount;
  mFirstFreeBlock.nextFreeBlock = 0;
  mFirstFreeBlock.blockCount = 0;
  mFirstFreeBlock.baseBlock = 0;
  mFirstFreeBlock.isFree = 1;
  mLastFreeBlock = 0;
  setRangeTo(true, 0, maxBlockCount, 0);
}

VulkanArenaSlot VulkanArenaAllocator::allocate(uint32_t size) {
  uint32_t requiredBlocks = (uint32_t)glm::ceil(
    (float)size / (float)POOL_BLOCK_SIZE);

  uint16_t prevBlockIndex = 0;
  auto *prevBlock = &mFirstFreeBlock;

  while (mFreeBlocks[prevBlock->nextFreeBlock].blockCount < requiredBlocks) {
    assert(
      mFreeBlocks[prevBlock->nextFreeBlock].nextFreeBlock !=
      INVALID_BLOCK_INDEX);

    prevBlock = &mFreeBlocks[
      mFreeBlocks[
        prevBlock->nextFreeBlock
      ].nextFreeBlock
    ];

    prevBlockIndex = prevBlock->nextFreeBlock;
  }

  VulkanArenaSlot slot = {
    POOL_BLOCK_SIZE * prevBlock->nextFreeBlock,
    POOL_BLOCK_SIZE * requiredBlocks
  };

  // This will no longer be a free block
  auto *oldFreeBlock = &mFreeBlocks[prevBlock->nextFreeBlock];

  if (oldFreeBlock->blockCount == requiredBlocks) {
    setRangeTo(
      false,
      prevBlock->nextFreeBlock,
      oldFreeBlock->nextFreeBlock,
      prevBlock->nextFreeBlock);
    prevBlock->nextFreeBlock = oldFreeBlock->nextFreeBlock;

    if (prevBlock->nextFreeBlock == INVALID_BLOCK_INDEX) {
      mLastFreeBlock = prevBlockIndex;
    }
  }
  else {
    setRangeTo(
      false,
      prevBlock->nextFreeBlock,
      prevBlock->nextFreeBlock + requiredBlocks,
      prevBlock->nextFreeBlock);
    
    prevBlock->nextFreeBlock = prevBlock->nextFreeBlock + requiredBlocks;

    auto *newFreeBlock = &mFreeBlocks[prevBlock->nextFreeBlock];

    newFreeBlock->blockCount = oldFreeBlock->blockCount - requiredBlocks;
    newFreeBlock->nextFreeBlock = oldFreeBlock->nextFreeBlock;

    if (newFreeBlock->nextFreeBlock == INVALID_BLOCK_INDEX) {
      // Index of newFreeBlock
      mLastFreeBlock = prevBlock->nextFreeBlock;
    }

    setRangeTo(
      true,
      prevBlock->nextFreeBlock,
      prevBlock->nextFreeBlock + newFreeBlock->blockCount,
      prevBlock->nextFreeBlock);
  }

  memset(oldFreeBlock, 0, sizeof(FreeBlock));

  return slot;
}

void VulkanArenaAllocator::free(uint32_t address) {
  uint32_t blockIndex = address / POOL_BLOCK_SIZE;
  FreeBlock *newFreeBlock = &mFreeBlocks[blockIndex];
  
  { // Check if the previous block adjacent is free
    FreeBlock &prev = mFreeBlocks[blockIndex - 1];
    if (prev.isFree) {
      // Need to merge with this block
      setRangeTo(
        true,
        blockIndex,
        blockIndex + newFreeBlock->blockCount,
        prev.baseBlock);
    }
  }
}

void VulkanArenaAllocator::debugLogState() {
  FreeBlock *prevBlock = &mFirstFreeBlock;

  printf("\n");
  LOG_INFOV(
    "First free block at %p\n",
    (void *)(uint64_t)(mFirstFreeBlock.nextFreeBlock * POOL_BLOCK_SIZE));
  LOG_INFOV(
    "Last free block at %p\n",
    (void *)(uint64_t)(mLastFreeBlock * POOL_BLOCK_SIZE));
  printf("\n");

  for (int i = 0; i < mFreeBlocks.capacity; ++i) {
    mFreeBlocks[i].log(i * POOL_BLOCK_SIZE);
    printf("\n");
  }
}

void VulkanArenaAllocator::setRangeTo(
  bool isFree, uint16_t start, uint16_t end, uint16_t base) {
  for (int i = start; i < end; ++i) {
    mFreeBlocks[i].isFree = isFree;
    mFreeBlocks[i].baseBlock = base;
  }
}

}
