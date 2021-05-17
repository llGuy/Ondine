#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanBuffer;

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
    const VkExtent2D &extent) const;
  void endRenderPass() const;

  void copyBuffer(
    const VulkanBuffer &dst, size_t dstOffset,
    const VulkanBuffer &src, size_t srcOffset,
    size_t size);

private:
  void init(VkCommandBuffer handle, VkCommandBufferLevel level);

private:
  VkCommandBuffer mCommandBuffer;
  VkCommandBufferLevel mLevel;
  VkSubpassContents mSubpassContents;

  friend class VulkanCommandPool;
  friend class VulkanQueue;
  friend class VulkanImgui;
};

}
