#include "yona_app.hpp"
#include "yona_filesystem.hpp"
#include "yona_renderer_sky.hpp"
#include "yona_vulkan_context.hpp"
#include "yona_vulkan_texture.hpp"

namespace Yona {

void RendererSky::init(VulkanContext &graphicsContext) {
  initSkyProperties(graphicsContext);
  preparePrecompute(graphicsContext);
  precompute(graphicsContext);
}

void RendererSky::tick(VulkanFrame &frame) {
  // precomputeTransmittance(frame.primaryCommandBuffer);
  precomputeSingleScattering(frame.primaryCommandBuffer);
}

void RendererSky::initSkyProperties(VulkanContext &graphicsContext) {
  /* 
     Based on the values of Eric Bruneton's atmosphere model.
     We aren't going to calculate these manually come on (at least not yet)
  */

  /* 
     These are the irradiance values at the top of the Earth's atmosphere
     for the wavelengths 680nm (red), 550nm (green) and 440nm (blue) 
     respectively. These are in W/m2
  */
  mSkyProperties.solarIrradiance = glm::vec3(1.474f, 1.8504f, 1.91198f);
  // Angular radius of the Sun (radians)
  mSkyProperties.solarAngularRadius = 0.004675f;
  mSkyProperties.bottomRadius = 6360.0f;
  mSkyProperties.topRadius = 6420.0f;

  mSkyProperties.rayleighDensity.layers[0] =
    DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
  mSkyProperties.rayleighDensity.layers[1] =
    DensityLayer { 0.0f, 1.0f, -0.125f, 0.0f, 0.0f };
  mSkyProperties.rayleighScatteringCoef =
    glm::vec3(0.005802f, 0.013558f, 0.033100f);

  mSkyProperties.mieDensity.layers[0] =
    DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
  mSkyProperties.mieDensity.layers[1] =
    DensityLayer { 0.0f, 1.0f, -0.833333f, 0.0f, 0.0f };
  mSkyProperties.mieScatteringCoef = glm::vec3(0.003996f, 0.003996f, 0.003996f);
  mSkyProperties.mieExtinctionCoef = glm::vec3(0.004440f, 0.004440f, 0.004440f);

  mSkyProperties.miePhaseFunctionG = 0.8f;

  mSkyProperties.absorptionDensity.layers[0] =
    DensityLayer { 25.000000f, 0.000000f, 0.000000f, 0.066667f, -0.666667f };
  mSkyProperties.absorptionDensity.layers[1] =
    DensityLayer { 0.000000f, 0.000000f, 0.000000f, -0.066667f, 2.666667f };
  mSkyProperties.absorptionExtinctionCoef =
    glm::vec3(0.000650f, 0.001881f, 0.000085f);
  mSkyProperties.groundAlbedo = glm::vec3(0.100000f, 0.100000f, 0.100000f);
  mSkyProperties.muSunMin = -0.207912f;

  mSkyPropertiesBuffer.init(
    graphicsContext.device(),
    sizeof(SkyProperties),
    (int)VulkanBufferFlag::UniformBuffer);

  mSkyPropertiesBuffer.fillWithStaging(
    graphicsContext.device(),
    graphicsContext.commandPool(),
    {(uint8_t *)&mSkyProperties, sizeof(mSkyProperties)});

  mSkyPropertiesUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mSkyPropertiesBuffer));
}

void RendererSky::initTemporaryPrecomputeTextures(
  VulkanContext &graphicsContext) {
    VkExtent3D extent = {
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH
    };
  
  mDeltaRayleighScatteringTexture.init(
    graphicsContext.device(), TextureType::T3D | TextureType::Attachment,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    extent, 1, 1);

  mDeltaMieScatteringTexture.init(
    graphicsContext.device(), TextureType::T3D | TextureType::Attachment,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    extent, 1, 1);
}

void RendererSky::preparePrecompute(VulkanContext &graphicsContext) {
  File precomputeVshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_precompute.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  File precomputeGshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_precompute.geom.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer quadVsh = precomputeVshFile.readBinary();
  Buffer quadGsh = precomputeGshFile.readBinary();

  initTemporaryPrecomputeTextures(graphicsContext);

  prepareTransmittancePrecompute(quadVsh, graphicsContext);
  prepareSingleScatteringPrecompute(quadVsh, quadGsh, graphicsContext);
}

