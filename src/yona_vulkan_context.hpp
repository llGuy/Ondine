#pragma once

#include "yona_io.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"

namespace Yona {

struct WindowContextInfo;

class VulkanContext {
public:
  VulkanContext();

  void initInstance();
  void initContext(const WindowContextInfo &surfaceInfo);

private:
  VulkanInstance mInstance;
  VulkanSurface mSurface;
  VulkanDevice mDevice;
};

}
