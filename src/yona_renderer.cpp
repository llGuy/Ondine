#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer::init(VulkanContext &vulkanContext) {
  mRendererSky.init(vulkanContext);

  initRenderPipelineStages(vulkanContext);
}

void Renderer::tick(const VulkanFrame &frame) {
  mRendererSky.tick(frame);
}

void Renderer::initRenderPipelineStages(VulkanContext &vulkanContext) {
  VulkanContextProperties ctxProperties = vulkanContext.getProperties();
}

}
