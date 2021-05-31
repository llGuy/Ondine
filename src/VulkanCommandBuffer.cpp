#include "Log.hpp"
#include <assert.h>
#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Yona {

void VulkanCommandBuffer::init(
  VkCommandBuffer handle,
  VkCommandBufferLevel level) {
  mViewportCount = 0;
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
  const VkExtent2D &extent) {
  pushViewport(extent);

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

void VulkanCommandBuffer::endRenderPass() {
  popViewport();
  vkCmdEndRenderPass(mCommandBuffer);
}

void VulkanCommandBuffer::end() const {
  vkEndCommandBuffer(mCommandBuffer);
}

void VulkanCommandBuffer::copyBuffer(
  const VulkanBuffer &dst, size_t dstOffset,
  const VulkanBuffer &src, size_t srcOffset,
  size_t size) const {
  VkBufferMemoryBarrier barriers[2];

  barriers[0] = dst.makeBarrier(
    dst.mUsedAtLatest, VK_PIPELINE_STAGE_TRANSFER_BIT, dstOffset, size);
  barriers[1] = src.makeBarrier(
    src.mUsedAtLatest, VK_PIPELINE_STAGE_TRANSFER_BIT, srcOffset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    dst.mUsedAtLatest | src.mUsedAtLatest,
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
    VK_PIPELINE_STAGE_TRANSFER_BIT, dst.mUsedAtEarliest, dstOffset, size);
  barriers[1] = src.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, src.mUsedAtEarliest, srcOffset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    dst.mUsedAtEarliest | src.mUsedAtEarliest,
    0,
    0, NULL,
    2, barriers,
    0, NULL);
}

void VulkanCommandBuffer::updateBuffer(
  const VulkanBuffer &buffer,
  size_t offset, size_t size,
  const void *data) const {
  auto barrier = buffer.makeBarrier(
    buffer.mUsedAtLatest, VK_PIPELINE_STAGE_TRANSFER_BIT, offset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    buffer.mUsedAtLatest,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, NULL,
    1, &barrier,
    0, NULL);

  vkCmdUpdateBuffer(mCommandBuffer, buffer.mBuffer, offset, size, data);

  barrier = buffer.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, buffer.mUsedAtEarliest, offset, size);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    buffer.mUsedAtEarliest,
    0,
    0, NULL,
    1, &barrier,
    0, NULL);
}

void VulkanCommandBuffer::setViewport(
  VkExtent2D extent, VkExtent2D offset, uint32_t maxDepth) const {
  if (extent.width == 0) {
    extent = mViewports[mViewportCount - 1];
  }

  VkViewport viewport = {};
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.x = offset.width;
  viewport.y = offset.height;
  viewport.maxDepth = 1;
  vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
}

void VulkanCommandBuffer::setScissor(
  VkOffset2D offset, VkExtent2D extent) const {
  if (extent.width == 0) {
    extent = mViewports[mViewportCount - 1];
  }

  VkRect2D rect = {};
  rect.extent = extent;
  rect.offset = offset;
  vkCmdSetScissor(mCommandBuffer, 0, 1, &rect);
}

void VulkanCommandBuffer::pushViewport(VkExtent2D viewport) {
  assert(mViewportCount < MAX_VIEWPORT_COUNT);
  mViewports[mViewportCount++] = viewport;
}

VkExtent2D VulkanCommandBuffer::popViewport() {
  assert(mViewportCount > 0);
  return mViewports[mViewportCount--];
}

void VulkanCommandBuffer::bindPipeline(const VulkanPipeline &pipeline) {
  vkCmdBindPipeline(
    mCommandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline.mPipeline);

  mCurrentPipeline = pipeline.mPipeline;
  mCurrentPipelineLayout = pipeline.mPipelineLayout;
}

void VulkanCommandBuffer::pushConstants(size_t size, void *ptr) {
  vkCmdPushConstants(
    mCommandBuffer, mCurrentPipelineLayout,
    VK_SHADER_STAGE_ALL, 0, size, ptr);
}

void VulkanCommandBuffer::draw(
  size_t vertexCount, size_t instanceCount,
  size_t firstVertex, size_t firstInstance) const {
  vkCmdDraw(
    mCommandBuffer,
    vertexCount, instanceCount,
    firstVertex, firstInstance);
}

void VulkanCommandBuffer::transitionImageLayout(
  const VulkanTexture &texture,
  VkImageLayout src, VkImageLayout dst,
  VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const {
  auto barrier = texture.makeBarrier(src, dst);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    srcStage,
    dstStage,
    0,
    0, NULL,
    0, NULL,
    1, &barrier);
}

}
