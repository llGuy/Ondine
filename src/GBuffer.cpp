#include "GBuffer.hpp"

namespace Ondine::Graphics {

VulkanRenderPass *GBuffer::sGBufferRenderPass = nullptr;

void GBuffer::init(
  VulkanContext &graphicsContext, const VkExtent2D &extent) {
  auto ctxProperties = graphicsContext.getProperties();

  mGBufferExtent = {
    ctxProperties.swapchainExtent.width,
    ctxProperties.swapchainExtent.height,
  };

  mGBufferFormats[0] = VK_FORMAT_R16G16B16A16_SFLOAT;
  mGBufferFormats[1] = VK_FORMAT_R16G16B16A16_SFLOAT;
  mGBufferFormats[2] = VK_FORMAT_R32G32B32A32_SFLOAT;
  mGBufferFormats[3] = ctxProperties.depthFormat;

  if (!sGBufferRenderPass) { // Create render pass
    sGBufferRenderPass = flAlloc<VulkanRenderPass>();

    VulkanRenderPassConfig renderPassConfig(4, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      mGBufferFormats[Albedo]);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      mGBufferFormats[Normal]);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      mGBufferFormats[Position]);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Depth,
      mGBufferFormats[Depth]);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U, 1U, 2U),
      makeArray<uint32_t, AllocationType::Linear>(),
      true);

    sGBufferRenderPass->init(graphicsContext.device(), renderPassConfig);
  }

  initTargets(graphicsContext);
}

void GBuffer::beginRender(VulkanFrame &frame) {
  frame.primaryCommandBuffer.beginRenderPass(
    *sGBufferRenderPass,
    mGBufferFBO,
    {}, mGBufferExtent);
}

void GBuffer::endRender(VulkanFrame &frame) {
  frame.primaryCommandBuffer.endRenderPass();
}

void GBuffer::resize(VulkanContext &vulkanContext, Resolution newResolution) {
  destroyTargets(vulkanContext);
  mGBufferExtent = {newResolution.width, newResolution.height};
  initTargets(vulkanContext);
}

const VulkanRenderPass &GBuffer::renderPass() const {
  return *sGBufferRenderPass;
}

const VulkanFramebuffer &GBuffer::framebuffer() const {
  return mGBufferFBO;
}

const VulkanUniform &GBuffer::uniform() const {
  return mGBufferUniform;
}

VkExtent2D GBuffer::extent() const {
  return mGBufferExtent;
}

void GBuffer::initTargets(VulkanContext &graphicsContext) {
  { // Create attachment textures
    mGBufferTextures[Albedo].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, mGBufferFormats[Albedo], VK_FILTER_LINEAR,
      {mGBufferExtent.width, mGBufferExtent.height, 1}, 1, 1);

    mGBufferTextures[Normal].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, mGBufferFormats[Normal], VK_FILTER_LINEAR,
      {mGBufferExtent.width, mGBufferExtent.height, 1}, 1, 1);

    mGBufferTextures[Position].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, mGBufferFormats[Position], VK_FILTER_LINEAR,
      {mGBufferExtent.width, mGBufferExtent.height, 1}, 1, 1);

    mGBufferTextures[Depth].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Depth, mGBufferFormats[Depth], VK_FILTER_NEAREST,
      {mGBufferExtent.width, mGBufferExtent.height, 1}, 1, 1);

    mAlbedoUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(
        mGBufferTextures[Albedo]));

    mGBufferUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(
        mGBufferTextures[Albedo],
        mGBufferTextures[Normal],
        mGBufferTextures[Position],
        mGBufferTextures[Depth]));
  }

  { // Create framebuffer
    VulkanFramebufferConfig fboConfig(4, *sGBufferRenderPass);

    fboConfig.addAttachment(mGBufferTextures[Albedo]);
    fboConfig.addAttachment(mGBufferTextures[Normal]);
    fboConfig.addAttachment(mGBufferTextures[Position]);
    fboConfig.addAttachment(mGBufferTextures[Depth]);

    mGBufferFBO.init(graphicsContext.device(), fboConfig);
  }
}

void GBuffer::destroyTargets(VulkanContext &graphicsContext) {
  mAlbedoUniform.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());

  mGBufferUniform.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());

  mGBufferFBO.destroy(graphicsContext.device());

  for (int i = 0; i < Count; ++i) {
    mGBufferTextures[i].destroy(graphicsContext.device());
  }
}

}
