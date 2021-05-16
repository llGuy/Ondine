#include "yona_log.hpp"
#include "yona_vulkan_render_pass.hpp"
#include "yona_vulkan_framebuffer.hpp"
#include "yona_vulkan_command_buffer.hpp"

namespace Yona {

void VulkanCommandBuffer::init(
  VkCommandBuffer handle,
  VkCommandBufferLevel level) {
  mCommandBuffer = handle;
  mLevel = level;

  switch (level) {
  case VK_COMMAND_BUFFER_LEVEL_PRIMARY: {
    mSubpassContents = VK_SUBPASS_CONTENTS_INLINE;
  } break;

  case VK_COMMAND_BUFFER_LEVEL_SECONDARY: {
    mSubpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
  } break;

  default: {
    LOG_ERRORV("Unsupported command buffer level: %d\n", (int)level);
    PANIC_AND_EXIT();
  } break;
  }
}

void VulkanCommandBuffer::begin(
  VkCommandBufferUsageFlags usage,
  VkCommandBufferInheritanceInfo *inheritance) const {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = usage;
  beginInfo.pInheritanceInfo = inheritance;

  vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
}

void VulkanCommandBuffer::beginRenderPass(
  const VulkanRenderPass &renderPass,
  const VulkanFramebuffer &framebuffer,
  const VkOffset2D &offset,
  const VkExtent2D &extent) const {
  VkRect2D renderArea = {};
  renderArea.offset = offset;
  renderArea.extent = extent;

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.framebuffer = framebuffer.mFramebuffer;
  renderPassBeginInfo.renderPass = renderPass.mRenderPass;
  renderPassBeginInfo.clearValueCount = renderPass.mClearValues.size;
  renderPassBeginInfo.pClearValues = renderPass.mClearValues.data;
  renderPassBeginInfo.renderArea = renderArea;

  vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, mSubpassContents);
}

void VulkanCommandBuffer::endRenderPass() const {
  vkCmdEndRenderPass(mCommandBuffer);
}

void VulkanCommandBuffer::end() const {
  vkEndCommandBuffer(mCommandBuffer);
}

}
