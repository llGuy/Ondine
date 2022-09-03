#pragma once

#include "Buffer.hpp"
#include <vulkan/vulkan.h>
#include "VulkanUniform.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::Graphics {

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanBuffer;
class VulkanCommandBuffer;
class VulkanTexture;
class VulkanArenaSlot;

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
  void setScissor(VkOffset2D offset = {}, VkExtent2D extent = {}) const;

  void pushViewport(VkExtent2D viewport);
  VkExtent2D popViewport();

  void bindPipeline(
    const VulkanPipeline &pipeline, 
    VulkanPipelineBindPoint bindPoint = VulkanPipelineBindPoint::Graphics);
  void pushConstants(size_t size, const void *ptr) const;
  void bindVertexBuffers(
    uint32_t firstBinding, uint32_t bindingCount,
    const VulkanBuffer *buffers, VkDeviceSize *offsets = nullptr) const;
  void bindVertexBuffersArena(const VulkanArenaSlot &slot) const;
  void bindIndexBuffer(
    VkDeviceSize offset, VkIndexType indexType,
    const VulkanBuffer &buffer) const;

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

  template <typename ...T>
  void bindUniformsCompute(const T &...uniforms) const {
    size_t count = sizeof...(T);
    auto pred = [](const VulkanUniform &uniform) -> VkDescriptorSet {
      return uniform.mDescriptorSet;
    };

    Array<VkDescriptorSet, AllocationType::Linear> sets =
      makeArrayPred<VkDescriptorSet, AllocationType::Linear>(pred, uniforms...);

    vkCmdBindDescriptorSets(
        mCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        mCurrentPipelineLayout,
        0,
        count,
        sets.data,
        0,
        NULL);
  }

  void dispatch(const glm::ivec3 &groups) const;

  void draw(
    size_t vertexCount, size_t instanceCount,
    size_t firstVertex, size_t firstInstance) const;

  void drawIndexed(
    uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const;

  void copyBuffer(
    const VulkanBuffer &dst, size_t dstOffset,
    const VulkanBuffer &src, size_t srcOffset,
    size_t size) const;

  // Transition image layout back to what it should be after calling this
  void copyBufferToImage(
    const VulkanTexture &dst,
    uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
    const VulkanBuffer &src, size_t srcOffset, uint32_t srcSize);

  void copyImageToBuffer(
    const VulkanBuffer &dst, size_t dstOffset, uint32_t dstSize,
    const VulkanTexture &src,
    uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel);

  void blitImage(
    const VulkanTexture &dst, VkImageLayout dstLayout,
    const VulkanTexture &src, VkImageLayout srcLayout,
    uint32_t baseLayer, uint32_t layerCount,
    VkPipelineStageFlags lastUsedDst,
    VkPipelineStageFlags lastUsedSrc);

  void updateBuffer(
    const VulkanBuffer &buffer,
    size_t offset, size_t size, const void *data) const;

  void transitionImageLayout(
    const VulkanTexture &texture,
    VkImageLayout src, VkImageLayout dst,
    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const;

#ifdef NDEBUG
  inline void dbgBeginRegion(
    const char *name,
    const glm::vec4 &color) const {
    // Compiler should optimise out calls to these functions
  }

  inline void dbgInsertMarker(
    const char *name,
    const glm::vec4 &color) const {
    // Compiler should optimise out calls to these functions
  }

  inline void dbgEndRegion() const {
    // Compiler should optimise out calls to these functions
  }
#else
  void dbgBeginRegion(
    const char *name,
    const glm::vec4 &color);

  void dbgInsertMarker(
    const char *name,
    const glm::vec4 &color);

  void dbgEndRegion();
#endif

private:
  void init(VkCommandBuffer handle, VkCommandBufferLevel level);

  /* Avoids the need for creating a copy */
  void bindVertexBuffers(
    uint32_t firstBinding, uint32_t bindingCount,
    const VkBuffer *buffers, VkDeviceSize *offsets) const;

private:
  static constexpr uint32_t MAX_VIEWPORT_COUNT = 5;

  VkCommandBuffer mCommandBuffer;
  VkCommandBufferLevel mLevel;
  VkSubpassContents mSubpassContents;
  VkExtent2D mViewports[MAX_VIEWPORT_COUNT];
  size_t mViewportCount;
  VkPipeline mCurrentPipeline;
  VkPipelineLayout mCurrentPipelineLayout;

  friend class VulkanCommandPool;
  friend class VulkanQueue;
  friend class VulkanImgui;
  friend class Model;
};

}
