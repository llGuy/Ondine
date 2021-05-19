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

void RendererSky::tick(const VulkanFrame &frame) {
  // precomputeTransmittance(frame.primaryCommandBuffer);
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

void RendererSky::preparePrecompute(VulkanContext &graphicsContext) {
  File precomputeVshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_precompute.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer quadVsh = precomputeVshFile.readBinary();

  prepareTransmittancePrecompute(quadVsh, graphicsContext);
}

void RendererSky::prepareTransmittancePrecompute(
  const Buffer &precomputeVsh,
  VulkanContext &graphicsContext) {
  VulkanRenderPassConfig renderPassConfig(1, 1);

  renderPassConfig.addAttachment(
    LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
    OutputUsage::FragmentShaderRead, AttachmentType::Color,
    VK_FORMAT_R32G32B32A32_SFLOAT);

  renderPassConfig.addSubpass(
    makeArray<uint32_t, AllocationType::Linear>(0U),
    makeArray<uint32_t, AllocationType::Linear>(),
    false);

  mPrecomputeTransmittanceRenderPass.init(
    graphicsContext.device(), renderPassConfig);

  File precomputeTransmittance = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_transmittance.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer transmittance = precomputeTransmittance.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {mPrecomputeTransmittanceRenderPass, 0},
    VulkanShader(
      graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), transmittance, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0, VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

  mPrecomputeTransmittancePipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);

  mPrecomputedTransmittance.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FILTER_LINEAR,
    {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, 1}, 1, 1);

  VulkanFramebufferConfig fboConfig(1, mPrecomputeTransmittanceRenderPass);
  fboConfig.addAttachment(mPrecomputedTransmittance);

  mPrecomputedTransmittanceFBO.init(graphicsContext.device(), fboConfig);
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
  const VulkanCommandBuffer &commandBuffer) {
  VkExtent2D extent = {TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT};

  commandBuffer.beginRenderPass(
    mPrecomputeTransmittanceRenderPass,
    mPrecomputedTransmittanceFBO,
    {}, extent);

  commandBuffer.bindPipeline(mPrecomputeTransmittancePipeline);
  commandBuffer.bindUniforms(
    mPrecomputeTransmittancePipeline, mSkyPropertiesUniform);

  commandBuffer.setViewport(extent);
  commandBuffer.setScissor({}, extent);

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();
}

}
