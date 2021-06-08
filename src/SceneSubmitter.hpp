#pragma once

#include "Camera.hpp"
#include "GBuffer.hpp"
#include "ModelManager.hpp"

namespace Ondine::Graphics {

class SceneSubmitter {
public:
  SceneSubmitter(ModelManager &modelManager);
  void init(
    const GBuffer &gbuffer,
    VulkanContext &graphicsContext);

  void submit(const Camera &camera, VulkanFrame &frame);

private:
  StaticModelHandle mTestModel;
  VulkanPipeline mTestPipeline;

  struct {
    glm::mat4 modelMatrix;
  } testPushConstant;

  ModelManager &mModelManager;
};

}
