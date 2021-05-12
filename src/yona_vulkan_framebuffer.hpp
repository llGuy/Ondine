#pragma once

#include "yona_io.hpp"
#include "yona_buffer.hpp"
#include "yona_vulkan_texture.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

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

private:
  VkFramebuffer mFramebuffer;
  Array<VulkanTexture> mAttachments;

  friend class VulkanSwapchain;
};

}
