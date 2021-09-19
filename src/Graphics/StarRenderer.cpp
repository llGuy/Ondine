#include "Model.hpp"
#include "Camera.hpp"
#include "StarRenderer.hpp"
#include <glm/gtc/random.hpp>
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

const char *const StarRenderer::STARS_VERT_SPV = "res/spv/Stars.vert.spv";
const char *const StarRenderer::STARS_FRAG_SPV = "res/spv/Stars.frag.spv";

constexpr uint32_t STAR_COUNT = 60;

StarRenderer::StarRenderer()
  : mPipelineModelConfig(STAR_COUNT) {
  
}

void StarRenderer::init(
  VulkanContext &graphicsContext,
  const GBuffer &gbuffer,
  uint32_t starCount) {
  mStarCount = STAR_COUNT;
  generateStars();

  mGBuffer = &gbuffer;

  { // Set tracked resource
    addTrackedPath(STARS_VERT_SPV, &mPipeline);
    addTrackedPath(STARS_FRAG_SPV, &mPipeline);
  }

  { // Create "model" which will enable us to render these stars and pipeline
    // Each instance will juse be a point
    mPipelineModelConfig.pushAttribute(
      {sizeof(Star), VK_FORMAT_R32G32B32_SFLOAT},
      {(uint8_t *)mStars.data, mStars.size * sizeof(Star)});

    mStarsModel.init(mPipelineModelConfig, graphicsContext);

    mPipeline.init(
      [] (
        VulkanPipeline &res,
        StarRenderer &owner,
        VulkanContext &graphicsContext) {
        Core::File vshFile = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          STARS_VERT_SPV,
          Core::FileOpenType::Binary | Core::FileOpenType::In);

        Core::File fshFile = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          STARS_FRAG_SPV,
          Core::FileOpenType::Binary | Core::FileOpenType::In);

        Buffer vsh = vshFile.readBinary();
        Buffer fsh = fshFile.readBinary();

        VulkanPipelineConfig pipelineConfig(
          {owner.mGBuffer->renderPass(), 0},
          VulkanShader{graphicsContext.device(), vsh, VulkanShaderType::Vertex},
          VulkanShader{graphicsContext.device(), fsh, VulkanShaderType::Fragment});

        pipelineConfig.enableDepthTesting();
        pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipelineConfig.configurePipelineLayout(
          // Rotation
          sizeof(PushConstant),
          // Camera UBO
          VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

        owner.mPipelineModelConfig.configureVertexInput(pipelineConfig);
      
        owner.mPipeline.res.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      },
      *this,
      graphicsContext);
  }
}

void StarRenderer::render(
  float starSize, const Camera &camera, VulkanFrame &frame) const {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(camera.uniform());

  mStarsModel.bindVertexBuffers(commandBuffer);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  PushConstant pushConstant = mPushConstant;

  pushConstant.starSize = starSize;
  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  mStarsModel.submitForRender(commandBuffer);
}

void StarRenderer::tick(
  const CameraProperties &camera,
  const LightingProperties &lighting,
  const PlanetProperties &planet,
  const Core::Tick &tick) {
  mPushConstant.transform = glm::rotate(
    lighting.rotationAngle * 1.1f,
    glm::vec3(1.0f, 0.0f, 0.0f));

  float muSun = glm::dot(
    glm::vec3(0.0f, 1.0f, 0.0f),
    lighting.data.sunDirection);

  const float FADE_START = 0.015f;
  float fadeAmount = (muSun - planet.muSunMin) / (FADE_START - planet.muSunMin);
  fadeAmount = glm::clamp(fadeAmount, 0.0f, 1.0f);

  mPushConstant.fade = 1.0f - fadeAmount;
  mPushConstant.fade = glm::pow(mPushConstant.fade, 5.0f);

  mPushConstant.fade *= 22.5f;
}

void StarRenderer::generateStars() {
  mStars.init(mStarCount);

  for (int i = 0; i < mStarCount; ++i) {
    mStars[i].azimuthAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].zenithAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].brightness = glm::linearRand(0.001f, 1.0f);
  }

  mStars.size = mStarCount;
}

}
