#pragma once

#include <vulkan/vulkan.h>
#include "yona_buffer.hpp"
#include "yona_vulkan_uniform.hpp"
#include "yona_vulkan_pipeline.hpp"

namespace Yona {

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanBuffer;
class VulkanCommandBuffer;

class VulkanCommandBuffer {
public:
  // Add inheritance info (with RenderStage)
  void begin(
    VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *inheritance) const;
  void end() const;

  void beginRenderPass(
    const VulkanRenderPass &renderPass,
    const VulkanFramebuffer &framebuffer,
    const VkOffset2D &offset,
    const VkExtent2D &extent);
  void endRenderPass();

  // With no parameters, set it to the render pass extent
  void setViewport(VkExtent2D extent = {}, uint32_t maxDepth = 1) const;
  void setScissor(VkOffset2D offset, VkExtent2D extent) const;

  void bindPipeline(const VulkanPipeline &pipeline);
  void pushConstants(size_t size, void *ptr);

  template <typename ...T>
  void bindUniforms(const T &...uniforms) const {
    size_t count = sizeof...(T);
    auto pred = [](const VulkanUniform &uniform) -> VkDescriptorSet {
      return uniform.mDescriptorSet;
    };

    Array<VkDescriptorSet, AllocationType::Linear> sets =
      makeArrayPred<VkDescriptorSet, AllocationType::Linear>(pred, uniforms...);

    vkCmdBindDescriptorSets(
        mCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        mCurrentPipelineLayout,
        0,
        count,
        sets.data,
        0,
        NULL);
  }

  void draw(
    size_t vertexCount, size_t instanceCount,
    size_t firstVertex, size_t firstInstance) const;

  void copyBuffer(
    const VulkanBuffer &dst, size_t dstOffset,
    const VulkanBuffer &src, size_t srcOffset,
    size_t size) const;

private:
  void init(VkCommandBuffer handle, VkCommandBufferLevel level);

private:
  VkCommandBuffer mCommandBuffer;
  VkCommandBufferLevel mLevel;
  VkSubpassContents mSubpassContents;
  VkExtent2D mCurrentRenderPassExtent;
  VkPipeline mCurrentPipeline;
  VkPipelineLayout mCurrentPipelineLayout;

  friend class VulkanCommandPool;
  friend class VulkanQueue;
  friend class VulkanImgui;
};

}
