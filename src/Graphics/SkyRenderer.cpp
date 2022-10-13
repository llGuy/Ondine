#include <cstdio>
#include <math.h>
#include "FileSystem.hpp"
#include "SkyRenderer.hpp"
#include "VulkanContext.hpp"
#include "RendererCache.hpp"
#include "VulkanTexture.hpp"

namespace Ondine::Graphics {

void SkyRenderer::init(
  VulkanContext &graphicsContext,
  const RenderStage &renderStage) {
  mCache = false;

  initSkyProperties(graphicsContext);

  initFinalTextures(graphicsContext);

  if (isPrecomputationNeeded(graphicsContext)) {
    LOG_INFO("Didn't find cached sky precomputations or using GPU\n");
    LOG_INFO("Precomputing sky textures\n");

    preparePrecompute(graphicsContext);
    precompute(graphicsContext);
  }
  else {
    LOG_INFO("Found cached sky precomputations\n");
    LOG_INFO("Loading precomputations from disk\n");

    loadFromCache(graphicsContext);
  }

  mViewDistanceMeters = 300.000000;
  mViewZenithAngleRadians = 1.470000;
  mViewAzimuthAngleRadians = 0.000000;
  mSunZenithAngleRadians = 1.300000;
  mSunAzimuthAngleRadians = 3.000000;
  mExposure = 10.000000;
}

void SkyRenderer::shutdown(VulkanContext &graphicsContext) {
  if (!Core::gFileSystem->isPathValid(
        (Core::MountPoint)Core::ApplicationMountPoints::Application,
        SKY_CACHE_DIRECTORY)) {
    Core::gFileSystem->makeDirectory(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      SKY_CACHE_DIRECTORY);
  }

  if (mCache) {
    LOG_INFO("Did sky precomputations during session - saving into cache\n");

    if (graphicsContext.device().deviceType() != DeviceType::DiscreteGPU)
      saveToCache(graphicsContext);
  }
}

void SkyRenderer::initSkyProperties(VulkanContext &graphicsContext) {
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
  mSkyProperties.bottomRadius = 6360.0f / 2.0f;
  mSkyProperties.topRadius = 6420.0f / 2.0f;

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
    sizeof(PlanetProperties),
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

void SkyRenderer::initFinalTextures(VulkanContext &graphicsContext) {
  make2DTextureAndUniform(
    {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, 1},
    mPrecomputedTransmittance, mPrecomputedTransmittanceUniform,
    graphicsContext);

  VkExtent3D extent3D = {
    SCATTERING_TEXTURE_WIDTH,
    SCATTERING_TEXTURE_HEIGHT,
    SCATTERING_TEXTURE_DEPTH
  };

  make3DTextureAndUniform(
    extent3D, mDeltaMieScatteringTexture,
    mDeltaMieScatteringUniform, graphicsContext, true);

  VkExtent3D extent = {
    IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1
  };

  make2DTextureAndUniform(
    extent, mPrecomputedIrradiance, mPrecomputedIrradianceUniform,
    graphicsContext);

  make3DTextureAndUniform(
    extent3D, mPrecomputedScattering, mPrecomputedScatteringUniform,
    graphicsContext, false);

  mRenderingUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(
      mPrecomputedTransmittance, mPrecomputedScattering,
      mDeltaMieScatteringTexture, mPrecomputedIrradiance));
}

void SkyRenderer::initTemporaryPrecomputeTextures(
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
    extent3D, mDeltaScatteringDensityTexture,
    mDeltaScatteringDensityUniform, graphicsContext, true);

  make2DTextureAndUniform(
    {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1},
    mDeltaIrradianceTexture, mDeltaIrradianceUniform, graphicsContext);
}

void SkyRenderer::preparePrecompute(VulkanContext &graphicsContext) {
  initTemporaryPrecomputeTextures(graphicsContext);

  prepareTransmittancePrecompute(graphicsContext);
  prepareSingleScatteringPrecompute(graphicsContext);
  prepareDirectIrradiancePrecompute(graphicsContext);
  prepareScatteringDensityPrecompute(graphicsContext);
  prepareMultipleScatteringPrecompute(graphicsContext);
}

void SkyRenderer::prepareTransmittancePrecompute(VulkanContext &graphicsContext) {
  { // Create pipeline
    VulkanPipelineConfig pipelineConfig(
      {NullReference<VulkanRenderPass>::nullRef, 0},
      VulkanShader(graphicsContext.device(), "res/spv/SkyTransmittance.comp.spv"));

    pipelineConfig.configurePipelineLayout(
      0, 
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});

    mPrecomputeTransmittancePipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }
}

