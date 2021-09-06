#include "ToneMapping.hpp"

namespace Ondine::Graphics {

void ToneMapping::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent,
  const ToneMappingProperties &initialProperties) {
  
}

void ToneMapping::render(
  VulkanFrame &frame, const VulkanUniform &previousOutput) {
  
}
void ToneMapping::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  
}

const VulkanRenderPass &ToneMapping::renderPass() const {
  
}

const VulkanFramebuffer &ToneMapping::framebuffer() const {
  
}

const VulkanUniform &ToneMapping::uniform() const {
  
}

VkExtent2D ToneMapping::extent() const {
  
}

void ToneMapping::initTargets(VulkanContext &graphicsContext) {
  
}

void ToneMapping::destroyTargets(VulkanContext &graphicsContext) {
  
}

}
