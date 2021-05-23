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
  // precomputeSingleScattering(frame.primaryCommandBuffer);
  // precomputeDirectIrradiance(frame.primaryCommandBuffer);
  // precomputeScatteringDensity(
  // frame.primaryCommandBuffer, 2);
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
  VkExtent3D extent3D = {
    SCATTERING_TEXTURE_WIDTH,
    SCATTERING_TEXTURE_HEIGHT,
    SCATTERING_TEXTURE_DEPTH
  };

  make3DTextureAndUniform(
    extent3D, mDeltaRayleighScatteringTexture,
    mDeltaRayleighScatteringUniform, graphicsContext);

  make3DTextureAndUniform(
    extent3D, mDeltaMieScatteringTexture,
    mDeltaMieScatteringUniform, graphicsContext);
  
  make3DTextureAndUniform(
    extent3D, mDeltaScatteringDensityTexture,
    mDeltaScatteringDensityUniform, graphicsContext);

  make3DTextureAndUniform(
    extent3D, mDeltaMultipleScatteringTexture,
    mDeltaMultipleScatteringUniform, graphicsContext);

  make2DTextureAndUniform(
    {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1},
    mDeltaIrradianceTexture, mDeltaIrradianceUniform, graphicsContext);
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
  prepareDirectIrradiancePrecompute(quadVsh, graphicsContext);
  prepareScatteringDensityPrecompute(quadVsh, quadGsh, graphicsContext);
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
    make2DTextureAndUniform(
      {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, 1},
      mPrecomputedTransmittance, mPrecomputedTransmittanceUniform,
      graphicsContext);

    VulkanFramebufferConfig fboConfig(1, mPrecomputeTransmittanceRenderPass);
    fboConfig.addAttachment(mPrecomputedTransmittance);

    mPrecomputeTransmittanceFBO.init(graphicsContext.device(), fboConfig);
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
      sizeof(PrecomputePushConstant),
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

    make3DTextureAndUniform(
      extent, mPrecomputedScattering, mPrecomputedScatteringUniform,
      graphicsContext);

    VulkanFramebufferConfig fboConfig(3, mPrecomputeSingleScatteringRenderPass);
    fboConfig.addAttachment(mDeltaRayleighScatteringTexture);
    fboConfig.addAttachment(mDeltaMieScatteringTexture);
    fboConfig.addAttachment(mPrecomputedScattering);

    mPrecomputeSingleScatteringFBO.init(graphicsContext.device(), fboConfig);
  }
}

void RendererSky::prepareDirectIrradiancePrecompute(
  const Buffer &precomputeVsh,
  VulkanContext &graphicsContext) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(2, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U, 1U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mPrecomputeDirectIrradianceRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }

  { // Create pipeline
    File precomputeDirectIrradiance = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/sky_direct_irradiance.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer fsh = precomputeDirectIrradiance.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeDirectIrradianceRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    pipelineConfig.configurePipelineLayout(
      0,
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

    mPrecomputeDirectIrradiancePipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create attachments and framebuffer
    VkExtent3D extent = {
      IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1
    };

    make2DTextureAndUniform(
      extent, mPrecomputedIrradiance, mPrecomputedIrradianceUniform,
      graphicsContext);

    VulkanFramebufferConfig fboConfig(2, mPrecomputeDirectIrradianceRenderPass);
    fboConfig.addAttachment(mDeltaIrradianceTexture);
    fboConfig.addAttachment(mPrecomputedIrradiance);

    mPrecomputeDirectIrradianceFBO.init(graphicsContext.device(), fboConfig);
  }
}

void RendererSky::prepareScatteringDensityPrecompute(
  const Buffer &precomputeVsh,
  const Buffer &precomputeGsh,
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

    mPrecomputeScatteringDensityRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }
  
  { // Create pipeline
    File precomputeScatteringDensity = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/sky_scattering_density.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer fsh = precomputeScatteringDensity.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeScatteringDensityRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), precomputeGsh, VulkanShaderType::Geometry),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    VulkanPipelineDescriptorLayout textureUL =
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

    pipelineConfig.configurePipelineLayout(
      sizeof(PrecomputePushConstant),
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      textureUL, textureUL, textureUL, textureUL, textureUL);

    mPrecomputeScatteringDensityPipeline.init(
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

    make3DTextureAndUniform(
      extent, mDeltaScatteringDensityTexture, mDeltaScatteringDensityUniform,
      graphicsContext);

    VulkanFramebufferConfig fboConfig(1, mPrecomputeScatteringDensityRenderPass);
    fboConfig.addAttachment(mDeltaScatteringDensityTexture);

    mPrecomputeScatteringDensityFBO.init(graphicsContext.device(), fboConfig);
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
    precomputeDirectIrradiance(commandBuffer);

    commandBuffer.transitionImageLayout(
      mDeltaMultipleScatteringTexture,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    for (
      int scatteringOrder = 2;
      scatteringOrder <= NUM_SCATTERING_ORDERS;
      ++scatteringOrder) {
      // precomputeScatteringDensity(commandBuffer, scatteringOrder);
    }
  }
  commandBuffer.end();

  queue.submitCommandBuffer(
    commandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());

  queue.idle();

  commandBuffer = commandPool.makeCommandBuffer(
    device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  { // Precompute
    for (
      int scatteringOrder = 2;
      scatteringOrder <= NUM_SCATTERING_ORDERS;
      ++scatteringOrder) {
      precomputeScatteringDensity(commandBuffer, scatteringOrder);
    }
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

  PrecomputePushConstant pushConstant = {};

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

void RendererSky::precomputeDirectIrradiance(
  VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeDirectIrradianceRenderPass,
    mPrecomputeDirectIrradianceFBO, {}, extent);

  commandBuffer.bindPipeline(mPrecomputeDirectIrradiancePipeline);
  commandBuffer.bindUniforms(
    mSkyPropertiesUniform, mPrecomputedTransmittanceUniform);

  commandBuffer.setViewport(extent);
  commandBuffer.setScissor({}, extent);

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

void RendererSky::precomputeScatteringDensity(
  VulkanCommandBuffer &commandBuffer,
  int scatteringOrder) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeScatteringDensityRenderPass,
    mPrecomputeScatteringDensityFBO, {}, extent);

  PrecomputePushConstant pushConstant = {};

  for (int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
    pushConstant.layer = layer;
    pushConstant.scatteringOrder = scatteringOrder;

    commandBuffer.bindPipeline(mPrecomputeScatteringDensityPipeline);

    commandBuffer.bindUniforms(
      mSkyPropertiesUniform,
      mPrecomputedTransmittanceUniform,
      mDeltaRayleighScatteringUniform,
      mDeltaMieScatteringUniform,
      mDeltaMultipleScatteringUniform,
      mPrecomputedIrradianceUniform);

    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

    commandBuffer.setViewport(extent);
    commandBuffer.setScissor({}, extent);

    commandBuffer.draw(4, 1, 0, 0);
  }

  commandBuffer.endRenderPass();
}

void RendererSky::make3DTextureAndUniform(
  const VkExtent3D extent,
  VulkanTexture &texture,
  VulkanUniform &uniform,
  VulkanContext &graphicsContext) {
  texture.init(
    graphicsContext.device(), TextureType::T3D | TextureType::Attachment,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    extent, 1, 1);

  uniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture));
}

void RendererSky::make2DTextureAndUniform(
  const VkExtent3D extent,
  VulkanTexture &texture,
  VulkanUniform &uniform,
  VulkanContext &graphicsContext) {
  texture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT, VK_FILTER_LINEAR,
    extent, 1, 1);

  uniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture));
}

}
