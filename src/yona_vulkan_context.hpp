#pragma once

#include "yona_vulkan_instance.hpp"

namespace Yona {

class VulkanContext {
public:
  /* Just so that outside code cannot modify the instance */
  VkInstance instance() const;
  /* Same for the logical device */
  VkDevice device() const;

private:
  VulkanInstance mInstance;
};

}
