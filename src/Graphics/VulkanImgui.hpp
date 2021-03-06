#pragma once

#include "Window.hpp"
#include <vulkan/vulkan.h>
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

class VulkanSwapchain;
class VulkanDevice;
class VulkanInstance;
class VulkanDescriptorPool;
class VulkanCommandPool;
struct VulkanFrame;

class VulkanImgui {
public:
  void init(
    const VulkanInstance &instance,
    const VulkanDevice &device,
    const VulkanSwapchain &swapchain,
    const VulkanDescriptorPool &descriptorPool,
    const VulkanCommandPool &commandPool,
    const Core::WindowContextInfo &surfaceInfo,
    const VulkanRenderPass &renderPass);

  void beginRender() const;
  void endRender(const VulkanFrame &frame) const;

private:
  static void imguiCallback(VkResult result);

private:
  // Doesn't contain state for now
};

}
