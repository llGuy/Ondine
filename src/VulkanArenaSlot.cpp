#include "VulkanBuffer.hpp"
#include "VulkanArenaSlot.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Ondine::Graphics {

VulkanArenaSlot:: VulkanArenaSlot(
  VulkanBuffer &buffer,
  uint32_t offset,
  uint32_t size)
  : mOffset(offset),
    mSize(size),
    mBuffer(buffer) {
  
}

void VulkanArenaSlot::write(
  const VulkanCommandBuffer &commandBuffer,
  const void *data, uint32_t size) {
  commandBuffer.updateBuffer(mBuffer, mOffset, mSize, data);
}

}
