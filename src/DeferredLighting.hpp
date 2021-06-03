#pragma once

#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine {

class GBuffer;
class Camera;
class PlanetRenderer;

class DeferredLighting : public RenderStage {
public:
  void init(VulkanContext &graphicsContext);

  void render(
    VulkanFrame &frame, const GBuffer &gbuffer,
    const Camera &camera, const PlanetRenderer &planet);

  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  VulkanUniform mLightingUniform;

  VulkanPipeline mLightingPipeline;
  VulkanRenderPass mLightingRenderPass;
  VulkanFramebuffer mLightingFBO;
  VulkanTexture mLightingTexture;
  VkExtent2D mLightingExtent;
};

}
