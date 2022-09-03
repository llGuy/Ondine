#include "ToneMapping.hpp"
#include "BloomRenderer.hpp"
#include "RendererDebug.hpp"
#include "VulkanPipeline.hpp"
#include "vulkan/vulkan_core.h"

namespace Ondine::Graphics {

const char *const ToneMapping::TONE_MAPPING_FRAG_SPV =
  "res/spv/ToneMapping.comp.spv";

void ToneMapping::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent,
  const ToneMappingProperties &initialProperties) {
  { // Create pipeline
    VulkanPipelineConfig pipelineConfig(
      {NullReference<VulkanRenderPass>::nullRef, 0},
      VulkanShader(graphicsContext.device(), "res/spv/ToneMapping.comp.spv"));

    pipelineConfig.configurePipelineLayout(
      0 /* Push constant size */,
      VulkanPipelineDescriptorLayout{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});

    mPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Attachments and framebuffer
    mExtent = {initialExtent.width, initialExtent.height};

    initTargets(graphicsContext);
  }

  { // Set properties
    mProperties = initialProperties;
  }
}

void ToneMapping::render(VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.dbgBeginRegion(
    "ToneMappingStage", DBG_TONE_MAPPING_COLOR);

  commandBuffer.transitionImageLayout(
    mTexture,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  commandBuffer.bindPipeline(mPipeline, VulkanPipelineBindPoint::Compute);
  commandBuffer.bindUniformsCompute(mToneMappingStorage);
  commandBuffer.dispatch(glm::ivec3(mExtent.width/16+1, mExtent.height/16+1, 1));

  commandBuffer.transitionImageLayout(
    mTexture,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  commandBuffer.dbgEndRegion();
}

void ToneMapping::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  destroyTargets(vulkanContext);
  mExtent = {newResolution.width, newResolution.height};
  initTargets(vulkanContext);
}

const VulkanRenderPass &ToneMapping::renderPass() const {
  return mRenderPass;
}

const VulkanFramebuffer &ToneMapping::framebuffer() const {
  return mFBO;
}

const VulkanUniform &ToneMapping::uniform() const {
  return mToneMappingOutput;
}

VkExtent2D ToneMapping::extent() const {
  return mExtent;
}

void ToneMapping::initTargets(VulkanContext &graphicsContext) {
  mTexture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::ComputeTarget,
    TextureContents::Color, TONE_MAPPING_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    {mExtent.width, mExtent.height, 1}, 1, 1);

  mToneMappingOutput.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mTexture));

  mToneMappingStorage.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mTexture),
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

  auto cmdbuf = graphicsContext.commandPool().makeCommandBuffer(
      graphicsContext.device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  cmdbuf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

  // Do layout transition
  cmdbuf.transitionImageLayout(
    mTexture,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

  cmdbuf.end();
  
  graphicsContext.device().graphicsQueue().submitCommandBuffer(
    cmdbuf,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());
}

void ToneMapping::destroyTargets(VulkanContext &graphicsContext) {
  mToneMappingOutput.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());
  mFBO.destroy(graphicsContext.device());
  mTexture.destroy(graphicsContext.device());
}

}
