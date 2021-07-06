#include "Log.hpp"
#include <assert.h>
#include "Utils.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Ondine::Graphics {

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

void VulkanCommandBuffer::pushConstants(size_t size, const void *ptr) const {
  vkCmdPushConstants(
    mCommandBuffer, mCurrentPipelineLayout,
    VK_SHADER_STAGE_ALL, 0, size, ptr);
}

void VulkanCommandBuffer::bindVertexBuffers(
  uint32_t firstBinding, uint32_t bindingCount,
  const VulkanBuffer *buffers, VkDeviceSize *offsets) const {
  VkBuffer *buffersRaw = STACK_ALLOC(VkBuffer, bindingCount);
  for (int i = 0; i < bindingCount; ++i) {
    buffersRaw[i] = buffers[i].mBuffer;
  }

  if (!offsets) {
    offsets = STACK_ALLOC(VkDeviceSize, bindingCount);
    zeroMemory(offsets, sizeof(VkDeviceSize) * bindingCount);
  }
  
  vkCmdBindVertexBuffers(
    mCommandBuffer, firstBinding, bindingCount,
    buffersRaw, offsets);
}

void VulkanCommandBuffer::bindVertexBuffers(
  uint32_t firstBinding, uint32_t bindingCount,
  const VkBuffer *buffers, VkDeviceSize *offsets) const {
  vkCmdBindVertexBuffers(
    mCommandBuffer, firstBinding, bindingCount,
    buffers, offsets);
}

void VulkanCommandBuffer::bindIndexBuffer(
  VkDeviceSize offset, VkIndexType indexType,
  const VulkanBuffer &buffer) const {
  vkCmdBindIndexBuffer(mCommandBuffer, buffer.mBuffer, offset, indexType);
}

