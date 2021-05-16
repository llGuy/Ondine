#pragma once

#include <vulkan/vulkan.h>
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

struct WindowContextInfo;
class VulkanSwapchain;
class VulkanDevice;
class VulkanInstance;
class VulkanDescriptorPool;
class VulkanCommandPool;
class VulkanFrame;

class VulkanImgui {
public:
  void init(
    const VulkanInstance &instance,
    const VulkanDevice &device,
    const VulkanSwapchain &swapchain,
    const VulkanDescriptorPool &descriptorPool,
    const VulkanCommandPool &commandPool,
    const WindowContextInfo &surfaceInfo);

  void render(const VulkanFrame &frame);

private:
  static void imguiCallback(VkResult result);

private:
  VulkanRenderPass mImguiRenderPass;

  // Store debug procs
};

}
