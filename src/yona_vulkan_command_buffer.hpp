#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

class VulkanRenderPass;
class VulkanFramebuffer;

class VulkanCommandBuffer {
public:
  // Add inheritance info (with RenderStage)
  void begin(
    VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *inheritance) const;

  void beginRenderPass(
    const VulkanRenderPass &renderPass,
    const VulkanFramebuffer &framebuffer,
    const VkOffset2D &offset,
    const VkExtent2D &extent) const;

  void endRenderPass() const;

  void end() const;

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
