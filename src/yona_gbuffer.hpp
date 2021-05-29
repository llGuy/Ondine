#pragma once

#include "yona_render_stage.hpp"
#include "yona_vulkan_context.hpp"

namespace Yona {

class GBuffer : public RenderStage {
public:
  enum {
    Albedo,
    Normal,
    Depth
  };

  void init(VulkanContext &graphicsContext);

  void beginRender(VulkanFrame &frame);
  void endRender(VulkanFrame &frame);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  VulkanUniform mAlbedoUniform;

  /* Contains depth buffer, normal buffer, albedo */
  VulkanUniform mGBufferUniform;

  VulkanRenderPass mGBufferRenderPass;
  VulkanFramebuffer mGBufferFBO;
  Array<VulkanTexture> mGBufferTextures;

  VkExtent2D mGBufferExtent;
};

}
