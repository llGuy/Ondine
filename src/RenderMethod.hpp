#pragma once

#include "Camera.hpp"
#include "FastMap.hpp"
#include "Clipping.hpp"
#include "VulkanFrame.hpp"
#include "ModelManager.hpp"
#include "PlanetRenderer.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::Graphics {

using RenderShader = VulkanPipeline;
using RenderShaderEntries = FastMapStd<std::string, RenderShader, 1000>;

// All the resources accessible to Ondine shaders
struct RenderResources {
  const Camera &camera;
  const PlanetRenderer &planet;
  const Clipping &clipping;
  // Skeletal animation stuff
};

using RenderBindResourceProc = void (*)(
  const VulkanCommandBuffer &commandBuffer,
  const RenderResources &resources);

using RenderPushConstantProc = void (*)(
  const VulkanCommandBuffer &commandBuffer,
  const struct SceneObject &object);

class RenderMethod {
public:
  RenderMethod() = default;

  RenderMethod(
    const ModelManager &models,
    const RenderShaderEntries &entries);

  void init(
    const std::string &shaderName,
    StaticModelHandle handleModel,
    RenderBindResourceProc bindResProc,
    RenderPushConstantProc pushConstantProc);

  void bindShader(const VulkanFrame &frame);

  void bindBuffers(const VulkanFrame &frame);

  void bindResources(
    const VulkanFrame &frame,
    const RenderResources &resources);

  void pushConstant(
    const VulkanFrame &frame,
    const struct SceneObject &object);

  void submit(const VulkanFrame &frame);
  
private:
  FastMapHandle mRenderShader;
  StaticModelHandle mModel;
  RenderBindResourceProc mBindResourceProc;
  RenderPushConstantProc mPushConstantProc;
  const RenderShaderEntries *mShaderEntries;
  const ModelManager *mModelManager;
};

using RenderMethodEntries = FastMapStd<std::string, RenderMethod, 1000>;
using RenderMethodHandle = FastMapHandle;

}
