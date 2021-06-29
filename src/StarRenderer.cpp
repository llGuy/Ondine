#include "Model.hpp"
#include "Application.hpp"
#include "StarRenderer.hpp"
#include <glm/gtc/random.hpp>

namespace Ondine::Graphics {

const char *const StarRenderer::STARS_VERT_SPV = "res/spv/Stars.vert.spv";
const char *const StarRenderer::STARS_FRAG_SPV = "res/spv/Stars.frag.spv";

StarRenderer::StarRenderer()
  : mPipelineModelConfig(1) {
  
}

void StarRenderer::init(
  VulkanContext &graphicsContext,
  const GBuffer &gbuffer,
  uint32_t starCount) {
  mStarCount = starCount;
  generateStars();

  mGBuffer = &gbuffer;

  { // Set tracked resource
    addTrackedPath(STARS_VERT_SPV, &mPipeline);
    addTrackedPath(STARS_FRAG_SPV, &mPipeline);
  }

  { // Create "model" which will enable us to render these stars and pipeline
    // Each instance will juse be a point
    mPipelineModelConfig.pushAttribute(
      {sizeof(Star), VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_INSTANCE},
      {(uint8_t *)mStars.data, mStars.size * sizeof(Star)});

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
        pipelineConfig.configurePipelineLayout(
          // Rotation
          sizeof(glm::mat4),
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

void StarRenderer::generateStars() {
  for (int i = 0; i < mStarCount; ++i) {
    mStars[i].azimuthAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].zenithAngleRadians = glm::radians(glm::linearRand(0.0f, 360.0f));
    mStars[i].brightness = glm::linearRand(0.5f, 1.0f);
  }
}

}
