#pragma once

#include "Camera.hpp"
#include "GBuffer.hpp"
#include "ModelManager.hpp"
#include "PlanetRenderer.hpp"

namespace Ondine::Graphics {

class SceneSubmitter {
public:
  SceneSubmitter(ModelManager &modelManager);

  void init(
    const GBuffer &gbuffer,
    VulkanContext &graphicsContext);

  void submit(
    const Camera &camera,
    const PlanetRenderer &planet,
    VulkanFrame &frame);

private:
  StaticModelHandle mTestModel;
  VulkanPipeline mTestPipeline;

  struct {
    glm::mat4 modelMatrix;
  } mTestPushConstant;

  ModelManager &mModelManager;
};

}
