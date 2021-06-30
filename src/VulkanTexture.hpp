#pragma once

#include "IO.hpp"
#include <stddef.h>
#include "Utils.hpp"
#include <vulkan/vulkan.h>

namespace Ondine::Graphics {

class VulkanDevice;

enum TextureType {
  T2D = BIT(0),
  Cubemap = BIT(1),
  T3D = BIT(2),
  Attachment = BIT(3),
  Input = BIT(4),
  StoreInRam = BIT(5),
  TransferSource = BIT(6),
  LinearTiling = BIT(7)
};

using TextureTypeBits = uint32_t;

DEFINE_BIT_OPS_FOR_ENUM_CLASS(TextureType, TextureTypeBits, uint32_t);

enum TextureContents {
  Color,
  Depth
};

class VulkanCommandPool;

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

  void fillWithStaging(
    const VulkanDevice &device,
    const VulkanCommandPool &commandPool,
    const Buffer &data);

  void destroy(const VulkanDevice &device);

  size_t memoryRequirement() const;

private:
  VkImage mImage;
  VkDeviceMemory mMemory;
  VkImageView mImageViewSample;
  VkImageView mImageViewAttachment;
  VkSampler mSampler;
  VkFormat mFormat;
  size_t mMemoryRequirement;
  VkExtent3D mExtent;
  uint32_t mLayerCount;
  uint32_t mLevelCount;
  uint32_t mViewLayerCount;
  TextureTypeBits mType;
  TextureContents mContents;
  VkImageAspectFlags mAspect;

  friend class VulkanSwapchain;
  friend class VulkanFramebufferConfig;
  friend class VulkanUniform;
  friend class VulkanCommandBuffer;
};

}
