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

  /* Just so that outside code cannot modify the instance */
  VkInstance instance() const;
  /* Same for the logical device */
  VkDevice device() const;

private:
  VulkanInstance mInstance;
  VulkanSurface mSurface;
  VulkanDevice mDevice;
};

}
