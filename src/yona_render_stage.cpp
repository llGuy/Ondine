#include "yona_render_stage.hpp"

namespace Yona {

void RenderStage::init(
  const VulkanDevice &device,
  VulkanRenderPassConfig &config) {
  mRenderPass.init(device, config);
}

void RenderStage::startStage(const VulkanFrame &frame) {
  // Start the render pass
}

void RenderStage::nextSubpass(const VulkanFrame &frame) {
  
}

void RenderStage::endStage(const VulkanFrame &frame) {
  // End the render pass
}

}