void RendererSky::prepareTransmittancePrecompute(
  const Buffer &precomputeVsh,
  VulkanContext &graphicsContext) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mPrecomputeTransmittanceRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }

  { // Create pipeline
    File precomputeTransmittance = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/sky_transmittance.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer fsh = precomputeTransmittance.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeTransmittanceRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    pipelineConfig.configurePipelineLayout(
      0, VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    mPrecomputeTransmittancePipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create attachments and framebuffer
    mPrecomputedTransmittance.init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
      {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, 1}, 1, 1);

    VulkanFramebufferConfig fboConfig(1, mPrecomputeTransmittanceRenderPass);
    fboConfig.addAttachment(mPrecomputedTransmittance);

    mPrecomputeTransmittanceFBO.init(graphicsContext.device(), fboConfig);
  }

  { // Create uniform for transmittance texture
    mPrecomputedTransmittanceUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(
        mPrecomputedTransmittance));
  }
}

void RendererSky::prepareSingleScatteringPrecompute(
  const Buffer &precomputeVsh,
  const Buffer &precomputeGsh,
  VulkanContext &graphicsContext) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(3, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    // The scattering texture
    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U, 1U, 2U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mPrecomputeSingleScatteringRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }
  
  { // Create pipeline
    File precomputeSingleScattering = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/sky_single_scattering.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer fsh = precomputeSingleScattering.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeSingleScatteringRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), precomputeGsh, VulkanShaderType::Geometry),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    pipelineConfig.configurePipelineLayout(
      sizeof(SingleScatteringPushConstant),
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

    mPrecomputeSingleScatteringPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create attachments and framebuffer
    VkExtent3D extent = {
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH
    };

    mPrecomputedSingleScattering.init(
      graphicsContext.device(), TextureType::T3D | TextureType::Attachment,
      TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
      extent, 1, 1);

    VulkanFramebufferConfig fboConfig(3, mPrecomputeSingleScatteringRenderPass);
    fboConfig.addAttachment(mDeltaRayleighScatteringTexture);
    fboConfig.addAttachment(mDeltaMieScatteringTexture);
    fboConfig.addAttachment(mPrecomputedSingleScattering);

    mPrecomputeSingleScatteringFBO.init(graphicsContext.device(), fboConfig);
  }
}

void RendererSky::precompute(VulkanContext &graphicsContext) {
  const auto &commandPool = graphicsContext.commandPool();
  const auto &device = graphicsContext.device();
  const auto &queue = device.graphicsQueue();

  VulkanCommandBuffer commandBuffer = commandPool.makeCommandBuffer(
    device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  { // Precompute
    precomputeTransmittance(commandBuffer);
    precomputeSingleScattering(commandBuffer);
  }
  commandBuffer.end();

  queue.submitCommandBuffer(
    commandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());

  queue.idle();
}

void RendererSky::precomputeTransmittance(
  VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeTransmittanceRenderPass,
    mPrecomputeTransmittanceFBO,
    {}, extent);

  commandBuffer.bindPipeline(mPrecomputeTransmittancePipeline);
  commandBuffer.bindUniforms(mSkyPropertiesUniform);

  commandBuffer.setViewport(extent);
  commandBuffer.setScissor({}, extent);

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

void RendererSky::precomputeSingleScattering(
  VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeSingleScatteringRenderPass,
    mPrecomputeSingleScatteringFBO, {}, extent);

  SingleScatteringPushConstant pushConstant = {};

  for (int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
    pushConstant.layer = layer;

    commandBuffer.bindPipeline(mPrecomputeSingleScatteringPipeline);
    commandBuffer.bindUniforms(
      mSkyPropertiesUniform, mPrecomputedTransmittanceUniform);

    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

    commandBuffer.setViewport(extent);
    commandBuffer.setScissor({}, extent);

    commandBuffer.draw(4, 1, 0, 0);
  }

  commandBuffer.endRenderPass();
}

}
