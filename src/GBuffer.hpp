#pragma once

#include "RenderStage.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Graphics {

class GBuffer : public RenderStage {
public:
  GBuffer() = default;
  ~GBuffer() override = default;

  void init(VulkanContext &graphicsContext, const VkExtent2D &extent);

  void beginRender(VulkanFrame &frame);
  void endRender(VulkanFrame &frame);

  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  enum {
    Albedo,
    Normal,
    Position,
    Depth,
    Count
  };

  VulkanUniform mAlbedoUniform;

  /* Contains depth buffer, normal buffer, albedo */
  VulkanUniform mGBufferUniform;

  /* All GBuffers will use this render pass */
  static VulkanRenderPass *sGBufferRenderPass;

  VulkanFramebuffer mGBufferFBO;
  VulkanTexture mGBufferTextures[Count];
  VkFormat mGBufferFormats[Count];

  VkExtent2D mGBufferExtent;
};

}