void VulkanCommandBuffer::draw(
  size_t vertexCount, size_t instanceCount,
  size_t firstVertex, size_t firstInstance) const {
  vkCmdDraw(
    mCommandBuffer,
    vertexCount, instanceCount,
    firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed(
  uint32_t indexCount, uint32_t instanceCount,
  uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const {
  vkCmdDrawIndexed(
    mCommandBuffer,
    indexCount,
    instanceCount,
    firstIndex,
    vertexOffset,
    firstInstance);
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

void VulkanCommandBuffer::copyBufferToImage(
  const VulkanTexture &dst,
  uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
  const VulkanBuffer &src, size_t srcOffset, uint32_t srcSize) {
  transitionImageLayout(
    dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  auto bufferBarrier = src.makeBarrier(
    src.mUsedAtLatest, VK_PIPELINE_STAGE_TRANSFER_BIT, srcOffset, srcSize);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    src.mUsedAtLatest,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, NULL,
    1, &bufferBarrier,
    0, NULL);

  VkImageAspectFlags aspect;
  switch (dst.mContents) {
  case TextureContents::Color: {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  } break;

  case TextureContents::Depth: {
    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  }

  VkBufferImageCopy region = {};
  region.bufferOffset = srcOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageExtent.width = dst.mExtent.width;
  region.imageExtent.height = dst.mExtent.height;
  region.imageExtent.depth = dst.mExtent.depth;
  region.imageSubresource.aspectMask = aspect;
  region.imageSubresource.mipLevel = mipLevel;
  region.imageSubresource.baseArrayLayer = baseLayer;
  region.imageSubresource.layerCount = layerCount;

  vkCmdCopyBufferToImage(
    mCommandBuffer,
    src.mBuffer,
    dst.mImage,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &region);

  bufferBarrier = src.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, src.mUsedAtEarliest, srcOffset, srcSize);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    src.mUsedAtEarliest,
    0,
    0, NULL,
    1, &bufferBarrier,
    0, NULL);

  // Responsability of the programmer to transition the layout after calling
}

void VulkanCommandBuffer::copyImageToBuffer(
  const VulkanBuffer &dst, size_t dstOffset, uint32_t dstSize,
  const VulkanTexture &src,
  uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel) {
  transitionImageLayout(
    src, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  auto bufferBarrier = dst.makeBarrier(
    dst.mUsedAtLatest, VK_PIPELINE_STAGE_TRANSFER_BIT, dstOffset, dstSize);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    dst.mUsedAtLatest,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, NULL,
    1, &bufferBarrier,
    0, NULL);

  VkImageAspectFlags aspect;
  switch (src.mContents) {
  case TextureContents::Color: {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  } break;

  case TextureContents::Depth: {
    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  }

  VkBufferImageCopy region = {};
  region.bufferOffset = dstOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageExtent.width = src.mExtent.width;
  region.imageExtent.height = src.mExtent.height;
  region.imageExtent.depth = src.mExtent.depth;
  region.imageSubresource.aspectMask = aspect;
  region.imageSubresource.mipLevel = mipLevel;
  region.imageSubresource.baseArrayLayer = baseLayer;
  region.imageSubresource.layerCount = layerCount;

  vkCmdCopyImageToBuffer(
    mCommandBuffer,
    src.mImage,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst.mBuffer,
    1, &region);

  bufferBarrier = dst.makeBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, dst.mUsedAtEarliest, dstOffset, dstSize);

  vkCmdPipelineBarrier(
    mCommandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    dst.mUsedAtEarliest,
    0,
    0, NULL,
    1, &bufferBarrier,
    0, NULL);

  // Responsability of the programmer to transition the layout after calling
}

// For now, only support images with the same amount of layers
void VulkanCommandBuffer::blitImage(
  const VulkanTexture &dst, VkImageLayout dstLayout,
  const VulkanTexture &src, VkImageLayout srcLayout,
  uint32_t baseLayer, uint32_t layerCount,
  VkPipelineStageFlags lastUsedDst,
  VkPipelineStageFlags lastUsedSrc) {
  transitionImageLayout(
    src, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    lastUsedSrc, VK_PIPELINE_STAGE_TRANSFER_BIT);
  transitionImageLayout(
    dst, dstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    lastUsedDst, VK_PIPELINE_STAGE_TRANSFER_BIT);

  VkImageBlit region = {};
  region.srcSubresource.aspectMask = src.mAspect;
  region.srcSubresource.mipLevel = 0;
  region.srcSubresource.baseArrayLayer = baseLayer;
  region.srcSubresource.layerCount = layerCount;
  region.srcOffsets[0] = {};
  region.srcOffsets[1] = VkOffset3D {
    (int32_t)src.mExtent.width,
    (int32_t)src.mExtent.height,
    (int32_t)src.mExtent.depth
  };

  region.dstSubresource.aspectMask = dst.mAspect;
  region.dstSubresource.mipLevel = 0;
  region.dstSubresource.baseArrayLayer = baseLayer;
  region.dstSubresource.layerCount = layerCount;
  region.dstOffsets[0] = {};
  region.dstOffsets[1] = VkOffset3D {
    (int32_t)dst.mExtent.width,
    (int32_t)dst.mExtent.height,
    (int32_t)dst.mExtent.depth
  };

  vkCmdBlitImage(
    mCommandBuffer,
    src.mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &region, VK_FILTER_NEAREST);
}

#ifndef NDEBUG
void VulkanCommandBuffer::dbgBeginRegion(
  const char *name,
  const glm::vec4 &color) {
  if (vkCmdDebugMarkerBegin) {
    VkDebugMarkerMarkerInfoEXT marker_info = {};
    marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    memcpy(marker_info.color, &color[0], sizeof(float) * 4);
    marker_info.pMarkerName = name;
    vkCmdDebugMarkerBegin(mCommandBuffer, &marker_info);
  }
}

void VulkanCommandBuffer::dbgInsertMarker(
  const char *name,
  const glm::vec4 &color) {
  if (vkCmdDebugMarkerInsert) {
    VkDebugMarkerMarkerInfoEXT marker_info = {};
    marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    memcpy(marker_info.color, &color[0], sizeof(float) * 4);
    marker_info.pMarkerName = name;
    vkCmdDebugMarkerInsert(mCommandBuffer, &marker_info);
  }
}

void VulkanCommandBuffer::dbgEndRegion() {
  if (vkCmdDebugMarkerEnd) {
    vkCmdDebugMarkerEnd(mCommandBuffer);
  }
}
#endif

}