void SkyRenderer::prepareSingleScatteringPrecompute(VulkanContext &graphicsContext) {
  { // Create pipeline
    VulkanPipelineConfig pipelineConfig(
      {NullReference<VulkanRenderPass>::nullRef, 0},
      VulkanShader(graphicsContext.device(), "res/spv/SkySingleScattering.comp.spv"));

    VulkanPipelineDescriptorLayout storageLayout = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};

    pipelineConfig.configurePipelineLayout(
      sizeof(PrecomputePushConstant),
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
      storageLayout, storageLayout, storageLayout);

    mPrecomputeSingleScatteringPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }
}

void SkyRenderer::prepareDirectIrradiancePrecompute(VulkanContext &graphicsContext) {
  { // Create pipeline (direct irradiance)
    VulkanPipelineConfig pipelineConfig(
      {NullReference<VulkanRenderPass>::nullRef, 0},
      VulkanShader(graphicsContext.device(), "res/spv/SkyDirectIrradiance.comp.spv"));

    VulkanPipelineDescriptorLayout storageLayout = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};

    pipelineConfig.configurePipelineLayout(
      0,
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
      storageLayout, storageLayout);

    mPrecomputeDirectIrradiancePipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create pipeline (indirect irradiance)
    VulkanPipelineConfig pipelineConfig(
      {mPrecomputeDirectIrradianceRenderPass, 0},
      VulkanShader(graphicsContext.device(), "res/spv/SkyIndirectIrradiance.comp.spv"));

    VulkanPipelineDescriptorLayout textureUL = {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1
    };

    VulkanPipelineDescriptorLayout storageUL = {
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1
    };

    pipelineConfig.configurePipelineLayout(
      sizeof(PrecomputePushConstant),
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      storageUL, storageUL, textureUL, textureUL, textureUL);

    mPrecomputeIndirectIrradiancePipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }
}

void SkyRenderer::prepareScatteringDensityPrecompute(VulkanContext &graphicsContext) {

  VulkanPipelineConfig pipelineConfig(
      {NullReference<VulkanRenderPass>::nullRef, 0},
      VulkanShader(graphicsContext.device(), "res/spv/SkyScatteringDensity.comp.spv"));

  VulkanPipelineDescriptorLayout textureUL =
  {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

  pipelineConfig.configurePipelineLayout(
      sizeof(PrecomputeSplitPushConstant),
      VulkanPipelineDescriptorLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
      textureUL, textureUL, textureUL, textureUL, textureUL);

  mPrecomputeScatteringDensityPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
}

void SkyRenderer::prepareMultipleScatteringPrecompute(VulkanContext &graphicsContext) {
  VulkanPipelineConfig pipelineConfig(
    {NullReference<VulkanRenderPass>::nullRef, 0},
    VulkanShader(graphicsContext.device(), "res/spv/SkyMultipleScattering.comp.spv"));

  VulkanPipelineDescriptorLayout textureUL =
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

  VulkanPipelineDescriptorLayout storageUL =
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};

  pipelineConfig.configurePipelineLayout(
    sizeof(PrecomputeSplitPushConstant),
    VulkanPipelineDescriptorLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    storageUL, storageUL, textureUL, textureUL);

  mPrecomputeMultipleScatteringPipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void SkyRenderer::precompute(VulkanContext &graphicsContext) {
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

  mCache = true;
}

void SkyRenderer::precomputeTransmittance(
  VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT};

  commandBuffer.bindPipeline(
      mPrecomputeTransmittancePipeline, VulkanPipelineBindPoint::Compute);
  commandBuffer.bindUniformsCompute(
      mSkyPropertiesUniform, mPrecomputedTransmittanceUniform.storage);

  prepareImageLayoutForCompute(commandBuffer, mPrecomputedTransmittance);

  commandBuffer.dispatch(glm::ivec3(extent.width/16, extent.height/16, 1));

  prepareImageLayoutForRead(commandBuffer, mPrecomputedTransmittance);
}

