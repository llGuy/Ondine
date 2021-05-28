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

  mViewDistanceMeters = 9000.000000;
  mViewZenithAngleRadians = 1.470000;
  mViewAzimuthAngleRadians = 0.000000;
  mSunZenithAngleRadians = 1.300000;
  mSunAzimuthAngleRadians = 3.000000;
  mExposure = 10.000000;
}

void RendererSky::tick(const Tick &tick, VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mDemo);

  mSunZenithAngleRadians += tick.dt * 0.01f;

  commandBuffer.bindUniforms(
    mSkyPropertiesUniform,
    mPrecomputedTransmittanceUniform,
    mPrecomputedScatteringUniform,
    mDeltaMieScatteringUniform,
    mPrecomputedIrradianceUniform);

  float cos_z = cos(mViewZenithAngleRadians);
  float sin_z = sin(mViewZenithAngleRadians);
  float cos_a = cos(mViewAzimuthAngleRadians);
  float sin_a = sin(mViewAzimuthAngleRadians);
  float ux[3] = { -sin_a, cos_a, 0.0 };
  float uy[3] = { -cos_z * cos_a, -cos_z * sin_a, sin_z };
  float uz[3] = { sin_z * cos_a, sin_z * sin_a, cos_z };
  float l = mViewDistanceMeters / 1000.0f;

  DemoPushConstant pushConstant = {};
  
  float model_from_view[16] = {
    // [0]    [1]    [2]       [3]
      ux[0], uy[0], uz[0], uz[0] * l,
    // [4]    [5]    [6]       [7]
      ux[1], uy[1], uz[1], uz[1] * l,
    // [8]    [9]    [10]      [11]
      ux[2], uy[2], uz[2], uz[2] * l,
    //[12]   [13]    [14]      [15]
      0.0,   0.0,   0.0,   1.0
  };

  memcpy(pushConstant.modelFromView, model_from_view, sizeof(float) * 16);

  std::swap(pushConstant.modelFromView[1], pushConstant.modelFromView[4]);
  std::swap(pushConstant.modelFromView[2], pushConstant.modelFromView[8]);
  std::swap(pushConstant.modelFromView[3], pushConstant.modelFromView[12]);
  std::swap(pushConstant.modelFromView[6], pushConstant.modelFromView[9]);
  std::swap(pushConstant.modelFromView[7], pushConstant.modelFromView[13]);
  std::swap(pushConstant.modelFromView[11], pushConstant.modelFromView[14]);

  const float kFovY = 50.0 / 180.0 * 3.1415f;
  const float kTanFovY = tan(kFovY / 2.0);
  float aspectRatio = static_cast<float>(frame.viewport.width) / frame.viewport.height;

  // Transform matrix from clip space to camera space (i.e. the inverse of a
  // GL_PROJECTION matrix).
  float view_from_clip[16] = {
    kTanFovY * aspectRatio, 0.0, 0.0, 0.0,
    0.0, kTanFovY, 0.0, 0.0,
    0.0, 0.0, 0.0, -1.0,
    0.0, 0.0, 1.0, 1.0
  };

  memcpy(pushConstant.viewFromClip, view_from_clip, sizeof(float) * 16);

  std::swap(pushConstant.viewFromClip[1], pushConstant.viewFromClip[4]);
  std::swap(pushConstant.viewFromClip[2], pushConstant.viewFromClip[8]);
  std::swap(pushConstant.viewFromClip[3], pushConstant.viewFromClip[12]);
  std::swap(pushConstant.viewFromClip[6], pushConstant.viewFromClip[9]);
  std::swap(pushConstant.viewFromClip[7], pushConstant.viewFromClip[13]);
  std::swap(pushConstant.viewFromClip[11], pushConstant.viewFromClip[14]);

  pushConstant.whitePoint = glm::vec3(1.0f);
  pushConstant.earthCenter = glm::vec3(0.0f, 0.0f, -6360.0f);
  pushConstant.sunSize = glm::vec2(0.0046750340586467079f, 0.99998907220740285f);
  pushConstant.camera = glm::vec3(
    model_from_view[3], model_from_view[7], model_from_view[11]);
  pushConstant.exposure = mExposure;
  pushConstant.sunDirection = glm::vec3(
    cos(mSunAzimuthAngleRadians) * sin(mSunZenithAngleRadians),
    sin(mSunAzimuthAngleRadians) * sin(mSunZenithAngleRadians),
    cos(mSunZenithAngleRadians));
  
  commandBuffer.pushConstants(sizeof(DemoPushConstant), &pushConstant);

  commandBuffer.setViewport({frame.viewport.width, frame.viewport.height});
  commandBuffer.setScissor({}, {frame.viewport.width, frame.viewport.height});

  commandBuffer.draw(4, 1, 0, 0);
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
    mDeltaRayleighScatteringUniform, graphicsContext, true);

  make3DTextureAndUniform(
    extent3D, mDeltaMieScatteringTexture,
    mDeltaMieScatteringUniform, graphicsContext, true);
  
  make3DTextureAndUniform(
    extent3D, mDeltaScatteringDensityTexture,
    mDeltaScatteringDensityUniform, graphicsContext, true);

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
  prepareMultipleScatteringPrecompute(quadVsh, quadGsh, graphicsContext);

  initDummyPipeline(quadVsh, graphicsContext);
}

