#pragma once

#include "GBuffer.hpp"
#include "SceneSubmitter.hpp"

namespace Ondine::Graphics {

/* Experiment: do screen-space or planar reflections produce better perf */
class WaterRenderer : public RenderStage {
public:
  void init(
    VulkanContext &graphicsContext,
    const CameraProperties &sceneCamera,
    const PlanetProperties &planetProperties);

  void tick(VulkanFrame &frame, SceneSubmitter &sceneSubmitter);
  void updateCameraInfo(
    const CameraProperties &camera,
    const PlanetRenderer &planet);

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
  static constexpr float VIEWPORT_SCALE = 0.3f;

  GBuffer mGBuffer;
  // Holds camera information of the camera from underneath the water level
  Camera mReflectionCamera;
  CameraProperties mCameraProperties;
  Resolution mReflectionViewport;
};

}
