#pragma once

#include "Scene.hpp"
#include "Scene.hpp"
#include "GBuffer.hpp"
#include "Clipping.hpp"
#include "RenderStage.hpp"
#include "StarRenderer.hpp"
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

/* Experiment: do screen-space or planar reflections produce better perf */
class WaterRenderer : public RenderStage {
public:
  WaterRenderer() = default;
  ~WaterRenderer() override = default;

  void init(
    VulkanContext &graphicsContext,
    const PlanetProperties &planetProperties);

  void tick(
    VulkanFrame &frame,
    const PlanetRenderer &planet,
    const SkyRenderer &sky,
    const StarRenderer &stars,
    const TerrainRenderer &terrainRenderer,
    Scene &sceneSubmitter);

  void updateCameraInfo(
    const CameraProperties &camera,
    const PlanetProperties &planet);

  void updateCameraUBO(const VulkanCommandBuffer &commandBuffer);
  void updateLightingUBO(
    const LightingProperties &properties,
    const VulkanCommandBuffer &commandBuffer);

  // Resolution of the game viewport
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  glm::vec3 reflectCameraPosition(
    const CameraProperties &sceneCamera,
    const PlanetProperties &planetProperties);

  glm::vec3 reflectCameraDirection(
    const CameraProperties &sceneCamera,
    const PlanetProperties &planetProperties);

private:
  static constexpr float VIEWPORT_SCALE = 0.5f;
  static constexpr float OCEAN_HEIGHT = 0.05f;

  GBuffer mGBuffer;
  // We use a deferred lighting stage to light the reflected image
  DeferredLighting mLighting;
  
  // Holds camera information of the camera from underneath the water level
  Camera mReflectionCamera;
  CameraProperties mCameraProperties;
  Resolution mReflectionViewport;
  Clipping mClipping;
};

}
