#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer3D::init(VulkanContext &vulkanContext) {
  mGBuffer.init(vulkanContext);
  mRendererSky.init(vulkanContext, mGBuffer);

  // Idle with all precomputation stuff
  vulkanContext.device().graphicsQueue().idle();

  auto properties = vulkanContext.getProperties();
  mPipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };
}

void Renderer3D::tick(const Tick &tick, VulkanFrame &frame) {
  mGBuffer.beginRender(frame);
  {
    // Renders the demo
    mRendererSky.tick(tick, frame, mPipelineViewport);
  }
  mGBuffer.endRender(frame);
}

void Renderer3D::resize(VulkanContext &vulkanContext, Resolution newResolution) {
  mGBuffer.resize(vulkanContext, newResolution);
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mGBuffer;
}

}
