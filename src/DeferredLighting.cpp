#include "FileSystem.hpp"
#include "Application.hpp"
#include "WaterRenderer.hpp"
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

const char *const DeferredLighting::LIGHTING_FRAG_SPV =
  "res/spv/Lighting.frag.spv";
const char *const DeferredLighting::LIGHTING_REFL_FRAG_SPV =
  "res/spv/LightingRefl.frag.spv";

void DeferredLighting::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent,
  const LightingProperties *properties) {
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

  { // Add tracked resources
    addTrackedPath(LIGHTING_FRAG_SPV, &mLightingPipeline);
    addTrackedPath(LIGHTING_REFL_FRAG_SPV, &mLightingReflPipeline);
  }

  { // Create pipeline
    mLightingPipeline.init(
      [](VulkanPipeline &res, DeferredLighting &owner,
         VulkanContext &graphicsContext) {
        Core::File lightingVert = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          "res/spv/Lighting.vert.spv",
          Core::FileOpenType::Binary | Core::FileOpenType::In);
        Buffer vsh = lightingVert.readBinary();

        Core::File lightingFrag = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          LIGHTING_FRAG_SPV,
          Core::FileOpenType::Binary | Core::FileOpenType::In);

        Buffer fsh = lightingFrag.readBinary();

        VulkanPipelineConfig pipelineConfig(
          {owner.mLightingRenderPass, 0},
          VulkanShader(
            graphicsContext.device(), vsh, VulkanShaderType::Vertex),
          VulkanShader(
            graphicsContext.device(), fsh, VulkanShaderType::Fragment));

        pipelineConfig.configurePipelineLayout(
          0,
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4});

        res.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      },
      *this,
      graphicsContext);
  }

  { // Create pipeline which renders reflections
    mLightingReflPipeline.init(
      [](VulkanPipeline &res, DeferredLighting &owner,
         VulkanContext &graphicsContext) {
        Core::File lightingVert = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          "res/spv/Lighting.vert.spv",
          Core::FileOpenType::Binary | Core::FileOpenType::In);
        Buffer vsh = lightingVert.readBinary();

        Core::File lightingFrag = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          LIGHTING_REFL_FRAG_SPV,
          Core::FileOpenType::Binary | Core::FileOpenType::In);

        Buffer fsh = lightingFrag.readBinary();

        VulkanPipelineConfig pipelineConfig(
          {owner.mLightingRenderPass, 0},
          VulkanShader(
            graphicsContext.device(), vsh, VulkanShaderType::Vertex),
          VulkanShader(
            graphicsContext.device(), fsh, VulkanShaderType::Fragment));

        pipelineConfig.configurePipelineLayout(
          0,
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

        res.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      },
      *this,
      graphicsContext);
  }

  { // Create attachments and framebuffer
    mLightingExtent = {
      initialExtent.width,
      initialExtent.height,
    };

    initTargets(graphicsContext);
  }

  { // Create lighting properties uniform buffer
    mLightingPropertiesBuffer.init(
      graphicsContext.device(),
      sizeof(LightingProperties),
      (int)VulkanBufferFlag::UniformBuffer);

    mLightingPropertiesUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanBuffer, AllocationType::Linear>(mLightingPropertiesBuffer));

    if (properties) {
      mLightingPropertiesBuffer.fillWithStaging(
        graphicsContext.device(),
        graphicsContext.commandPool(),
        {(uint8_t *)properties, sizeof(LightingProperties)});
    }
  }
}

void DeferredLighting::render(
  VulkanFrame &frame,
  const GBuffer &gbuffer,
  const Camera &camera,
  const PlanetRenderer &planet,
  const SkyRenderer &sky) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.beginRenderPass(
    mLightingRenderPass,
    mLightingFBO,
    {}, mLightingExtent);

  commandBuffer.bindPipeline(mLightingPipeline.res);
  commandBuffer.bindUniforms(
    gbuffer.uniform(),
    camera.uniform(),
    planet.uniform(),
    mLightingPropertiesUniform,
    sky.uniform());

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

void DeferredLighting::render(
  VulkanFrame &frame, const GBuffer &gbuffer,
  const Camera &camera, const PlanetRenderer &planet,
  const WaterRenderer &water,
  const SkyRenderer &sky) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.beginRenderPass(
    mLightingRenderPass,
    mLightingFBO,
    {}, mLightingExtent);

  commandBuffer.bindPipeline(mLightingReflPipeline.res);
  commandBuffer.bindUniforms(
    gbuffer.uniform(),
    camera.uniform(),
    planet.uniform(),
    mLightingPropertiesUniform,
    sky.uniform(),
    water.uniform());

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
  return mLightingOutputUniform;
}

VkExtent2D DeferredLighting::extent() const {
  return mLightingExtent;
}

void DeferredLighting::initTargets(VulkanContext &graphicsContext) {
  mLightingTexture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR,
    {mLightingExtent.width, mLightingExtent.height, 1}, 1, 1);

  mLightingOutputUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mLightingTexture));

  VulkanFramebufferConfig fboConfig(1, mLightingRenderPass);
  fboConfig.addAttachment(mLightingTexture);
  mLightingFBO.init(graphicsContext.device(), fboConfig);
}

void DeferredLighting::destroyTargets(VulkanContext &graphicsContext) {
  mLightingOutputUniform.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());
  mLightingFBO.destroy(graphicsContext.device());
  mLightingTexture.destroy(graphicsContext.device());
}

void DeferredLighting::updateData(
  const VulkanCommandBuffer &commandBuffer,
  const LightingProperties &properties) {
  commandBuffer.updateBuffer(
    mLightingPropertiesBuffer, 0, sizeof(properties), &properties);
}

}
