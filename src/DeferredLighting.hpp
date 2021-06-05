#pragma once

#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

struct LightingProperties {
  // Vector going out towards the sun
  alignas(16) glm::vec3 sunDirection;
  alignas(16) glm::vec3 sunSize;
  alignas(16) glm::vec3 white;
  alignas(4) float exposure;
};

class GBuffer;
class Camera;
class SkyRenderer;
class PlanetRenderer;

class DeferredLighting : public RenderStage {
public:
  void init(
    VulkanContext &graphicsContext,
    const LightingProperties *properties = nullptr);

  void render(
    VulkanFrame &frame, const GBuffer &gbuffer,
    const Camera &camera, const PlanetRenderer &planet,
    const SkyRenderer &sky);

  void updateData(
    const VulkanCommandBuffer &commandBuffer,
    const LightingProperties &properties);

  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  VulkanUniform mLightingOutputUniform;
  VulkanUniform mLightingPropertiesUniform;

  VulkanBuffer mLightingPropertiesBuffer;

  VulkanPipeline mLightingPipeline;
  VulkanRenderPass mLightingRenderPass;
  VulkanFramebuffer mLightingFBO;
  VulkanTexture mLightingTexture;
  VkExtent2D mLightingExtent;
};

}
