#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

VkAccessFlags findAccessFlagsForStage(VkPipelineStageFlags stage);

class VulkanDevice;

class VulkanSemaphore {
public:
  void init(const VulkanDevice &device);

private:
  VkSemaphore mSemaphore;

  friend class VulkanQueue;
  friend class VulkanSwapchain;
};

class VulkanFence {
public:
  VulkanFence();

  void init(const VulkanDevice &device, VkFenceCreateFlags flags);
  void wait(const VulkanDevice &device);
  void reset(const VulkanDevice &device);

private:
  VkFence mFence;

  friend class VulkanQueue;
};

}
