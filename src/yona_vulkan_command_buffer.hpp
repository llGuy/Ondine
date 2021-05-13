#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

class VulkanCommandBuffer {
public:
  

private:
  VkCommandBuffer mCommandBuffer;

  friend class VulkanCommandPool;
};

}
