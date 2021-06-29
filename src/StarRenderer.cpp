#include "Model.hpp"
#include "Camera.hpp"
#include "Application.hpp"
#include "StarRenderer.hpp"
#include <glm/gtc/random.hpp>

namespace Ondine::Graphics {

const char *const StarRenderer::STARS_VERT_SPV = "res/spv/Stars.vert.spv";
const char *const StarRenderer::STARS_FRAG_SPV = "res/spv/Stars.frag.spv";

constexpr uint32_t STAR_COUNT = 200;

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
      {sizeof(Star), VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX},
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

void StarRenderer::render(const Camera &camera, VulkanFrame &frame) const {
  auto &commandBuffer = frame.primaryCommandBuffer;
  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(camera.uniform());

  mStarsModel.bindVertexBuffers(commandBuffer);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.pushConstants(sizeof(mPushConstant), &mPushConstant);

  mStarsModel.submitForRender(commandBuffer);
}

void StarRenderer::tick(
  const CameraProperties &camera,
  const LightingProperties &lighting,
  const PlanetProperties &planet,
  const Core::Tick &tick) {
  mPushConstant.transform = glm::rotate(
    glm::radians(-mCurrentRotation),
    glm::vec3(1.0f, 0.0f, 0.0f));

  float muSun = glm::dot(
    glm::vec3(0.0f, 1.0f, 0.0f),
    lighting.sunDirection);

  const float FADE_START = 0.03f;// -0.001f;
  float fadeAmount = (muSun - planet.muSunMin) / (FADE_START - planet.muSunMin);
  fadeAmount = glm::clamp(fadeAmount, 0.0f, 1.0f);

  mPushConstant.fade = 1.0f - fadeAmount;
  mPushConstant.fade = glm::pow(mPushConstant.fade, 2.0f);
  LOG_INFOV("%f\n", mPushConstant.fade);

  mCurrentRotation += tick.dt;

  if (mCurrentRotation > 360.0f) {
    mCurrentRotation -= 360.0f;
  }
}

void StarRenderer::generateStars() {
  mStars.init(mStarCount);

  for (int i = 0; i < mStarCount; ++i) {
    mStars[i].azimuthAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].zenithAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].brightness = glm::linearRand(0.1f, 1.0f);
  }

  mStars.size = mStarCount;
}

}