void SkyRenderer::precomputeSingleScattering(
  VulkanCommandBuffer &commandBuffer) {

  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};
  PrecomputePushConstant pushConstant = {};

  commandBuffer.bindPipeline(
      mPrecomputeSingleScatteringPipeline,
      VulkanPipelineBindPoint::Compute);
  commandBuffer.bindUniformsCompute(
    mSkyPropertiesUniform, mPrecomputedTransmittanceUniform.sampler,
    mDeltaRayleighScatteringUniform.storage, mDeltaMieScatteringUniform.storage,
    mPrecomputedScatteringUniform.storage);

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  prepareImageLayoutForCompute(commandBuffer, mPrecomputedScattering);
  prepareImageLayoutForCompute(commandBuffer, mDeltaRayleighScatteringTexture);
  prepareImageLayoutForCompute(commandBuffer, mDeltaMieScatteringTexture);

  commandBuffer.dispatch(glm::ivec3(
        extent.width / 16, extent.height/16,
        SCATTERING_TEXTURE_DEPTH));

  prepareImageLayoutForRead(commandBuffer, mPrecomputedScattering);
  prepareImageLayoutForRead(commandBuffer, mDeltaRayleighScatteringTexture);
  prepareImageLayoutForRead(commandBuffer, mDeltaMieScatteringTexture);
}

void SkyRenderer::precomputeDirectIrradiance(
  VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT};

  commandBuffer.bindPipeline(
      mPrecomputeDirectIrradiancePipeline, VulkanPipelineBindPoint::Compute);
  commandBuffer.bindUniformsCompute(
    mSkyPropertiesUniform, mPrecomputedTransmittanceUniform.sampler,
    mDeltaIrradianceUniform.storage, mPrecomputedIrradianceUniform.storage);

  prepareImageLayoutForCompute(commandBuffer, mDeltaIrradianceTexture);
  prepareImageLayoutForCompute(commandBuffer, mPrecomputedIrradiance);

  commandBuffer.dispatch(glm::ivec3(
        extent.width/16, extent.height/16, 1));

  prepareImageLayoutForRead(commandBuffer, mDeltaIrradianceTexture);
  prepareImageLayoutForRead(commandBuffer, mPrecomputedIrradiance);
}

void SkyRenderer::precomputeIndirectIrradiance(
  VulkanCommandBuffer &commandBuffer,
  int scatteringOrder) {
  VkExtent2D extent = {IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT};

  commandBuffer.bindPipeline(
    mPrecomputeIndirectIrradiancePipeline, VulkanPipelineBindPoint::Compute);

  commandBuffer.bindUniformsCompute(
    mSkyPropertiesUniform,
    mDeltaIrradianceUniform.storage,
    mPrecomputedIrradianceUniform.storage,
    mDeltaRayleighScatteringUniform.sampler,
    mDeltaMieScatteringUniform.sampler,
    mDeltaMultipleScatteringUniform.sampler);

  PrecomputePushConstant pushConstant = {};
  pushConstant.layer = 0;
  pushConstant.scatteringOrder = scatteringOrder - 1;

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  prepareImageLayoutForCompute(commandBuffer, mDeltaIrradianceTexture);
  prepareImageLayoutForCompute(commandBuffer, mPrecomputedIrradiance);

  commandBuffer.dispatch(glm::ivec3(
        extent.width/16, extent.height/16, 1));

  prepareImageLayoutForRead(commandBuffer, mDeltaIrradianceTexture);
  prepareImageLayoutForRead(commandBuffer, mPrecomputedIrradiance);
}

void SkyRenderer::precomputeScatteringDensity(
  VulkanCommandBuffer &commandBuffer,
  uint32_t splitIndex, int scatteringOrder,
  uint32_t startLayer, uint32_t endLayer) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  PrecomputeSplitPushConstant pushConstant = {};

  pushConstant.scatteringOrder = scatteringOrder;
  pushConstant.depthOffset = startLayer;

  commandBuffer.bindPipeline(
      mPrecomputeScatteringDensityPipeline, VulkanPipelineBindPoint::Compute);

  commandBuffer.bindUniformsCompute(
      mSkyPropertiesUniform,
      mDeltaScatteringDensityUniform.storage,
      mPrecomputedTransmittanceUniform.sampler,
      mDeltaRayleighScatteringUniform.sampler,
      mDeltaMieScatteringUniform.sampler,
      mDeltaMultipleScatteringUniform.sampler,
      mDeltaIrradianceUniform.sampler);

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  prepareImageLayoutForCompute(commandBuffer, mDeltaScatteringDensityTexture);

  commandBuffer.dispatch(glm::ivec3(
        extent.width/16, extent.height/16, endLayer-startLayer));

  prepareImageLayoutForRead(commandBuffer, mDeltaScatteringDensityTexture);
}

