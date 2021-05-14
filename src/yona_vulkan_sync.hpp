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
};

class VulkanFence {
public:
  void init(const VulkanDevice &device, VkFenceCreateFlags flags);

private:
  VkFence mFence;
};

}
