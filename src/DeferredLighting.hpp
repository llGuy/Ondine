#pragma once

#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "TrackedResource.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

struct LightingProperties {
  // Vector going out towards the sun
  alignas(16) glm::vec3 sunDirection;
  alignas(16) glm::vec3 sunSize;
  alignas(16) glm::vec3 white;
  alignas(16) glm::vec3 waterSurfaceColor;
  alignas(4) float exposure;
  alignas(4) float time;
  alignas(4) float dt;
};

class Camera;
class GBuffer;
class SkyRenderer;
class WaterRenderer;
class PlanetRenderer;

class DeferredLighting :
  public RenderStage,
  public ResourceTracker {
public:
  DeferredLighting() = default;
  ~DeferredLighting() override = default;

  void init(
    VulkanContext &graphicsContext,
    VkExtent2D initialExtent,
    const LightingProperties *properties = nullptr);

  // Doesn't render planar reflections at a certain planet radius
  void render(
    VulkanFrame &frame, const GBuffer &gbuffer,
    const Camera &camera, const PlanetRenderer &planet,
    const SkyRenderer &sky);

  // Renders planet reflections at a certain planet radius
  void render(
    VulkanFrame &frame, const GBuffer &gbuffer,
    const Camera &camera, const PlanetRenderer &planet,
    const WaterRenderer &water,
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
  static const char *const LIGHTING_FRAG_SPV;
  static const char *const LIGHTING_REFL_FRAG_SPV;

  VulkanUniform mLightingOutputUniform;
  VulkanUniform mLightingPropertiesUniform;

  VulkanBuffer mLightingPropertiesBuffer;

  TrackedResource<VulkanPipeline, DeferredLighting> mLightingReflPipeline;
  TrackedResource<VulkanPipeline, DeferredLighting> mLightingPipeline;

  VulkanRenderPass mLightingRenderPass;
  VulkanFramebuffer mLightingFBO;
  VulkanTexture mLightingTexture;
  VkExtent2D mLightingExtent;
};

}
