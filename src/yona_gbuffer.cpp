#include "yona_gbuffer.hpp"

namespace Yona {

void GBuffer::init(VulkanContext &graphicsContext) {
  auto ctxProperties = graphicsContext.getProperties();

  mGBufferExtent = {
    ctxProperties.swapchainExtent.width,
    ctxProperties.swapchainExtent.height,
  };

  VkExtent3D extent = {mGBufferExtent.width, mGBufferExtent.height, 1};

  VkFormat attachmentFormats[] = {
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    ctxProperties.depthFormat
  };

  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(3, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      attachmentFormats[Albedo]);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      attachmentFormats[Normal]);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::ClearThenStore,
      OutputUsage::FragmentShaderRead, AttachmentType::Depth,
      attachmentFormats[Depth]);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U, 1U),
      makeArray<uint32_t, AllocationType::Linear>(),
      true);

    mGBufferRenderPass.init(graphicsContext.device(), renderPassConfig);
  }

  { // Create attachment textures
    mGBufferTextures.init(3);

    mGBufferTextures[Albedo].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, attachmentFormats[Albedo], VK_FILTER_LINEAR,
      extent, 1, 1);

    mGBufferTextures[Normal].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, attachmentFormats[Normal], VK_FILTER_LINEAR,
      extent, 1, 1);

    mGBufferTextures[Depth].init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Depth, attachmentFormats[Depth], VK_FILTER_LINEAR,
      extent, 1, 1);

    mAlbedoUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(
        mGBufferTextures[Albedo]));
  }

  { // Create framebuffer
    VulkanFramebufferConfig fboConfig(1, mGBufferRenderPass);

    fboConfig.addAttachment(mGBufferTextures[Albedo]);
    fboConfig.addAttachment(mGBufferTextures[Normal]);
    fboConfig.addAttachment(mGBufferTextures[Depth]);

    mGBufferFBO.init(graphicsContext.device(), fboConfig);
  }
}

void GBuffer::beginRender(VulkanFrame &frame) {
  frame.primaryCommandBuffer.beginRenderPass(
    mGBufferRenderPass,
    mGBufferFBO,
    {}, mGBufferExtent);
}

void GBuffer::endRender(VulkanFrame &frame) {
  frame.primaryCommandBuffer.endRenderPass();
}

const VulkanRenderPass &GBuffer::renderPass() const {
  return mGBufferRenderPass;
}

const VulkanFramebuffer &GBuffer::framebuffer() const {
  return mGBufferFBO;
}

const VulkanUniform &GBuffer::uniform() const {
  return mAlbedoUniform;
}

VkExtent2D GBuffer::extent() const {
  return mGBufferExtent;
}

}
