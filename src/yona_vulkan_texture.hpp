#pragma once

#include "yona_io.hpp"
#include <vulkan/vulkan.h>

namespace Yona {

class VulkanTexture {
public:
  VulkanTexture() = default;

private:
  VkImage mImage;
  VkDeviceMemory mMemory;
  VkImageView mImageView;
  VkSampler mSampler;
  Resolution mResolution;
  uint32_t mLayerCount;

  friend class VulkanSwapchain;
  friend class VulkanFramebufferConfig;
};

}
