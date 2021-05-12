#include "yona_buffer.hpp"
#include "yona_renderer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void Renderer::init(const VulkanContext &vulkanContext) {
  initRenderPipelineStages(vulkanContext);
}

void Renderer::initRenderPipelineStages(const VulkanContext &vulkanContext) {
  VulkanContextProperties ctxProperties = vulkanContext.getProperties();

  { // Final render stage (of which the output will get presented to the screen)
    VulkanRenderPassConfig finalRenderPassConfig (1, 1);

    finalRenderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::Present, AttachmentType::Color,
      ctxProperties.swapchainFormat);

    finalRenderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mFinalStage.init(vulkanContext.device(), finalRenderPassConfig);
  }
}

}