void SkyRenderer::precomputeMultipleScattering(
  VulkanCommandBuffer &commandBuffer,
  uint32_t splitIndex, int scatteringOrder,
  uint32_t startLayer, uint32_t endLayer) {
  VkExtent2D extent = {SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT};

  PrecomputeSplitPushConstant pushConstant = {};

  pushConstant.scatteringOrder = scatteringOrder;
  pushConstant.depthOffset = startLayer;

  commandBuffer.bindPipeline(
      mPrecomputeMultipleScatteringPipeline,
      VulkanPipelineBindPoint::Compute);

  commandBuffer.bindUniformsCompute(
      mSkyPropertiesUniform,
      mDeltaMultipleScatteringUniform.storage,
      mPrecomputedScatteringUniform.storage,
      mPrecomputedTransmittanceUniform.sampler,
      mDeltaScatteringDensityUniform.sampler);

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  prepareImageLayoutForCompute(commandBuffer, mDeltaMultipleScatteringTexture);
  prepareImageLayoutForCompute(commandBuffer, mPrecomputedScattering);

  commandBuffer.dispatch(glm::ivec3(
        extent.width/16, extent.height/16, endLayer-startLayer));

  prepareImageLayoutForRead(commandBuffer, mDeltaMultipleScatteringTexture);
  prepareImageLayoutForRead(commandBuffer, mPrecomputedScattering);
}

void SkyRenderer::make3DTextureAndUniform(
  const VkExtent3D extent,
  VulkanTexture &texture,
  PrecomputedUniform &uniform,
  VulkanContext &graphicsContext,
  bool isTemporary) {
  TextureTypeBits textureType =
    TextureType::T3D | TextureType::ComputeTarget | TextureType::TransferSource;
  if (isTemporary) {
    // textureType |= TextureType::StoreInRam;
  }

  texture.init(
    graphicsContext.device(), textureType,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT16, VK_FILTER_LINEAR,
    extent, 1, 1);

  uniform.storage.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture),
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

  uniform.sampler.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture));
}

void SkyRenderer::make2DTextureAndUniform(
  const VkExtent3D extent,
  VulkanTexture &texture,
  PrecomputedUniform &uniform,
  VulkanContext &graphicsContext) {
  texture.init(
    graphicsContext.device(),
    TextureType::T2D | TextureType::ComputeTarget | TextureType::TransferSource,
    TextureContents::Color, PRECOMPUTED_TEXTURE_FORMAT32, VK_FILTER_LINEAR,
    extent, 1, 1);

  uniform.storage.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture),
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

  uniform.sampler.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(texture));
}

void SkyRenderer::initDemoPipeline(
  VulkanContext &graphicsContext,
  const RenderStage &renderStage) {
  Core::File precomputeDummyVsh = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    "res/spv/SkyDemo.vert.spv",
    Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer precomputeVsh = precomputeDummyVsh.readBinary();

  Core::File precomputeDummy = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    "res/spv/SkyDemo.frag.spv",
    Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer fsh = precomputeDummy.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {renderStage.renderPass(), 0},
    VulkanShader(
      graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), fsh, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    sizeof(DemoPushConstant),
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4});

  mDemo.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

const VulkanUniform &SkyRenderer::uniform() const {
  return mRenderingUniform;
}

bool SkyRenderer::isPrecomputationNeeded(const VulkanContext &context) const {
  const char *const names[] = {
    SKY_TRANSMITTANCE_CACHE_FILENAME,
    SKY_SCATTERING_CACHE_FILENAME,
    SKY_MIE_SCATTERING_CACHE_FILENAME,
    SKY_IRRADIANCE_CACHE_FILENAME,
  };

  if (!Core::gFileSystem->isPathValid(
        (Core::MountPoint)(Core::ApplicationMountPoints::Application),
        SKY_CACHE_DIRECTORY)) {
    return true;
  }

  for (int i = 0; i < 4; ++i) {
    if (!Core::gFileSystem->isPathValid(
          (Core::MountPoint)(Core::ApplicationMountPoints::Application),
          std::string(SKY_CACHE_DIRECTORY) + names[i])) {
      return true;
    }
  }

  return (context.device().deviceType() == DeviceType::DiscreteGPU);
}

