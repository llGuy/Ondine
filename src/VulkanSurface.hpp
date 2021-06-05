#pragma once

#include <vulkan/vulkan.h>
#include "Window.hpp"

namespace Ondine::Graphics {

class VulkanSurface {
public:
  VulkanSurface() = default;

  void init(
    const class VulkanInstance &instance,
    const Core::WindowContextInfo &info);

private:
  VkSurfaceKHR mSurface;

  friend class VulkanInstance;
  friend class VulkanDevice;
  friend class VulkanSwapchain;
};

}
