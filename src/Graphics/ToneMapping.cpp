#include "ToneMapping.hpp"
#include "RendererDebug.hpp"

namespace Ondine::Graphics {

const char *const ToneMapping::TONE_MAPPING_FRAG_SPV =
  "res/spv/ToneMapping.frag.spv";

void ToneMapping::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent,
  const ToneMappingProperties &initialProperties) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      TONE_MAPPING_TEXTURE_FORMAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mRenderPass.init(graphicsContext.device(), renderPassConfig);
  }

  { // Add tracked paths
    addTrackedPath(TONE_MAPPING_FRAG_SPV, &mPipeline);
  }

  { // Create pipeline
    mPipeline.init(
      [](VulkanPipeline &res, ToneMapping &owner, VulkanContext &context) {
        VulkanPipelineConfig pipelineConfig(
          {owner.mRenderPass, 0},
          VulkanShader(context.device(), "res/spv/TexturedQuad.vert.spv"),
          VulkanShader(context.device(), TONE_MAPPING_FRAG_SPV));

        pipelineConfig.configurePipelineLayout(
          sizeof(ToneMappingProperties),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

        res.init(context.device(), context.descriptorLayouts(), pipelineConfig);
      },
      *this,
      graphicsContext);
  }

  { // Attachments and framebuffer
    mExtent = {initialExtent.width, initialExtent.height};

    initTargets(graphicsContext);
  }

  { // Set properties
    mProperties = initialProperties;
  }
}

void ToneMapping::render(
  VulkanFrame &frame, const RenderStage &previousOutput) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.dbgBeginRegion(
    "ToneMappingStage", DBG_TONE_MAPPING_COLOR);

  commandBuffer.beginRenderPass(mRenderPass, mFBO, {}, mExtent);

  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(previousOutput.uniform());
  commandBuffer.pushConstants(sizeof(ToneMappingProperties), &mProperties);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();

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
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, TONE_MAPPING_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    {mExtent.width, mExtent.height, 1}, 1, 1);

  mToneMappingOutput.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mTexture));

  VulkanFramebufferConfig fboConfig(1, mRenderPass);
  fboConfig.addAttachment(mTexture);
  mFBO.init(graphicsContext.device(), fboConfig);
}

void ToneMapping::destroyTargets(VulkanContext &graphicsContext) {
  mToneMappingOutput.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());
  mFBO.destroy(graphicsContext.device());
  mTexture.destroy(graphicsContext.device());
}

}
