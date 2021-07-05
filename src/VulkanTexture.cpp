#include "Utils.hpp"
#include "Vulkan.hpp"
#include "VulkanSync.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"
#include "VulkanCommandPool.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Ondine::Graphics {

void VulkanTexture::init(
  const VulkanDevice &device,
  TextureTypeBits type, TextureContents contents, VkFormat format,
  VkFilter filter, VkExtent3D extent, size_t layerCount,
  size_t mipLevels) {
  mExtent = extent;
  mType = type;
  mLayerCount = layerCount;
  mContents  = contents;
  mLevelCount = mipLevels;
  mFormat = format;

  mViewLayerCount = mLayerCount;

  VkImageType imageType;
  VkImageCreateFlags imageFlags = 0;
  VkImageViewType viewTypeSample, viewTypeAttachment;
  VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkImageTiling tilingMode = VK_IMAGE_TILING_OPTIMAL;
  VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  if (type & (TextureType::WrapSampling)) {
    type &= ~(TextureType::WrapSampling);
    addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }

  if (type & (TextureType::LinearTiling)) {
    type &= ~(TextureType::LinearTiling);
    tilingMode = VK_IMAGE_TILING_LINEAR;
  }

  VkImageUsageFlags usage =
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  if (type & TextureType::TransferSource) {
    type &= ~(TextureType::TransferSource);
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (type & TextureType::Attachment) {
    type &= ~(TextureType::Attachment);

    // Yes I know we are doing this switch statement but who cares
    switch (contents) {
    case TextureContents::Color: {
      usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    } break;

    case TextureContents::Depth: {
      usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } break;
    }
  }

  if (type & TextureType::Input) {
    type &= ~(TextureType::Input);
    usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }

  if (type & TextureType::StoreInRam) {
    type &= ~(TextureType::StoreInRam);
    memoryFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  }

  switch (type) {
  case TextureType::T2D: {
    imageType = VK_IMAGE_TYPE_2D;
    if (layerCount == 1) {
      viewTypeAttachment = viewTypeSample = VK_IMAGE_VIEW_TYPE_2D;
    }
    else {
      viewTypeAttachment = viewTypeSample = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
  } break;

  case TextureType::T3D: {
    imageType = VK_IMAGE_TYPE_3D;
    // May need to change this to 3D or have multiple views
    viewTypeAttachment = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewTypeSample = VK_IMAGE_VIEW_TYPE_3D;
    imageFlags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    mViewLayerCount = extent.depth;
  } break;

  case TextureType::Cubemap: {
    imageType = VK_IMAGE_TYPE_2D;
    imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    viewTypeSample = viewTypeAttachment = VK_IMAGE_VIEW_TYPE_CUBE;
  } break;
  }

  switch (contents) {
  case TextureContents::Color: {
    mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
  } break;

  case TextureContents::Depth: {
    mAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
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
  imageInfo.tiling = tilingMode;
  imageInfo.usage = usage;
  imageInfo.extent = extent;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(
    vkCreateImage(device.mLogicalDevice, &imageInfo, NULL, &mImage));

  mMemory = device.allocateImageMemory(
    mImage, memoryFlags, &mMemoryRequirement);

  VkImageViewCreateInfo imageViewInfo = {};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = mImage;
  imageViewInfo.viewType = viewTypeSample;
  imageViewInfo.format = format;
  imageViewInfo.subresourceRange.aspectMask = mAspect;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = mipLevels;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = mLayerCount;

  VK_CHECK(
    vkCreateImageView(
      device.mLogicalDevice, &imageViewInfo, NULL, &mImageViewSample));

  if (type == TextureType::T3D) {
    imageViewInfo.viewType = viewTypeAttachment;
    imageViewInfo.subresourceRange.layerCount = mViewLayerCount;

    VK_CHECK(
      vkCreateImageView(
        device.mLogicalDevice, &imageViewInfo, NULL, &mImageViewAttachment));
  }
  else {
    mImageViewAttachment = mImageViewSample;
  }

  VkSamplerCreateInfo samplerInfo = {};
  // In future may need to change this
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = filter;
  samplerInfo.minFilter = filter;
  samplerInfo.addressModeU = addressMode;
  samplerInfo.addressModeV = addressMode;
  samplerInfo.addressModeW = addressMode;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  VK_CHECK(
    vkCreateSampler(device.mLogicalDevice, &samplerInfo, NULL, &mSampler));
}


void VulkanTexture::initFromFile(
  const VulkanDevice &device,
  const VulkanCommandPool &commandPool,
  const char *imagePath,
  TextureTypeBits type, TextureContents contents, VkFormat format,
  VkFilter filter, size_t mipLevels) {
  Core::File imageFile = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    imagePath,
    Core::FileOpenType::Binary | Core::FileOpenType::In);
  Buffer unparsed = imageFile.readBinary();
  ImagePixels parsed = getImagePixelsFromBuffer(unparsed);

  init(
    device, type, contents, format, filter,
    {(uint32_t)parsed.width, (uint32_t)parsed.height, 1}, 1, mipLevels);

  fillWithStaging(
    device,
    commandPool,
    {(uint8_t *)parsed.data, (uint32_t)(parsed.width * parsed.height * 4)});
}

VkImageMemoryBarrier VulkanTexture::makeBarrier(
  VkImageLayout oldLayout,
  VkImageLayout newLayout) const {
  VkImageMemoryBarrier imageBarrier = {};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.srcAccessMask = findAccessFlagsForImageLayout(oldLayout);
  imageBarrier.dstAccessMask = findAccessFlagsForImageLayout(newLayout);
  imageBarrier.oldLayout = oldLayout;
  imageBarrier.newLayout = newLayout;
  imageBarrier.image = mImage;

  switch (mContents) {
  case TextureContents::Color: {
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  } break;

  case TextureContents::Depth: {
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  }

  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = mLevelCount;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = mLayerCount;

  return imageBarrier;
}

void VulkanTexture::destroy(const VulkanDevice &device) {
  vkDestroyImage(device.mLogicalDevice, mImage, nullptr);
  vkFreeMemory(device.mLogicalDevice, mMemory, nullptr);
  vkDestroyImageView(device.mLogicalDevice, mImageViewSample, nullptr);
  if (mImageViewSample != mImageViewAttachment &&
      mImageViewAttachment != VK_NULL_HANDLE) {
    vkDestroyImageView(device.mLogicalDevice, mImageViewAttachment, nullptr);
  }
  vkDestroySampler(device.mLogicalDevice, mSampler, nullptr);

  mImage = VK_NULL_HANDLE;
  mImageViewSample = VK_NULL_HANDLE;
  mImageViewAttachment = VK_NULL_HANDLE;
  mSampler = VK_NULL_HANDLE;
  mMemory = VK_NULL_HANDLE;
}

size_t VulkanTexture::memoryRequirement() const {
  return mMemoryRequirement;
}

void VulkanTexture::fillWithStaging(
  const VulkanDevice &device,
  const VulkanCommandPool &commandPool,
  const Buffer &data) {
  // ImagePixels pixels = ;

  VulkanBuffer stagingBuffer;

  stagingBuffer.init(
    device, data.size,
    VulkanBufferFlag::Mappable | VulkanBufferFlag::TransferSource);

  void *mappedMemory = stagingBuffer.map(device, data.size, 0);
  memcpy(mappedMemory, data.data, data.size);
  stagingBuffer.unmap(device);

  VulkanCommandBuffer commandBuffer = commandPool.makeCommandBuffer(
    device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  commandBuffer.copyBufferToImage(
    *this, 0, mLayerCount, 0,
    stagingBuffer, 0, data.size);

  commandBuffer.transitionImageLayout(
    *this,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  commandBuffer.end();

  device.mGraphicsQueue.submitCommandBuffer(
    commandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());

  device.mGraphicsQueue.idle();

  commandPool.freeCommandBuffer(device, commandBuffer);

  stagingBuffer.destroy(device);
}

}
