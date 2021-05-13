#pragma once

#include "yona_io.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"
#include "yona_vulkan_swapchain.hpp"
#include "yona_vulkan_command_pool.hpp"

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

  /* Starts the render pass which renders to the swapchain */
  void beginSwapchainRender();
  /* Ends the render pass which renders to the swapchain */
  void endSwapchainRender();

  const VulkanDevice &device() const;
  VulkanContextProperties getProperties() const;

private:
  VulkanInstance mInstance;
  VulkanSurface mSurface;
  VulkanDevice mDevice;
  VulkanSwapchain mSwapchain;
  VulkanRenderPass mFinalRenderPass;
  Array<VulkanFramebuffer> mFinalFramebuffers;
  VulkanCommandPool mCommandPool;
};

}
