#pragma once

#include <vulkan/vulkan.h>
#include "Buffer.hpp"
#include "VulkanUniform.hpp"
#include "VulkanPipeline.hpp"

namespace Yona {

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanBuffer;
class VulkanCommandBuffer;
class VulkanTexture;

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
  void setViewport(
    VkExtent2D extent = {},
    VkExtent2D offset = {},
    uint32_t maxDepth = 1) const;
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

  void transitionImageLayout(
    const VulkanTexture &texture,
    VkImageLayout src, VkImageLayout dst,
    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const;

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
