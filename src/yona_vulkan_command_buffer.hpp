#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

class VulkanCommandBuffer {
public:
  // Add inheritance info (with RenderStage)
  void begin(
    VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *inheritance);

  void end();

private:
  VkCommandBuffer mCommandBuffer;

  friend class VulkanCommandPool;
  friend class VulkanQueue;
  friend class VulkanImgui;
};

}
