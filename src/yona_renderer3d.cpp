#include "yona_buffer.hpp"
#include "yona_renderer3d.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext) {
  
}

void Renderer3D::init() {
  mGBuffer.init(mGraphicsContext);
  mRendererSky.init(mGraphicsContext, mGBuffer);

  // Idle with all precomputation stuff
  mGraphicsContext.device().graphicsQueue().idle();

  auto properties = mGraphicsContext.getProperties();
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

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  mPipelineViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(mGraphicsContext, newResolution);
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mGBuffer;
}



}
