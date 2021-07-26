#pragma once

#include "Log.hpp"
#include <stdint.h>
#include "Buffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanArenaSlot.hpp"

namespace Ondine::Graphics {

class VulkanContext;

class VulkanArenaAllocator {
public:
  void init(
    uint32_t maxBlockCount,
    VulkanBufferFlagBits bufferFlags,
    VulkanContext &graphicsContext);

  VulkanArenaSlot allocate(uint32_t size);
  void free(VulkanArenaSlot &slot);

  void debugLogState();

private:
  struct FreeBlock {
    // Might need to add more information in bitfield
    uint16_t next;
    uint16_t prev;
    uint16_t base;
    struct {
      uint16_t isFree : 1;
      uint16_t blockCount : 15;
    };

    void log(uint32_t address) {
      if (isFree) {
        LOG_INFOV("Free block at %p\n", (void *)(uint64_t)address);
        if (blockCount) {
          LOG_INFOV("\tBase of %d blocks\n", blockCount);
          LOG_INFOV(
            "\tNext free block at %p\n",
            (void *)((uint64_t)next * POOL_BLOCK_SIZE));
          LOG_INFOV(
            "\tPrevious free block at %p\n",
            (void *)((uint64_t)prev * POOL_BLOCK_SIZE));
        }
        else {
          LOG_INFOV(
            "\tRoot at %p\n",
            (void *)((uint64_t)base * POOL_BLOCK_SIZE));
        }
      }
      else {
        LOG_INFOV("Occupied block at %p\n", (void *)(uint64_t)address);
        if (blockCount) {
          LOG_INFOV("\tBase of %d blocks\n", blockCount);
        }
        else {
          LOG_INFOV(
            "\tRoot at %p\n",
            (void *)((uint64_t)base * POOL_BLOCK_SIZE));
        }
      }
    }
  };

  void setRangeTo(bool isFree, uint16_t start, uint16_t end, uint16_t base);
  FreeBlock *getBlock(uint32_t index);

  void swapBlockOrder(uint16_t aIndex, uint16_t bIndex);
  // Sorts the linked list from a, backwards until the beginning
  void sortFrom(uint16_t blockIndex);

private:
  static constexpr uint32_t POOL_BLOCK_SIZE = 4096;
  static constexpr uint32_t INVALID_BLOCK_INDEX = 0xFFFF;

  uint32_t mAllocatedSize;
  VulkanBuffer mGPUPool;
  // Sorted linked list (smallest to largest)
  FreeBlock mFirstFreeBlock;
  // Index to the block whose nextFreeBlock member is equal to 0xFFFF
  uint16_t mLastFreeBlock;
  Array<FreeBlock> mBlocks;
};

}
