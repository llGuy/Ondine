#pragma once

#include <stdint.h>

namespace Ondine::Graphics {

class VulkanBuffer;
class VulkanCommandBuffer;

class VulkanArenaSlot {
public:
  VulkanArenaSlot() = default;

  VulkanArenaSlot(
    VulkanBuffer &buffer,
    uint32_t offset,
    uint32_t size);

  void write(
    const VulkanCommandBuffer &commandBuffer,
    const void *data, uint32_t size);

  inline uint32_t size() const {
    return mSize;
  }

private:
  // Offset will be aligned with the size of one block
  uint32_t mOffset;
  // Size will be aligned with the size of one block
  uint32_t mSize;

  VulkanBuffer *mBuffer;

  friend class VulkanArenaAllocator;
  friend class VulkanCommandBuffer;
};

}
