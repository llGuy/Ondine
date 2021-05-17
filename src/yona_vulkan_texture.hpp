#pragma once

#include <stddef.h>
#include "yona_io.hpp"
#include <vulkan/vulkan.h>

namespace Yona {

class VulkanDevice;

enum TextureType {
  T2D,
  Cubemap,
  T3D
};

enum TextureContents {
  Color,
  Depth
};

class VulkanTexture {
public:
  VulkanTexture() = default;

  void init(
    const VulkanDevice &device,
    TextureType type, TextureContents contents, VkFormat format,
    VkFilter filter, VkExtent3D extent, size_t layerCount,
    size_t mipLevels);

private:
  VkImage mImage;
  VkDeviceMemory mMemory;
  VkImageView mImageView;
  VkSampler mSampler;
  Resolution mResolution;
  uint32_t mLayerCount;
  TextureType mType;

  friend class VulkanSwapchain;
  friend class VulkanFramebufferConfig;
  friend class VulkanUniform;
};

}
