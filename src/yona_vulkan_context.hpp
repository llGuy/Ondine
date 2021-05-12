#pragma once

#include "yona_io.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"
#include "yona_vulkan_swapchain.hpp"

namespace Yona {

struct WindowContextInfo;

struct VulkanContextProperties {
  VkFormat depthFormat;
  VkFormat swapchainFormat;
};

class VulkanContext {
public:
  VulkanContext();

  void initInstance();
  void initContext(const WindowContextInfo &surfaceInfo);

  const VulkanDevice &device() const;
  VulkanContextProperties getProperties() const;

private:
  VulkanInstance mInstance;
  VulkanSurface mSurface;
  VulkanDevice mDevice;
  VulkanSwapchain mSwapchain;
};

}
