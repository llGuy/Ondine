#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer::init(const VulkanContext &vulkanContext) {
  initRenderPipelineStages(vulkanContext);
}

void Renderer::initRenderPipelineStages(const VulkanContext &vulkanContext) {
  VulkanContextProperties ctxProperties = vulkanContext.getProperties();
}

}
