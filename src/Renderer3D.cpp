#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "VulkanRenderPass.hpp"

namespace Yona {

Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext) {
  
}

void Renderer3D::init() {
  mCamera.init(mGraphicsContext);
  mGBuffer.init(mGraphicsContext);
  mSkyRenderer.init(mGraphicsContext, mGBuffer);

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
    mSkyRenderer.tick(tick, frame, mPipelineViewport);
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
