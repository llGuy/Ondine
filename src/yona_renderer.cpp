#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer::init(VulkanContext &vulkanContext) {
  mGBuffer.init(vulkanContext);
  mRendererSky.init(vulkanContext, mGBuffer);

  // Idle with all precomputation stuff
  vulkanContext.device().graphicsQueue().idle();
}

void Renderer::tick(const Tick &tick, VulkanFrame &frame) {
  mGBuffer.beginRender(frame);
  {
    // Renders the demo
    mRendererSky.tick(tick, frame);
  }
  mGBuffer.endRender(frame);
}

void Renderer::resize(VulkanContext &vulkanContext) {
  mRendererSky.resize(vulkanContext, mGBuffer);
}

const RenderStage &Renderer::mainRenderStage() const {
  return mGBuffer;
}

}
