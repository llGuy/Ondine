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
    mBuffer(&buffer) {
  
}

void VulkanArenaSlot::write(
  const VulkanCommandBuffer &commandBuffer,
  const void *data, uint32_t size) {
  static const uint32_t MAX_UPDATE_SIZE = 65536;
  uint32_t updateCount = size / MAX_UPDATE_SIZE;
  uint32_t offset = 0;
  for (int i = 0; i < updateCount; ++i) {
    uint8_t *ptr = (uint8_t *)data + offset;
    commandBuffer.updateBuffer(
      *mBuffer, mOffset + offset, MAX_UPDATE_SIZE, ptr);
    offset += MAX_UPDATE_SIZE;
  }
  
  uint32_t lastUpdate = size - offset;
  if (lastUpdate) {
    uint8_t *ptr = (uint8_t *)data + offset;
    commandBuffer.updateBuffer(
      *mBuffer, mOffset + offset, lastUpdate, ptr);
  }
}

}