void RendererSky::prepareTransmittancePrecompute(
  const Buffer &precomputeVsh,
  VulkanContext &graphicsContext) {
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT32);

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
      PRECOMPUTED_TEXTURE_FORMAT16);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT16);

    // The scattering texture
    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT16);

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
      graphicsContext, false);

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
      LoadAndStoreOp::LoadThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT32);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::LoadThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      PRECOMPUTED_TEXTURE_FORMAT32);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U, 1U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mPrecomputeDirectIrradianceRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }

  { // Create pipeline (direct irradiance)
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

  { // Create pipeline (indirect irradiance)
    File precomputeIndirectIrradiance = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/sky_indirect_irradiance.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer fsh = precomputeIndirectIrradiance.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeDirectIrradianceRenderPass, 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    pipelineConfig.enableBlendingSame(
      1, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);

    VulkanPipelineDescriptorLayout textureUL = {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1
    };

    pipelineConfig.configurePipelineLayout(
      sizeof(PrecomputePushConstant),
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      textureUL, textureUL, textureUL);

    mPrecomputeIndirectIrradiancePipeline.init(
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
  File precomputeScatteringDensity = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_scattering_density.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer fsh = precomputeScatteringDensity.readBinary();

  VkExtent3D extent = {
    SCATTERING_TEXTURE_WIDTH,
    SCATTERING_TEXTURE_HEIGHT,
    SCATTERING_TEXTURE_DEPTH
  };

  auto prepareProc =
    [this, &precomputeVsh, &precomputeGsh, &graphicsContext, &fsh] (
      LoadAndStoreOp loadAndStoreOp,
      OutputUsage outputUsage,
      VulkanPipeline &dstPipeline,
      VulkanRenderPass &dstRenderPass,
      VulkanFramebuffer &dstFramebuffer) {
      { // Create first render pass
        VulkanRenderPassConfig renderPassConfig(1, 1);
        
        renderPassConfig.addAttachment(
          loadAndStoreOp, LoadAndStoreOp::DontCareThenDontCare,
          outputUsage, AttachmentType::Color,
          PRECOMPUTED_TEXTURE_FORMAT16);

        renderPassConfig.addSubpass(
          makeArray<uint32_t, AllocationType::Linear>(0U),
          makeArray<uint32_t, AllocationType::Linear>(),
          false);

        dstRenderPass.init(graphicsContext.device(), renderPassConfig);
      }
  
      { // Create pipeline

        VulkanPipelineConfig pipelineConfig(
          {dstRenderPass, 0},
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

        dstPipeline.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      }

      { // Create attachments and framebuffer
        VulkanFramebufferConfig fboConfig(1, dstRenderPass);
        fboConfig.addAttachment(mDeltaScatteringDensityTexture);

        dstFramebuffer.init(graphicsContext.device(), fboConfig);
      }
    };

  mPrecomputeScatteringDensity.prepare(prepareProc);
}

void RendererSky::prepareMultipleScatteringPrecompute(
  const Buffer &precomputeVsh,
  const Buffer &precomputeGsh,
  VulkanContext &graphicsContext) {
  File precomputeMultipleScattering = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_multiple_scattering.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer fsh = precomputeMultipleScattering.readBinary();

  VkExtent3D extent = {
    SCATTERING_TEXTURE_WIDTH,
    SCATTERING_TEXTURE_HEIGHT,
    SCATTERING_TEXTURE_DEPTH
  };

  auto prepareProc =
    [this, &precomputeVsh, &precomputeGsh, &graphicsContext, &fsh] (
      LoadAndStoreOp loadAndStoreOp,
      OutputUsage outputUsage,
      VulkanPipeline &dstPipeline,
      VulkanRenderPass &dstRenderPass,
      VulkanFramebuffer &dstFramebuffer) {
      { // Create first render pass
        VulkanRenderPassConfig renderPassConfig(2, 1);
        
        renderPassConfig.addAttachment(
          LoadAndStoreOp::LoadThenStore, LoadAndStoreOp::DontCareThenDontCare,
          outputUsage, AttachmentType::Color,
          PRECOMPUTED_TEXTURE_FORMAT16);

        renderPassConfig.addAttachment(
          LoadAndStoreOp::LoadThenStore, LoadAndStoreOp::DontCareThenDontCare,
          outputUsage, AttachmentType::Color,
          PRECOMPUTED_TEXTURE_FORMAT16);

        renderPassConfig.addSubpass(
          makeArray<uint32_t, AllocationType::Linear>(0U, 1U),
          makeArray<uint32_t, AllocationType::Linear>(),
          false);

        dstRenderPass.init(graphicsContext.device(), renderPassConfig);
      }
  
      { // Create pipeline

        VulkanPipelineConfig pipelineConfig(
          {dstRenderPass, 0},
          VulkanShader(
            graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
          VulkanShader(
            graphicsContext.device(), precomputeGsh, VulkanShaderType::Geometry),
          VulkanShader(
            graphicsContext.device(), fsh, VulkanShaderType::Fragment));

        pipelineConfig.enableBlendingSame(
          1, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);

        VulkanPipelineDescriptorLayout textureUL =
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

        pipelineConfig.configurePipelineLayout(
          sizeof(PrecomputePushConstant),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
          textureUL, textureUL);

        dstPipeline.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      }

      { // Create attachments and framebuffer
        VulkanFramebufferConfig fboConfig(2, dstRenderPass);
        fboConfig.addAttachment(mDeltaMultipleScatteringTexture);
        fboConfig.addAttachment(mPrecomputedScattering);

        dstFramebuffer.init(graphicsContext.device(), fboConfig);
      }
    };

  mPrecomputeMultipleScattering.prepare(prepareProc);
}

void RendererSky::precompute(VulkanContext &graphicsContext) {
  const auto &commandPool = graphicsContext.commandPool();
  const auto &device = graphicsContext.device();
  const auto &queue = device.graphicsQueue();

  static constexpr uint32_t MAX_COMMAND_BUFFERS = 25;

  Array<VulkanCommandBuffer, AllocationType::Linear> commandBuffers(
    MAX_COMMAND_BUFFERS);

  auto recordComputation =
    [&commandBuffers, &commandPool, &graphicsContext] (auto computation) {
      const auto &commandPool = graphicsContext.commandPool();
      const auto &device = graphicsContext.device();

      auto &currentCommandBuffer = commandBuffers[commandBuffers.size];

      currentCommandBuffer = commandPool.makeCommandBuffer(
        device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

      currentCommandBuffer.begin(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

      // Record commands
      computation(currentCommandBuffer);

      currentCommandBuffer.end();

      commandBuffers.size++;
    };

  recordComputation([this](VulkanCommandBuffer &commandBuffer) {
    precomputeTransmittance(commandBuffer);
    precomputeSingleScattering(commandBuffer);
    precomputeDirectIrradiance(commandBuffer);
  });

  for (
    int scatteringOrder = 2;
    scatteringOrder <= NUM_SCATTERING_ORDERS;
    ++scatteringOrder) {

    // Compute the scattering density in two command buffers (quite intense)
    recordComputation(
      [this, scatteringOrder](VulkanCommandBuffer &commandBuffer) {
        precomputeScatteringDensity(
          commandBuffer, 0, scatteringOrder,
          0, SCATTERING_TEXTURE_DEPTH / 2);
      });

    recordComputation(
      [this, scatteringOrder](VulkanCommandBuffer &commandBuffer) {
        precomputeScatteringDensity(
          commandBuffer, 1, scatteringOrder,
          SCATTERING_TEXTURE_DEPTH / 2, SCATTERING_TEXTURE_DEPTH);
      });

    recordComputation(
      [this, scatteringOrder] (VulkanCommandBuffer &commandBuffer) {
        precomputeIndirectIrradiance(commandBuffer, scatteringOrder);
      });

    recordComputation(
      [this, scatteringOrder](VulkanCommandBuffer &commandBuffer) {
        precomputeMultipleScattering(
          commandBuffer, 0, scatteringOrder,
          0, SCATTERING_TEXTURE_DEPTH / 2);
      });

    recordComputation(
      [this, scatteringOrder](VulkanCommandBuffer &commandBuffer) {
        precomputeMultipleScattering(
          commandBuffer, 1, scatteringOrder,
          SCATTERING_TEXTURE_DEPTH / 2, SCATTERING_TEXTURE_DEPTH);
      });
  }

  /* Initialise the semaphores */
  Array<VulkanSemaphore, AllocationType::Linear> semaphores(
    commandBuffers.size - 1);

  for (int i = 0; i < semaphores.capacity; ++i) {
    semaphores[i].init(graphicsContext.device());
  }

  for (int i = 0; i < commandBuffers.size; ++i) {
    auto &commandBuffer = commandBuffers[i];

    Array<VulkanSemaphore, AllocationType::Linear> wait = {};
    Array<VulkanSemaphore, AllocationType::Linear> signal = {};

    if (i > 0) {
      wait = makeArray<VulkanSemaphore, AllocationType::Linear>(
        semaphores[i - 1]);
    }

    if (i < commandBuffers.size - 1) {
      signal = makeArray<VulkanSemaphore, AllocationType::Linear>(
        semaphores[i]);
    }

    queue.submitCommandBuffer(
      commandBuffer, wait, signal,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VulkanFence());
  }
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

  commandBuffer.transitionImageLayout(
    mPrecomputedIrradiance,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  commandBuffer.transitionImageLayout(
    mDeltaIrradianceTexture,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

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

void RendererSky::precomputeIndirectIrradiance(
  VulkanCommandBuffer &commandBuffer,
  int scatteringOrder) {
  VkExtent2D extent = {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT};

  commandBuffer.transitionImageLayout(
    mPrecomputedIrradiance,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  commandBuffer.transitionImageLayout(
    mDeltaIrradianceTexture,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  commandBuffer.beginRenderPass(
    mPrecomputeDirectIrradianceRenderPass,
    mPrecomputeDirectIrradianceFBO, {}, extent);

  commandBuffer.bindPipeline(mPrecomputeIndirectIrradiancePipeline);
  commandBuffer.bindUniforms(
    mSkyPropertiesUniform,
    mDeltaRayleighScatteringUniform,
    mDeltaMieScatteringUniform,
    mDeltaMultipleScatteringUniform);

  PrecomputePushConstant pushConstant = {};
  pushConstant.layer = 0;
  pushConstant.scatteringOrder = scatteringOrder - 1;

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  commandBuffer.setViewport(extent);
  commandBuffer.setScissor({}, extent);

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

void RendererSky::precomputeScatteringDensity(
  VulkanCommandBuffer &commandBuffer,
  uint32_t splitIndex, int scatteringOrder,
  uint32_t startLayer, uint32_t endLayer) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeScatteringDensity.renderPass[splitIndex],
    mPrecomputeScatteringDensity.fbo[splitIndex], {}, extent);

  PrecomputePushConstant pushConstant = {};

  for (int layer = startLayer; layer < endLayer; ++layer) {
    pushConstant.layer = layer;
    pushConstant.scatteringOrder = scatteringOrder;

    commandBuffer.bindPipeline(
      mPrecomputeScatteringDensity.pipeline[splitIndex]);

    commandBuffer.bindUniforms(
      mSkyPropertiesUniform,
      mPrecomputedTransmittanceUniform,
      mDeltaRayleighScatteringUniform,
      mDeltaMieScatteringUniform,
      mDeltaMultipleScatteringUniform,
      mDeltaIrradianceUniform);

    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

    commandBuffer.setViewport(extent);
    commandBuffer.setScissor({}, extent);

    commandBuffer.draw(4, 1, 0, 0);
  }

  commandBuffer.endRenderPass();
}

void RendererSky::precomputeMultipleScattering(
  VulkanCommandBuffer &commandBuffer,
  uint32_t splitIndex, int scatteringOrder,
  uint32_t startLayer, uint32_t endLayer) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  commandBuffer.transitionImageLayout(
    mDeltaMultipleScatteringTexture,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  commandBuffer.transitionImageLayout(
    mPrecomputedScattering,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  commandBuffer.beginRenderPass(
    mPrecomputeMultipleScattering.renderPass[splitIndex],
    mPrecomputeMultipleScattering.fbo[splitIndex], {}, extent);

  PrecomputePushConstant pushConstant = {};

  for (int layer = startLayer; layer < endLayer; ++layer) {
    pushConstant.layer = layer;
    pushConstant.scatteringOrder = scatteringOrder;

    commandBuffer.bindPipeline(
      mPrecomputeMultipleScattering.pipeline[splitIndex]);

    commandBuffer.bindUniforms(
      mSkyPropertiesUniform,
      mPrecomputedTransmittanceUniform,
      mDeltaScatteringDensityUniform);

    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

    commandBuffer.setViewport(extent);
    commandBuffer.setScissor({}, extent);

    commandBuffer.draw(4, 1, 0, 0);
  }

  commandBuffer.endRenderPass();

  /*
  commandBuffer.transitionImageLayout(
    mDeltaMultipleScatteringTexture,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  commandBuffer.transitionImageLayout(
    mPrecomputedScattering,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  */
}

void RendererSky::make3DTextureAndUniform(
  const VkExtent3D extent,
  VulkanTexture &texture,
  VulkanUniform &uniform,
  VulkanContext &graphicsContext,
  bool isTemporary) {
  TextureTypeBits textureType = TextureType::T3D | TextureType::Attachment;
  if (isTemporary) {
    // textureType |= TextureType::StoreInRam;
  }

  texture.init(
    graphicsContext.device(), textureType,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT16, VK_FILTER_LINEAR,
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
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT32, VK_FILTER_LINEAR,
    extent, 1, 1);

  uniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture));
}

void RendererSky::initDummyPipeline(
  const Buffer &vsha,
  VulkanContext &graphicsContext) {
  File precomputeDummyVsh = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_demo.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer precomputeVsh = precomputeDummyVsh.readBinary();

  File precomputeDummy = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_demo.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer fsh = precomputeDummy.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {graphicsContext.finalRenderPass(), 0},
    VulkanShader(
      graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), fsh, VulkanShaderType::Fragment));

  VulkanPipelineDescriptorLayout textureUL =
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

  pipelineConfig.configurePipelineLayout(
    sizeof(DemoPushConstant),
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    textureUL, textureUL, textureUL, textureUL);

  mDemo.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

}
