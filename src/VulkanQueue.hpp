#pragma once

#include <vulkan/vulkan.h>
#include "Buffer.hpp"
#include "VulkanSync.hpp"

namespace Yona {

class VulkanCommandBuffer;
class VulkanSwapchain;

class VulkanQueue {
public:
  void idle() const;

  void submitCommandBuffer(
    const VulkanCommandBuffer &commandBuffer,
    const Array<VulkanSemaphore, AllocationType::Linear> &waitSemaphores,
    const Array<VulkanSemaphore, AllocationType::Linear> &signalSemaphores,
    VkPipelineStageFlags waitStage,
    const VulkanFence &fence) const;

  VkResult present(
    const VulkanSwapchain &swapchain,
    const VulkanSemaphore &wait) const;

private:
  VkQueue mQueue;

  friend class VulkanDevice;
  friend class VulkanImgui;
};

}
