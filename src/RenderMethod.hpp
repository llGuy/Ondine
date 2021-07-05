#pragma once

#include "Camera.hpp"
#include "FastMap.hpp"
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
  // Skeletal animation stuff
};

using RenderBindResourceProc = void (*)(
  const VulkanCommandBuffer &commandBuffer,
  const RenderResources &resources);

class RenderMethod {
public:
  RenderMethod(
    const ModelManager &models,
    const RenderShaderEntries &entries);

  void init(
    const std::string &shaderName,
    StaticModelHandle handleModel,
    RenderBindResourceProc proc);

  void prepare(const VulkanFrame &frame);

  void bindResources(
    const VulkanFrame &frame,
    const RenderResources &resources);
  
private:
  FastMapHandle mRenderShader;
  StaticModelHandle mModel;
  RenderBindResourceProc mBindResourceProc;
  const RenderShaderEntries &mShaderEntries;
  const ModelManager &mModelManager;
};

using RenderMethodEntries = FastMapStd<std::string, RenderMethod, 1000>;
using RenderMethodHandle = FastMapHandle;

}
