#include "Vulkan.hpp"
#include "VulkanDevice.hpp"
#include "VulkanCommandPool.hpp"

namespace Ondine {

void VulkanCommandPool::init(const VulkanDevice &device) {
  VkCommandPoolCreateInfo commandPoolInfo = {};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.queueFamilyIndex = device.mQueueFamilies.graphicsFamily;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VK_CHECK(
    vkCreateCommandPool(
      device.mLogicalDevice,
      &commandPoolInfo,
      NULL,
      &mCommandPool));
}

VulkanCommandBuffer VulkanCommandPool::makeCommandBuffer(
  const VulkanDevice &device,
  VkCommandBufferLevel level) const {
  VulkanCommandBuffer commandBuffer;

  makeCommandBuffers(device, level, 1, &commandBuffer.mCommandBuffer);
  commandBuffer.init(commandBuffer.mCommandBuffer, level);
  
  return commandBuffer;
}

void VulkanCommandPool::makeCommandBuffers(
  const VulkanDevice &device,
  VkCommandBufferLevel level,
  size_t count,
  VkCommandBuffer *commandBuffers) const {
  VkCommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.level = level;
  allocateInfo.commandPool = mCommandPool;
  allocateInfo.commandBufferCount = count;

  VK_CHECK(
    vkAllocateCommandBuffers(
      device.mLogicalDevice, &allocateInfo, commandBuffers));
}

void VulkanCommandPool::freeCommandBuffer(
  const VulkanDevice &device,
  const VulkanCommandBuffer &commandBuffer) const {
  vkFreeCommandBuffers(
    device.mLogicalDevice, mCommandPool, 1, &commandBuffer.mCommandBuffer);
}

}
