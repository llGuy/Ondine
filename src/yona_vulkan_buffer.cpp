#include "yona_vulkan.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_buffer.hpp"
#include "yona_vulkan_command_pool.hpp"
#include "yona_vulkan_command_buffer.hpp"

namespace Yona {

void VulkanBuffer::init(
  const VulkanDevice &device,
  size_t size,
  VulkanBufferFlagBits type) {
  VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  mUsedAt = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  mUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  if (type & VulkanBufferFlag::VertexBuffer) {
    mUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    mUsedAt = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  }
  if (type & VulkanBufferFlag::IndexBuffer) {
    mUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    mUsedAt = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  }
  if (type & VulkanBufferFlag::UniformBuffer) {
    mUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    mUsedAt = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
      VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  if (type & VulkanBufferFlag::Mappable) {
    memoryFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  }
  if (type & VulkanBufferFlag::TransferSource) {
    mUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    mUsedAt = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }

  mSize = size;

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = mSize;
  bufferInfo.usage = mUsage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device.mLogicalDevice, &bufferInfo, NULL, &mBuffer));

  mMemory = device.allocateBufferMemory(mBuffer, memoryFlags);
}

void VulkanBuffer::destroy(const VulkanDevice &device) {
  vkFreeMemory(device.mLogicalDevice, mMemory, nullptr);
  vkDestroyBuffer(device.mLogicalDevice, mBuffer, nullptr);
}

void VulkanBuffer::fillWithStaging(
  const VulkanDevice &device,
  const VulkanCommandPool &commandPool,
  const Buffer &data) {
  VulkanBuffer stagingBuffer;
  stagingBuffer.init(
    device, data.size,
    VulkanBufferFlag::Mappable | VulkanBufferFlag::TransferSource);

  void *mappedMemory = stagingBuffer.map(device, data.size, 0);
  memcpy(mappedMemory, data.data, data.size);
  stagingBuffer.unmap(device);

  VulkanCommandBuffer commandBuffer = commandPool.makeCommandBuffer(
    device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  commandBuffer.copyBuffer(
    *this, 0, stagingBuffer, 0, data.size);
  commandBuffer.end();

  device.mGraphicsQueue.submitCommandBuffer(
    commandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());

  device.mGraphicsQueue.idle();

  commandPool.freeCommandBuffer(device, commandBuffer);

  stagingBuffer.destroy(device);
}

void *VulkanBuffer::map(
  const VulkanDevice &device, size_t size, size_t offset) const {
  void *ptr;
  vkMapMemory(device.mLogicalDevice, mMemory, offset, size, 0, &ptr);
  return ptr;
}

void VulkanBuffer::unmap(const VulkanDevice &device) const {
  vkUnmapMemory(device.mLogicalDevice, mMemory);
}

VkBufferMemoryBarrier VulkanBuffer::makeBarrier(
  VkPipelineStageFlags src,
  VkPipelineStageFlags dst,
  size_t offset, size_t size) const {
  VkBufferMemoryBarrier bufferBarrier = {};
  bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.buffer = mBuffer;
  bufferBarrier.size = size;
  bufferBarrier.offset = offset;
  bufferBarrier.size = mSize;
  bufferBarrier.srcAccessMask = findAccessFlagsForStage(src);
  bufferBarrier.dstAccessMask = findAccessFlagsForStage(dst);

  return bufferBarrier;
}

}
