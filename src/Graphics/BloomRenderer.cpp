#include "BloomRenderer.hpp"
#include "RendererDebug.hpp"

namespace Ondine::Graphics {

const char *BloomRenderer::BLOOM_BLUR_FRAG_SPV = "res/spv/BloomBlur.frag.spv";
const char *BloomRenderer::BLOOM_PREFILTER_FRAG_SPV =
  "res/spv/BloomPrefilter.frag.spv";
const char *BloomRenderer::BLOOM_ADDITIVE_FRAG_SPV =
  "res/spv/BloomAdditive.frag.spv";

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

    mAdditivePipeline.init(
      [](VulkanPipeline &res, BloomRenderer &owner, VulkanContext &context) {
        VulkanPipelineConfig pipelineConfig(
          {owner.mRenderPass, 0},
          VulkanShader(context.device(), "res/spv/TexturedQuad.vert.spv"),
          VulkanShader(context.device(), BLOOM_ADDITIVE_FRAG_SPV));

        pipelineConfig.configurePipelineLayout(
          sizeof(BloomProperties),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
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

  mProperties.intensity = glm::vec4(0.003f);
  mProperties.threshold = 1.0f;
}

void BloomRenderer::render(VulkanFrame &frame, const RenderStage &previous) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  { // Prefilter stage
    commandBuffer.dbgBeginRegion(
      "BloomPrefilterStage", DBG_BLOOM_PREFILTER_COLOR);

    commandBuffer.beginRenderPass(
      mRenderPass,
      mPrefilteredTarget.fbo,
      {},
      mPrefilteredExtent);

    commandBuffer.bindPipeline(mPrefilterPipeline.res);
    commandBuffer.bindUniforms(previous.uniform());
    commandBuffer.pushConstants(sizeof(BloomProperties), &mProperties);

    commandBuffer.setViewport();
    commandBuffer.setScissor();

    commandBuffer.draw(4, 1, 0, 0);

    commandBuffer.endRenderPass();

    commandBuffer.dbgEndRegion();
  }

  { // Blur passes
    VulkanUniform *previousOutput = &mPrefilteredTarget.uniform;

    auto properties = mProperties;

    char dbgName[] = "BloomBlur0HStage";

    for (int targetIdx = 0; targetIdx < mBlurTargets.size; ++targetIdx) {
      auto *pair = &mBlurTargets[targetIdx];

      dbgName[10] = 'H';

      properties.horizontal = true;
      properties.scale = glm::vec4(
        1.0f / (float)pair->extent.width,
        1.0f / (float)pair->extent.height,
        0.0f, 1.0f);

      for (int pass = 0; pass < 2; ++pass) {
        dbgName[9] = targetIdx + '0';
        commandBuffer.dbgBeginRegion(
          dbgName, DBG_BLOOM_PREFILTER_COLOR);

        commandBuffer.beginRenderPass(
          mRenderPass,
          pair->blurTargets[pass].fbo,
          {},
          pair->extent);

        commandBuffer.bindPipeline(mBlurPipeline.res);
        commandBuffer.bindUniforms(*previousOutput);
        commandBuffer.pushConstants(sizeof(BloomProperties), &properties);

        commandBuffer.setViewport();
        commandBuffer.setScissor();

        commandBuffer.draw(4, 1, 0, 0);

        commandBuffer.endRenderPass();

        commandBuffer.dbgEndRegion();

        properties.horizontal = !properties.horizontal;
        dbgName[10] = 'V';
        previousOutput = &pair->blurTargets[pass].uniform;
      }
    }
  }

  { // Addition passes
    VulkanUniform *previousOutput = &mPrefilteredTarget.uniform;

    auto properties = mProperties;

    char dbgName[] = "BloomAdditive0Stage";

    for (int targetIdx = mBlurTargets.size - 2; targetIdx >= 0; --targetIdx) {
      auto *highres = &mBlurTargets[targetIdx];
      auto *lowres = &mBlurTargets[targetIdx + 1];

      dbgName[13] = targetIdx + '0';
      commandBuffer.dbgBeginRegion(
        dbgName, DBG_BLOOM_PREFILTER_COLOR);

      commandBuffer.beginRenderPass(
        mRenderPass,
        highres->additiveTarget.fbo,
        {},
        highres->extent);

      commandBuffer.bindPipeline(mAdditivePipeline.res);

      if (targetIdx == mBlurTargets.size - 2) {
        commandBuffer.bindUniforms(
          lowres->blurTargets[1].uniform,
          highres->blurTargets[1].uniform);
      }
      else {
        commandBuffer.bindUniforms(
          lowres->additiveTarget.uniform,
          highres->blurTargets[1].uniform);
      }

      commandBuffer.pushConstants(sizeof(BloomProperties), &properties);

      commandBuffer.setViewport();
      commandBuffer.setScissor();

      commandBuffer.draw(4, 1, 0, 0);

      commandBuffer.endRenderPass();

      commandBuffer.dbgEndRegion();
    }
  }
}

void BloomRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  destroyTargets(vulkanContext);
  mExtent = {newResolution.width, newResolution.height};
  initTargets(vulkanContext);
}

const VulkanRenderPass &BloomRenderer::renderPass() const {
  return mRenderPass;
}

const VulkanFramebuffer &BloomRenderer::framebuffer() const {
  return NullReference<VulkanFramebuffer>::nullRef;
  // TODO
}

const VulkanUniform &BloomRenderer::uniform() const {
  return mBlurTargets[0].additiveTarget.uniform;
  // return NullReference<VulkanUniform>::nullRef;
  // TODO
}

VkExtent2D BloomRenderer::extent() const {
  return mExtent;
}

void BloomRenderer::initTargets(VulkanContext &graphicsContext) {
  VkExtent2D extent = {mExtent.width, mExtent.height};

  /* 
     Initialise the prefiltering target which will be at half the resolution 
     of the whole rendering pipeline.
     
     NOTE: To tinker with a little...
  */
  initTarget(mPrefilteredTarget, extent, graphicsContext);

  mPrefilteredExtent = extent;
  LOG_INFOV(
    "Prefilter Extent: %dx%d\n",
    (int)extent.width,
    (int)extent.height);

  uint32_t blurTargetCount = glm::log2((float)extent.width) / 2 + 1;
  mBlurTargets.init(blurTargetCount);

  for (int i = 0; i < blurTargetCount; ++i) {
    VkExtent2D currentExtent = extent;
    currentExtent.width /= glm::pow(2, i);
    currentExtent.height /= glm::pow(2, i);

    LOG_INFOV(
      "Extent: %dx%d\n",
      (int)currentExtent.width,
      (int)currentExtent.height);

    mBlurTargets[i].extent = currentExtent;
    initTarget(mBlurTargets[i].blurTargets[0], currentExtent, graphicsContext);
    initTarget(mBlurTargets[i].blurTargets[1], currentExtent, graphicsContext);
    initTarget(mBlurTargets[i].additiveTarget, currentExtent, graphicsContext);
  }

  mBlurTargets.size = blurTargetCount;
}

void BloomRenderer::initTarget(
  Target &target,
  VkExtent2D extent,
  VulkanContext &graphicsContext) {
  target.texture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, BLOOM_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    {extent.width, extent.height, 1}, 1, 1);

  target.uniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(target.texture));

  VulkanFramebufferConfig fboConfig(1, mRenderPass);
  fboConfig.addAttachment(target.texture);
  target.fbo.init(graphicsContext.device(), fboConfig);
}

void BloomRenderer::destroyTarget(Target &target, VulkanContext &context) {
  target.uniform.destroy(context.device(), context.descriptorPool());
  target.fbo.destroy(context.device());
  target.texture.destroy(context.device());
}

void BloomRenderer::destroyTargets(VulkanContext &graphicsContext) {
  VkExtent2D extent = {mExtent.width / 2, mExtent.height / 2};

  destroyTarget(mPrefilteredTarget, graphicsContext);

  uint32_t blurTargetCount = glm::log2((float)extent.width) / 2 + 1;

  for (int i = 0; i < blurTargetCount; ++i) {
    destroyTarget(mBlurTargets[i].blurTargets[0], graphicsContext);
    destroyTarget(mBlurTargets[i].blurTargets[1], graphicsContext);
    destroyTarget(mBlurTargets[i].additiveTarget, graphicsContext);
  }

  mBlurTargets.free();
}

}
