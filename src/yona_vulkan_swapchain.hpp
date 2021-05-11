#pragma once

#include "yona_io.hpp"
#include "yona_buffer.hpp"
#include <vulkan/vulkan.h>
#include "yona_vulkan_device.hpp"

namespace Yona {

class VulkanSwapchain {
public:
  VulkanSwapchain() = default;

  void init(
    const VulkanDevice &device,
    const VulkanSurface &surface,
    const Resolution &initialResolution);

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
};

}
