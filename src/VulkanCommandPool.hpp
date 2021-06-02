#pragma once

#include "Utils.hpp"
#include <vulkan/vulkan.h>
#include "Buffer.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Ondine {

class VulkanDevice;

class VulkanCommandPool {
public:
  void init(const VulkanDevice &device);

  VulkanCommandBuffer makeCommandBuffer(
    const VulkanDevice &device,
    VkCommandBufferLevel level) const;

  template <AllocationType A>
  void makeCommandBuffers(
    const VulkanDevice &device,
    VkCommandBufferLevel level,
    Array<VulkanCommandBuffer, A> &commandBuffers) const {
    size_t count = MAX(commandBuffers.size, commandBuffers.capacity);

    VkCommandBuffer *ptr = STACK_ALLOC(
      VkCommandBuffer,
      // Just precaution
      count);

    makeCommandBuffers(device, level, count, ptr);

    for (int i = 0; i < count; ++i) {
      commandBuffers[i].init(ptr[i], level);
    }
  }

  void freeCommandBuffer(
    const VulkanDevice &device,
    const VulkanCommandBuffer &commandBuffer) const;

private:
  void makeCommandBuffers(
    const VulkanDevice &device,
    VkCommandBufferLevel level,
    size_t count,
    VkCommandBuffer *commandBuffers) const;

private:
  VkCommandPool mCommandPool;
};

}
