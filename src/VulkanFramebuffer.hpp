#pragma once

#include "IO.hpp"
#include "Buffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine {

class VulkanFramebufferConfig {
public:
  /* 
     Don't need to pass resolution or layer count, 
     will be set when adding attachments
   */
  VulkanFramebufferConfig(
    size_t attachmentCount,
    const VulkanRenderPass &renderPass);

  void addAttachment(const VulkanTexture &texture);

private:
  void finishConfiguration();

private:
  Resolution mResolution;
  uint32_t mLayerCount;
  Array<VulkanTexture, AllocationType::Linear> mAttachments;
  const VulkanRenderPass &mCompatibleRenderPass;
  VkFramebufferCreateInfo mCreateInfo;

  friend class VulkanFramebuffer;
};

class VulkanFramebuffer {
public:
  VulkanFramebuffer() = default;

  void init(const VulkanDevice &device, VulkanFramebufferConfig &config);
  void destroy(const VulkanDevice &device);

private:
  VkFramebuffer mFramebuffer;

  friend class VulkanSwapchain;
  friend class VulkanCommandBuffer;
  friend class VulkanContext;
};

}
