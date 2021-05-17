#include "yona_log.hpp"
#include "yona_vulkan_buffer.hpp"
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

void VulkanCommandBuffer::copyBuffer(
  const VulkanBuffer &dst, size_t dstOffset,
  const VulkanBuffer &src, size_t srcOffset,
  size_t size) {
  VkBufferMemoryBarrier barriers[2];

  barriers[0] = dst.makeBarrier(
    dst.mUsedAt, VK_PIPELINE_STAGE_TRANSFER_BIT, dstOffset, size);
  barriers[1] = src.makeBarrier(
    src.mUsedAt, VK_PIPELINE_STAGE_TRANSFER_BIT, srcOffset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, NULL,
    2, barriers,
    0, NULL);

  VkBufferCopy copyRegion = {};
  copyRegion.dstOffset = dstOffset;
  copyRegion.srcOffset = srcOffset;
  copyRegion.size = size;

  vkCmdCopyBuffer(mCommandBuffer, src.mBuffer, dst.mBuffer, 1, &copyRegion);

  barriers[0] = dst.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, dst.mUsedAt, dstOffset, size);
  barriers[1] = src.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, src.mUsedAt, srcOffset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, NULL,
    2, barriers,
    0, NULL);
}

}
