#pragma once

#include "yona_io.hpp"
#include "yona_buffer.hpp"
#include <vulkan/vulkan.h>
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_framebuffer.hpp"

namespace Yona {

class VulkanSemaphore;

class VulkanSwapchain {
public:
  VulkanSwapchain() = default;

  void init(
    const VulkanDevice &device,
    const VulkanSurface &surface,
    const Resolution &initialResolution);

  void destroy(const VulkanDevice &device);

  Array<VulkanFramebuffer> makeFramebuffers(
    const VulkanDevice &device,
    const VulkanRenderPass &renderPass) const;

  uint32_t acquireNextImage(
    const VulkanDevice &device,
    const VulkanSemaphore &semaphore);

private:
  VkSurfaceFormatKHR chooseSurfaceFormat(
    VkSurfaceFormatKHR *availableFormats,
    uint32_t format_count);

  VkExtent2D chooseSwapchainExtent(
    const Resolution &initialResolution,
    const VkSurfaceCapabilitiesKHR *capabilities);

  VkPresentModeKHR chooseSurfacePresentMode(
    const VkPresentModeKHR *availablePresentModes,
    uint32_t presentModeCount);

private:
  VkSwapchainKHR mSwapchain;
  /* Just store raw images and image views here */
  Array<VkImage> mImages;
  Array<VkImageView> mImageViews;

  VkFormat mFormat;
  VkExtent2D mExtent;
  VkPresentModeKHR mPresentMode;

  uint32_t mImageIndex;

  friend class VulkanContext;
  friend class VulkanImgui;
  friend class VulkanQueue;
};

}
