#include "FileSystem.hpp"
#include "Application.hpp"
#include "WaterRenderer.hpp"
#include "RendererDebug.hpp"
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

void LightingProperties::fastForwardTo(FastForwardDst dst) {
  switch (dst) {
  case FastForwardDst::Sunset: {
    dstAngle = glm::radians(87.0f);
  } break;

  case FastForwardDst::Midday: {
    dstAngle = 0.0f;
  } break;

  case FastForwardDst::Midnight: {
    dstAngle = glm::radians(180.0f);
  } break;

  case FastForwardDst::Sunrise: {
    dstAngle = glm::radians(270.0f);
  } break;

  case FastForwardDst::BeautifulMoment: {
    dstAngle = glm::radians(93.0f);
  } break;
  }

  fastForwardTime = 0.0f;
  isFastForwarding = true;

  if (rotationAngle > dstAngle) {
    rotationAngle -= glm::radians(360.0f);
  }

  diff = dstAngle - rotationAngle;
  srcAngle = rotationAngle;
}

void LightingProperties::tick(
  const Core::Tick &tick,
  const PlanetProperties &planet) {
  data.dt = tick.dt;
  data.time = tick.accumulatedTime;
  data.continuous += tick.dt * 0.1f;
  if (data.continuous > 10.0f) {
    data.continuous -= 10.0f;
  }

  float rotationDiff = glm::radians(1.0f * tick.dt * 0.1f);

  if (isFastForwarding) {
    fastForwardTime += tick.dt;
    float scaledTime = fastForwardTime * 0.1f;

    rotationDiff = 
      diff * (6.0f * scaledTime - 6.0f * scaledTime * scaledTime);

    rotationAngle += rotationDiff * tick.dt;

    if (rotationAngle > dstAngle - 0.0001f) {
      isFastForwarding = false;
    }
  }
  else if (!pause) {
    rotationAngle += rotationDiff;
  }

  if (rotationAngle > glm::radians(360.0f)) {
    rotationAngle -= glm::radians(360.0f);
  }

  glm::mat3 rotation = glm::mat3(
    glm::rotate(
      rotationAngle,
      glm::vec3(1.0f, 0.0f, 0.0f)));

  data.sunDirection = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

  float muSun = glm::dot(
    glm::vec3(0.0f, 1.0f, 0.0f),
    data.sunDirection);

  const float FADE_START = 0.033f;
  float fadeAmount = (muSun - planet.muSunMin) /
    (FADE_START - planet.muSunMin);
  fadeAmount = glm::clamp(fadeAmount, 0.0f, 1.0f);

  data.moonStrength = 1.0f - fadeAmount;
  data.moonStrength = glm::pow(
    data.moonStrength, 1.0f) * 0.005f;

  // We want the light of the moon to appear later
  const float LIGHTING_FADE_START = 0.0f;
  float lightingFadeAmount = (muSun - planet.muSunMin) /
    (LIGHTING_FADE_START - planet.muSunMin);
  lightingFadeAmount = glm::clamp(lightingFadeAmount, 0.0f, 1.0f);

  data.moonLightingStrength = 1.0f - lightingFadeAmount;
  data.moonLightingStrength = glm::pow(
    data.moonLightingStrength, 1.0f) * 0.005f;
}

