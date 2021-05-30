#include "Application.hpp"
#include "FileSystem.hpp"
#include "PlanetRenderer.hpp"

namespace Yona {

void PlanetRenderer::init(
  VulkanContext &graphicsContext,
  const RenderStage &renderStage) {
  File precomputeDummyVsh = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/planet.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer precomputeVsh = precomputeDummyVsh.readBinary();

  File precomputeDummy = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/planet.frag.spv",
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
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

  mPipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

}
