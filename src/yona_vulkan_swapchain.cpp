#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_vulkan.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_swapchain.hpp"

namespace Yona {

void VulkanSwapchain::init(
  const VulkanDevice &device,
  const VulkanSurface &surface,
  const Resolution &initialResolution) {
  mImageIndex = 0;

  const DeviceSwapchainSupport &swapchainDetails = device.mSwapchainSupport;

  VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(
    swapchainDetails.availableFormats,
    swapchainDetails.availableFormatsCount);

  VkExtent2D surfaceExtent = chooseSwapchainExtent(
    initialResolution,
    &swapchainDetails.capabilities);

  VkPresentModeKHR presentMode = chooseSurfacePresentMode(
    swapchainDetails.availablePresentModes,
    swapchainDetails.availablePresentModesCount);

  // add 1 to the minimum images supported in the swapchain
  uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;

  if (imageCount > swapchainDetails.capabilities.maxImageCount &&
      swapchainDetails.capabilities.maxImageCount)
    imageCount = swapchainDetails.capabilities.maxImageCount;

  uint32_t queueFamilyIndices[] = {
    (uint32_t)device.mQueueFamilies.graphicsFamily,
    (uint32_t)device.mQueueFamilies.presentFamily
  };

  VkSwapchainCreateInfoKHR swapchainInfo = {};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = surface.mSurface;
  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainInfo.imageExtent = surfaceExtent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  swapchainInfo.imageSharingMode =
    (device.mQueueFamilies.graphicsFamily ==
     device.mQueueFamilies.presentFamily) ?
    VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

  swapchainInfo.queueFamilyIndexCount =
    (device.mQueueFamilies.graphicsFamily ==
     device.mQueueFamilies.presentFamily) ?
    0 : 2;

  swapchainInfo.pQueueFamilyIndices =
    (device.mQueueFamilies.graphicsFamily ==
     device.mQueueFamilies.presentFamily) ?
    NULL : queueFamilyIndices;

  swapchainInfo.preTransform = swapchainDetails.capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result = vkCreateSwapchainKHR(
    device.mLogicalDevice, &swapchainInfo,
    NULL, &mSwapchain);

  if (result == VK_SUCCESS) {
    LOG_INFO("Created Vulkan swapchain\n");
  }
  else {
    LOG_ERROR("Failed to create Vulkan swapchain\n");
    PANIC_AND_EXIT();
  }

  vkGetSwapchainImagesKHR(device.mLogicalDevice, mSwapchain, &imageCount, NULL);

  mImages.init(imageCount);
  mImages.size = imageCount;

  result = vkGetSwapchainImagesKHR(
    device.mLogicalDevice, mSwapchain,
    &imageCount, mImages.data);

  mExtent = surfaceExtent;
  mFormat = surfaceFormat.format;
  mPresentMode = presentMode;

  mImageViews.init(imageCount);

  for (uint32_t i = 0; i < imageCount; ++i, ++mImageViews.size) {
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = mImages[i];
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = mFormat;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(
      vkCreateImageView(
        device.mLogicalDevice,
        &imageViewInfo,
        NULL,
        &mImageViews[i]));
  }
}

Array<VulkanFramebuffer> VulkanSwapchain::makeFramebuffers(
  const VulkanDevice &device,
  const VulkanRenderPass &renderPass) const {
  Array<VulkanFramebuffer> framebuffers(mImageViews.size);

  for (int i = 0; i < mImageViews.size; ++i) {
    VulkanTexture textureAttachment;
    textureAttachment.mImageView = mImageViews[i];
    textureAttachment.mLayerCount = 1;
    textureAttachment.mViewLayerCount = 1;
    textureAttachment.mExtent = {mExtent.width, mExtent.height, 1};

    VulkanFramebufferConfig framebufferConfig (1, renderPass);
    framebufferConfig.addAttachment(textureAttachment);

    framebuffers[i].init(device, framebufferConfig);
  }

  framebuffers.size = framebuffers.capacity;

  return framebuffers;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(
  VkSurfaceFormatKHR *availableFormats,
  uint32_t formatCount) {
  if (formatCount == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    VkSurfaceFormatKHR format;
    format.format = VK_FORMAT_B8G8R8A8_UNORM;
    format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  }

  for (uint32_t i = 0; i < formatCount; ++i) {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
        availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return(availableFormats[i]);
    }
  }

  return availableFormats[0];
}

VkExtent2D VulkanSwapchain::chooseSwapchainExtent(
  const Resolution &initialResolution,
  const VkSurfaceCapabilitiesKHR *capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return(capabilities->currentExtent);
  }
  else {
    VkExtent2D actualExtent = {
      (uint32_t)initialResolution.width,
      (uint32_t)initialResolution.height
    };

    actualExtent.width = MAX(
      capabilities->minImageExtent.width,
      MIN(
        capabilities->maxImageExtent.width,
        actualExtent.width));

    actualExtent.height = MAX(
      capabilities->minImageExtent.height,
      MIN(
        capabilities->maxImageExtent.height,
        actualExtent.height));

    return actualExtent;
  }
}

VkPresentModeKHR VulkanSwapchain::chooseSurfacePresentMode(
  const VkPresentModeKHR *availablePresentModes,
  uint32_t presentModeCount) {
  VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < presentModeCount; ++i) {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return(availablePresentModes[i]);
    }
    else if (availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      bestMode = availablePresentModes[i];
    }
  }
  return bestMode;
}

uint32_t VulkanSwapchain::acquireNextImage(
  const VulkanDevice &device,
  const VulkanSemaphore &semaphore) {
  VkFence nullFence = VK_NULL_HANDLE;

  VkResult result = vkAcquireNextImageKHR(
    device.mLogicalDevice, 
    mSwapchain, 
    UINT64_MAX, 
    semaphore.mSemaphore, 
    nullFence, 
    &mImageIndex);

  return mImageIndex;
}

}