void SkyRenderer::loadFromCache(VulkanContext &graphicsContext) {
  const char *const names[4] = {
    SKY_TRANSMITTANCE_CACHE_FILENAME,
    SKY_SCATTERING_CACHE_FILENAME,
    SKY_MIE_SCATTERING_CACHE_FILENAME,
    SKY_IRRADIANCE_CACHE_FILENAME,
  };

  VulkanTexture *textures[4] = {
    &mPrecomputedTransmittance,
    &mPrecomputedScattering,
    &mDeltaMieScatteringTexture,
    &mPrecomputedIrradiance
  };

  VulkanCommandBuffer cmdbufs[4] = {};
  VulkanBuffer staging[4] = {};

  VkExtent3D scatteringExtent = {
    SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH
  };
  VkExtent3D irradianceExtent = {
    IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1
  };

  auto &commandPool = graphicsContext.commandPool();

  for (int i = 0; i < 4; ++i) {
    Core::File file = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      std::string(SKY_CACHE_DIRECTORY) + names[i],
      Core::FileOpenType::Binary | Core::FileOpenType::In);

    Buffer bin = file.readBinary();

    VulkanBuffer &buffer = staging[i];
    buffer.init(
      graphicsContext.device(), bin.size,
      VulkanBufferFlag::Mappable | VulkanBufferFlag::TransferSource);

    memcpy(
      buffer.map(graphicsContext.device(), bin.size, 0),
      bin.data,
      bin.size);

    buffer.unmap(graphicsContext.device());

    // Copy to image
    VulkanCommandBuffer &commandBuffer = cmdbufs[i];
    commandBuffer = commandPool.makeCommandBuffer(
      graphicsContext.device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

    commandBuffer.copyBufferToImage(
      *textures[i],
      0, 1, 0,
      buffer, 0, bin.size);

    commandBuffer.transitionImageLayout(
      *textures[i],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    commandBuffer.end();

    graphicsContext.device().graphicsQueue().submitCommandBuffer(
      commandBuffer,
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      0, VulkanFence());

    LOG_INFOV("Loaded %s\n", names[i]);
  }

  graphicsContext.device().idle();

  for (int i = 0; i < 4; ++i) {
    commandPool.freeCommandBuffer(graphicsContext.device(), cmdbufs[i]);
    staging[i].destroy(graphicsContext.device());
  }

  LOG_INFO("Finished loading sky cache\n");
}

void SkyRenderer::saveToCache(VulkanContext &graphicsContext) {
  const char *const names[4] = {
    SKY_TRANSMITTANCE_CACHE_FILENAME,
    SKY_SCATTERING_CACHE_FILENAME,
    SKY_MIE_SCATTERING_CACHE_FILENAME,
    SKY_IRRADIANCE_CACHE_FILENAME,
  };

  VulkanTexture *textures[4] = {
    &mPrecomputedTransmittance,
    &mPrecomputedScattering,
    &mDeltaMieScatteringTexture,
    &mPrecomputedIrradiance
  };

  VulkanCommandBuffer cmdbufs[4] = {};
  VulkanBuffer staging[4] = {};

  VkExtent3D scatteringExtent = {
    SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH
  };
  VkExtent3D irradianceExtent = {
    IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1
  };

  auto &commandPool = graphicsContext.commandPool();

  for (int i = 0; i < 4; ++i) {
    Core::File file = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      std::string(SKY_CACHE_DIRECTORY) + names[i],
      Core::FileOpenType::Binary | Core::FileOpenType::Out);

    size_t size = textures[i]->memoryRequirement();

    VulkanBuffer &buffer = staging[i];
    buffer.init(
      graphicsContext.device(), size,
      (VulkanBufferFlagBits)VulkanBufferFlag::Mappable);

    // Copy to buffer
    VulkanCommandBuffer &commandBuffer = cmdbufs[i];
    commandBuffer = commandPool.makeCommandBuffer(
      graphicsContext.device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

    commandBuffer.transitionImageLayout(
      *textures[i], 
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

    commandBuffer.copyImageToBuffer(
      buffer, 0, size,
      *textures[i],
      0, 1, 0);

    commandBuffer.end();

    graphicsContext.device().graphicsQueue().submitCommandBuffer(
      commandBuffer,
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      0, VulkanFence());

    graphicsContext.device().idle();

    void *data = buffer.map(graphicsContext.device(), size, 0);
    file.write(data, size);
    buffer.unmap(graphicsContext.device());
  }

  for (int i = 0; i < 4; ++i) {
    commandPool.freeCommandBuffer(graphicsContext.device(), cmdbufs[i]);
    staging[i].destroy(graphicsContext.device());
  }
}

void SkyRenderer::prepareImageLayoutForCompute(
  VulkanCommandBuffer &cmdbuf, const VulkanTexture &texture) {
  cmdbuf.transitionImageLayout(
    texture,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}
void SkyRenderer::prepareImageLayoutForRead(
  VulkanCommandBuffer &cmdbuf, const VulkanTexture &texture) {
  cmdbuf.transitionImageLayout(
    texture,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

}
