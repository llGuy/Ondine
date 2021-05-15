#include "yona_vulkan_command_buffer.hpp"

namespace Yona {

void VulkanCommandBuffer::begin(
  VkCommandBufferUsageFlags usage,
  VkCommandBufferInheritanceInfo *inheritance) {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = usage;
  beginInfo.pInheritanceInfo = inheritance;

  vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
}

void VulkanCommandBuffer::end() {
  vkEndCommandBuffer(mCommandBuffer);
}

}
