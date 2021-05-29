#pragma once

#include <stddef.h>
#include "yona_io.hpp"
#include "yona_utils.hpp"
#include <vulkan/vulkan.h>

namespace Yona {

class VulkanDevice;

enum TextureType {
  T2D = BIT(0),
  Cubemap = BIT(1),
  T3D = BIT(2),
  Attachment = BIT(3),
  Input = BIT(4),
  StoreInRam = BIT(5)
};

using TextureTypeBits = uint32_t;

DEFINE_BIT_OPS_FOR_ENUM_CLASS(TextureType, TextureTypeBits, uint32_t);

enum TextureContents {
  Color,
  Depth
};

class VulkanTexture {
public:
  VulkanTexture() = default;

  void init(
    const VulkanDevice &device,
    TextureTypeBits type, TextureContents contents, VkFormat format,
    VkFilter filter, VkExtent3D extent, size_t layerCount,
    size_t mipLevels);

  VkImageMemoryBarrier makeBarrier(
    VkImageLayout oldLayout,
    VkImageLayout newLayout) const;

  void destroy(const VulkanDevice &device);

private:
  VkImage mImage;
  VkDeviceMemory mMemory;
  VkImageView mImageViewSample;
  VkImageView mImageViewAttachment;
  VkSampler mSampler;
  VkExtent3D mExtent;
  uint32_t mLayerCount;
  uint32_t mLevelCount;
  uint32_t mViewLayerCount;
  TextureTypeBits mType;
  TextureContents mContents;

  friend class VulkanSwapchain;
  friend class VulkanFramebufferConfig;
  friend class VulkanUniform;
  friend class VulkanCommandBuffer;
};

}
