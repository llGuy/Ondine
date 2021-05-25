#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer::init(VulkanContext &vulkanContext) {
  mGBuffer.init(vulkanContext);
  mRendererSky.init(vulkanContext);

  // Idle with all precomputation stuff
  vulkanContext.device().graphicsQueue().idle();
}

void Renderer::tickIn(VulkanFrame &frame) {
  mRendererSky.tickIn(frame);
}

void Renderer::tickOut(VulkanFrame &frame) {
  mRendererSky.tickOut(frame);
}

}