void LightingProperties::rotateBy(float radians) {
  rotationAngle += radians;

  glm::mat3 rotation = glm::mat3(
    glm::rotate(
      rotationAngle,
      glm::vec3(1.0f, 0.0f, 0.0f)));

  data.sunDirection = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

const char *const DeferredLighting::LIGHTING_FRAG_SPV =
  "res/spv/Lighting.frag.spv";
const char *const DeferredLighting::LIGHTING_REFL_FRAG_SPV =
  "res/spv/LightingRefl.frag.spv";

VulkanTexture *DeferredLighting::sWaterNormalMapTexture = nullptr;
VulkanTexture *DeferredLighting::sWaterDistortionTexture = nullptr;
VulkanUniform *DeferredLighting::sWaterUniform = nullptr;

VulkanTexture *DeferredLighting::sBRDFLutTexture = nullptr;
VulkanUniform *DeferredLighting::sBRDFLutUniform = nullptr;

void DeferredLighting::init(
  VulkanContext &graphicsContext,
  VkExtent2D initialExtent,
  const LightingProperties *properties) {
  { // Load water normal map
    if (!sWaterNormalMapTexture) {
      sWaterNormalMapTexture = flAlloc<VulkanTexture>();
      sWaterNormalMapTexture->initFromFile(
        graphicsContext.device(), graphicsContext.commandPool(),
        "res/textures/WaterNormalMap.png",
        TextureType::T2D | TextureType::WrapSampling, TextureContents::Color,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, 1);

      sWaterDistortionTexture = flAlloc<VulkanTexture>();

      sWaterDistortionTexture->initFromFile(
        graphicsContext.device(), graphicsContext.commandPool(),
        "res/textures/WaterNormalMap.jpeg",
        TextureType::T2D | TextureType::WrapSampling, TextureContents::Color,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, 1);

      sWaterUniform = flAlloc<VulkanUniform>();

      sWaterUniform->init(
        graphicsContext.device(),
        graphicsContext.descriptorPool(),
        graphicsContext.descriptorLayouts(),
        makeArray<VulkanTexture, AllocationType::Linear>(
          *sWaterNormalMapTexture, *sWaterDistortionTexture));
    }
  }

  if (!sBRDFLutTexture) {
    sBRDFLutTexture = flAlloc<VulkanTexture>();
    sBRDFLutUniform = flAlloc<VulkanUniform>();
    precomputeBRDFLut(graphicsContext);
  }

  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      LIGHTING_TEXTURE_FORMAT);

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
        VulkanPipelineConfig pipelineConfig(
          {owner.mLightingRenderPass, 0},
          VulkanShader(graphicsContext.device(), "res/spv/Lighting.vert.spv"),
          VulkanShader(graphicsContext.device(), LIGHTING_FRAG_SPV));

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
        VulkanPipelineConfig pipelineConfig(
          {owner.mLightingRenderPass, 0},
          VulkanShader(graphicsContext.device(), "res/spv/Lighting.vert.spv"),
          VulkanShader(graphicsContext.device(), LIGHTING_REFL_FRAG_SPV));

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
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
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
      sizeof(LightingProperties::data),
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
        {(uint8_t *)&properties->data, sizeof(LightingProperties::data)});
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

  commandBuffer.dbgBeginRegion(
    "LightingStage", DBG_DEFERRED_LIGHTING_COLOR);

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

  commandBuffer.dbgEndRegion();
}

void DeferredLighting::render(
  VulkanFrame &frame, const GBuffer &gbuffer,
  const Camera &camera, const PlanetRenderer &planet,
  const WaterRenderer &water,
  const SkyRenderer &sky) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.dbgBeginRegion(
    "LightingReflStage", DBG_DEFERRED_LIGHTING_COLOR);

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
    water.uniform(),
    *sWaterUniform,
    *sBRDFLutUniform);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();

  commandBuffer.dbgEndRegion();
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
    TextureContents::Color, LIGHTING_TEXTURE_FORMAT, VK_FILTER_LINEAR,
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

void DeferredLighting::precomputeBRDFLut(VulkanContext &graphicsContext) {
  VulkanRenderPass precomputeBRDFRenderPass;
  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      VK_FORMAT_R32G32_SFLOAT);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    precomputeBRDFRenderPass.init(
      graphicsContext.device(), renderPassConfig);
  }

  Resolution resolution = {512, 512};
  VulkanFramebuffer precomputeBRDFFBO;
  { // Create framebuffer and attachments
    sBRDFLutTexture->init(
      graphicsContext.device(),
      TextureType::Attachment | TextureType::T2D,
      TextureContents::Color,
      VK_FORMAT_R32G32_SFLOAT,
      VK_FILTER_LINEAR,
      {resolution.width, resolution.height, 1},
      1, 1);

    sBRDFLutUniform->init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(*sBRDFLutTexture));

    VulkanFramebufferConfig fboConfig(1, precomputeBRDFRenderPass);
    fboConfig.addAttachment(*sBRDFLutTexture);
    precomputeBRDFFBO.init(graphicsContext.device(), fboConfig);
  }

  VulkanPipeline precomputeBRDFPipeline;
  { // Create precompute graphics pipeline
    Core::File precomputeVsh = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/TexturedQuad.vert.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);
    Buffer vsh = precomputeVsh.readBinary();

    Core::File precomputeFsh = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/BRDFLut.frag.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);
    Buffer fsh = precomputeFsh.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {precomputeBRDFRenderPass, 0},
      VulkanShader(graphicsContext.device(), vsh, VulkanShaderType::Vertex),
      VulkanShader(graphicsContext.device(), fsh, VulkanShaderType::Fragment));
    pipelineConfig.configurePipelineLayout(0);

    precomputeBRDFPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Precompute
    VulkanCommandBuffer commandBuffer =
      graphicsContext.commandPool().makeCommandBuffer(
        graphicsContext.device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

    commandBuffer.beginRenderPass(
      precomputeBRDFRenderPass,
      precomputeBRDFFBO,
      {}, {resolution.width, resolution.height});

    commandBuffer.bindPipeline(precomputeBRDFPipeline);
    commandBuffer.setViewport();
    commandBuffer.setScissor();
    commandBuffer.draw(4, 1, 0, 0);
    commandBuffer.endRenderPass();

    commandBuffer.end();

    graphicsContext.device().graphicsQueue().submitCommandBuffer(
      commandBuffer,
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      makeArray<VulkanSemaphore, AllocationType::Linear>(),
      0, VulkanFence());

    graphicsContext.device().idle();
  }
}

}
