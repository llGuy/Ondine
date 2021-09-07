#include "BloomRenderer.hpp"

namespace Ondine::Graphics {

const char *BloomRenderer::BLOOM_BLUR_FRAG_SPV = "res/spv/BloomBlur.frag.spv";
const char *BloomRenderer::BLOOM_PREFILTER_FRAG_SPV =
  "res/spv/BloomPrefilter.frag.spv";

void BloomRenderer::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent) {
  {  // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      BLOOM_TEXTURE_FORMAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mRenderPass.init(graphicsContext.device(), renderPassConfig);
  }

  { // Add tracked paths
    addTrackedPath(BLOOM_PREFILTER_FRAG_SPV, &mPrefilterPipeline);
    addTrackedPath(BLOOM_BLUR_FRAG_SPV, &mBlurPipeline);
  }

  { // Create pipelines
    mPrefilterPipeline.init(
      [](VulkanPipeline &res, BloomRenderer &owner, VulkanContext &context) {
        VulkanPipelineConfig pipelineConfig(
          {owner.mRenderPass, 0},
          VulkanShader(context.device(), "res/spv/TexturedQuad.vert.spv"),
          VulkanShader(context.device(), BLOOM_PREFILTER_FRAG_SPV));

        pipelineConfig.configurePipelineLayout(
          sizeof(BloomProperties),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

        res.init(context.device(), context.descriptorLayouts(), pipelineConfig);
      },
      *this,
      graphicsContext);

    mBlurPipeline.init(
      [](VulkanPipeline &res, BloomRenderer &owner, VulkanContext &context) {
        VulkanPipelineConfig pipelineConfig(
          {owner.mRenderPass, 0},
          VulkanShader(context.device(), "res/spv/TexturedQuad.vert.spv"),
          VulkanShader(context.device(), BLOOM_BLUR_FRAG_SPV));

        pipelineConfig.configurePipelineLayout(
          sizeof(BloomProperties),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

        res.init(context.device(), context.descriptorLayouts(), pipelineConfig);
      },
      *this,
      graphicsContext);
  }

  { // Attachments and framebuffers
    mExtent = {initialExtent.width, initialExtent.height};

    initTargets(graphicsContext);
  }
}

void BloomRenderer::render(VulkanFrame &frame, const VulkanUniform &previous) {
  
}

void BloomRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  
}

const VulkanRenderPass &BloomRenderer::renderPass() const {
  return mRenderPass;
}

const VulkanFramebuffer &BloomRenderer::framebuffer() const {
  // TODO
}

const VulkanUniform &BloomRenderer::uniform() const {
  // TODO
}

VkExtent2D BloomRenderer::extent() const {
  return mExtent;
}

void BloomRenderer::initTargets(VulkanContext &graphicsContext) {
  
}

void BloomRenderer::destroyTargets(VulkanContext &graphicsContext) {
  
}

}
