#include "IO.hpp"
#include "Camera.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "VulkanFrame.hpp"
#include "PlanetRenderer.hpp"

namespace Ondine::Graphics {

void PlanetRenderer::init(
  VulkanContext &graphicsContext,
  const RenderStage &renderStage,
  const PlanetProperties *properties) {
  { // Create pipeline
    Core::File precomputeDummyVsh = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/Planet.vert.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);

    Buffer precomputeVsh = precomputeDummyVsh.readBinary();

    Core::File precomputeDummy = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/Planet.frag.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);

    Buffer fsh = precomputeDummy.readBinary();

    VulkanPipelineConfig pipelineConfig(
      {renderStage.renderPass(), 0},
      VulkanShader(
        graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
      VulkanShader(
        graphicsContext.device(), fsh, VulkanShaderType::Fragment));

    VulkanPipelineDescriptorLayout textureUL =
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

    pipelineConfig.configurePipelineLayout(
      0,
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    pipelineConfig.enableDepthTesting();

    mPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }

  { // Create uniform buffer
    mPlanetPropertiesBuffer.init(
      graphicsContext.device(),
      sizeof(PlanetProperties),
      (int)VulkanBufferFlag::UniformBuffer);

    mPlanetPropertiesUniform.init(
      graphicsContext.device(),
      graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanBuffer, AllocationType::Linear>(mPlanetPropertiesBuffer));

    if (properties) {
      mPlanetPropertiesBuffer.fillWithStaging(
        graphicsContext.device(),
        graphicsContext.commandPool(),
        {(uint8_t *)properties, sizeof(PlanetProperties)});
    }
  }
}

void PlanetRenderer::tick(
  const Core::Tick &tick,
  VulkanFrame &frame,
  const Camera &camera) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline);
  commandBuffer.bindUniforms(camera.uniform(), mPlanetPropertiesUniform);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);
}

void PlanetRenderer::updateData(
  const VulkanCommandBuffer &commandBuffer,
  const PlanetProperties &properties) {
  commandBuffer.updateBuffer(
    mPlanetPropertiesBuffer, 0, sizeof(properties), &properties);
}

const VulkanUniform &PlanetRenderer::uniform() const {
  return mPlanetPropertiesUniform;
}

}
