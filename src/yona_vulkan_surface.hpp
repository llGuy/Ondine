#pragma once

#include <vulkan/vulkan.h>
#include "yona_window.hpp"

namespace Yona {

class VulkanSurface {
public:
  VulkanSurface() = default;

  void init(
    const class VulkanInstance &instance,
    const WindowContextInfo &info);

private:
  VkSurfaceKHR mSurface;

  friend class VulkanInstance;
  friend class VulkanDevice;
};

}
