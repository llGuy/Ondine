#include "IO.hpp"
#include "Camera.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "VulkanFrame.hpp"
#include "PlanetRenderer.hpp"

namespace Yona {

void PlanetRenderer::init(
  VulkanContext &graphicsContext,
  const RenderStage &renderStage) {
  { // Create pipeline
    File precomputeDummyVsh = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/Planet.vert.spv",
      FileOpenType::Binary | FileOpenType::In);

    Buffer precomputeVsh = precomputeDummyVsh.readBinary();

    File precomputeDummy = gFileSystem->createFile(
      (MountPoint)ApplicationMountPoints::Application,
      "res/spv/Planet.frag.spv",
      FileOpenType::Binary | FileOpenType::In);

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
  }
}

void PlanetRenderer::tick(
  const Tick &tick,
  VulkanFrame &frame,
  const Camera &camera) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline);

  commandBuffer.bindUniforms(
    camera.uniform(),
    mPlanetPropertiesUniform);

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

}
