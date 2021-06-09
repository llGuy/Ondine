#include "WaterRenderer.hpp"

namespace Ondine::Graphics {

void WaterRenderer::init(
  VulkanContext &graphicsContext) {
  
}

void WaterRenderer::tick(
  VulkanFrame &frame, SceneSubmitter &sceneSubmitter) {
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    // sceneSubmitter.submit(mReflectionCamera, frame);
  }
  mGBuffer.endRender(frame);
}

void WaterRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  
}

}
