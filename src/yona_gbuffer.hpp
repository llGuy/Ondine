#pragma once

#include "yona_vulkan_context.hpp"

namespace Yona {

class GBuffer {
public:
  enum {
    Albedo,
    Normal,
    Depth
  };

  void init(VulkanContext &graphicsContext);

  void beginRender(VulkanFrame &frame);
  void endRender(VulkanFrame &frame);

private:
  /* Contains depth buffer, normal buffer, albedo */
  VulkanUniform mGBufferUniform;

  VulkanRenderPass mGBufferRenderPass;
  VulkanFramebuffer mGBufferFBO;
  Array<VulkanTexture> mGBufferTextures;

  VkExtent2D mGBufferExtent;
};

}
