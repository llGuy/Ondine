#include "yona_vulkan.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_texture.hpp"

namespace Yona {

void VulkanTexture::init(
  const VulkanDevice &device,
  TextureType type, TextureContents contents, VkFormat format,
  VkFilter filter, VkExtent3D extent, size_t layerCount,
  size_t mipLevels) {
  mType = type;

  VkImageType imageType;
  VkImageCreateFlags imageFlags = 0;
  VkImageViewType viewType;

  switch (type) {
  case TextureType::T2D: {
    imageType = VK_IMAGE_TYPE_2D;
    if (layerCount == 1) {
      viewType = VK_IMAGE_VIEW_TYPE_2D;
    }
    else {
      viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
  } break;

  case TextureType::T3D: {
    imageType = VK_IMAGE_TYPE_3D;
    viewType = VK_IMAGE_VIEW_TYPE_3D;
  } break;

  case TextureType::Cubemap: {
    imageType = VK_IMAGE_TYPE_2D;
    imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  } break;
  }

  VkImageAspectFlags aspect;
  switch (contents) {
  case TextureContents::Color: {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  } break;

  case TextureContents::Depth: {
    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  }

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.flags = imageFlags;
  imageInfo.arrayLayers = layerCount;
  imageInfo.extent = extent;
  imageInfo.format = format;
  imageInfo.imageType = imageType;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.mipLevels = mipLevels;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(
    vkCreateImage(device.mLogicalDevice, &imageInfo, NULL, &mImage));

  mMemory = device.allocateImageMemory(
    mImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageViewCreateInfo imageViewInfo = {};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = mImage;
  imageViewInfo.viewType = viewType;
  imageViewInfo.format = format;
  imageViewInfo.subresourceRange.aspectMask = aspect;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = mipLevels;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = layerCount;

  VK_CHECK(
    vkCreateImageView(device.mLogicalDevice, &imageViewInfo, NULL, &mImageView));

  VkSamplerCreateInfo samplerInfo = {};
  // In future may need to change this
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = filter;
  samplerInfo.minFilter = filter;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  VK_CHECK(
    vkCreateSampler(device.mLogicalDevice, &samplerInfo, NULL, &mSampler));
}

}
