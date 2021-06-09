#pragma once

#include "GBuffer.hpp"
#include "SceneSubmitter.hpp"

namespace Ondine::Graphics {

/* Experiment: do screen-space or planar reflections produce better perf */
class WaterRenderer : public RenderStage {
public:
  void init(VulkanContext &graphicsContext);

  void tick(VulkanFrame &frame, SceneSubmitter &sceneSubmitter);

  // Resolution of the game viewport
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  GBuffer mGBuffer;
  // Holds camera information of the camera from underneath the water level
  Camera mReflectionCamera;
};

}
