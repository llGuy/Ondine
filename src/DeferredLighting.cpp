#include "FileSystem.hpp"
#include "Application.hpp"
#include "DeferredLighting.hpp"

namespace Ondine {

void DeferredLighting::init(VulkanContext &graphicsContext) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      VK_FORMAT_R8G8B8A8_UNORM);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mLightingRenderPass.init(graphicsContext.device(), renderPassConfig);
  }

  { // Create pipeline
    File lightingVert = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/TexturedQuad.vert.spv",
      FileOpenType::Binary | FileOpenType::In);

    File lightingFrag = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/Lighting.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer vsh = lightingVert.readBinary();
    Buffer fsh = lightingFrag.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mLightingRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), vsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    pipelineConfig.configurePipelineLayout(
      0,
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    mLightingPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create attachments and framebuffer
    auto ctxProperties = graphicsContext.getProperties();

    mLightingExtent = {
      ctxProperties.swapchainExtent.width,
      ctxProperties.swapchainExtent.height,
    };

    initTargets(graphicsContext);
  }
}

void DeferredLighting::render(
  VulkanFrame &frame,
  const GBuffer &gbuffer,
  const Camera &camera,
  const PlanetRenderer &planet) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.beginRenderPass(
    mLightingRenderPass,
    mLightingFBO,
    {}, mLightingExtent);

  commandBuffer.bindPipeline(mLightingPipeline);
  commandBuffer.bindUniforms(
    gbuffer.uniform(), camera.uniform(), planet.uniform());

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

void DeferredLighting::resize(
  VulkanContext &vulkanContext,
  Resolution newResolution) {
  destroyTargets(vulkanContext);
  mLightingExtent = {newResolution.width, newResolution.height};
  initTargets(vulkanContext);
}

const VulkanRenderPass &DeferredLighting::renderPass() const {
  return mLightingRenderPass;
}

const VulkanFramebuffer &DeferredLighting::framebuffer() const {
  return mLightingFBO;
}

const VulkanUniform &DeferredLighting::uniform() const {
  return mLightingUniform;
}

VkExtent2D DeferredLighting::extent() const {
  return mLightingExtent;
}

void DeferredLighting::initTargets(VulkanContext &graphicsContext) {
  mLightingTexture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR,
    {mLightingExtent.width, mLightingExtent.height, 1}, 1, 1);

  mLightingUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mLightingTexture));

  VulkanFramebufferConfig fboConfig(1, mLightingRenderPass);
  fboConfig.addAttachment(mLightingTexture);
  mLightingFBO.init(graphicsContext.device(), fboConfig);
}

void DeferredLighting::destroyTargets(VulkanContext &graphicsContext) {
  mLightingUniform.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());
  mLightingFBO.destroy(graphicsContext.device());
  mLightingTexture.destroy(graphicsContext.device());
}

}
