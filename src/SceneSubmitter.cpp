#include "FileSystem.hpp"
#include "Application.hpp"
#include "SceneSubmitter.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

SceneSubmitter::SceneSubmitter(ModelManager &modelManager)
  : mModelManager(modelManager) {
  
}

void SceneSubmitter::init(
  const GBuffer &gbuffer,
  VulkanContext &graphicsContext) {
  { // Create test model
    ModelConfig modelConfig;
    mTestModel = mModelManager.loadStaticModel(
      "res/model/Taurus.fbx", graphicsContext, modelConfig);

    Core::File vshFile = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/BaseModel.vert.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);

    Core::File fshFile = Core::gFileSystem->createFile(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      "res/spv/BaseModel.frag.spv",
      Core::FileOpenType::Binary | Core::FileOpenType::In);

    Buffer vsh = vshFile.readBinary();
    Buffer fsh = fshFile.readBinary();
    
    VulkanPipelineConfig pipelineConfig(
      {gbuffer.renderPass(), 0},
      VulkanShader{graphicsContext.device(), vsh, VulkanShaderType::Vertex},
      VulkanShader{graphicsContext.device(), fsh, VulkanShaderType::Fragment});

    pipelineConfig.enableDepthTesting();
    pipelineConfig.configurePipelineLayout(
      sizeof(testPushConstant),
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    modelConfig.configureVertexInput(pipelineConfig);
      
    mTestPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }
}

void SceneSubmitter::submit(
  const Camera &camera,
  VulkanFrame &frame) {
  // Render test model
  auto &commandBuffer = frame.primaryCommandBuffer;
  commandBuffer.bindPipeline(mTestPipeline);
  commandBuffer.bindUniforms(camera.uniform());

  testPushConstant.modelMatrix =
    glm::translate(glm::vec3(0.0f, 10.0f, 0.0f)) *
    glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 0.0f, 0.0f)) *
    glm::scale(glm::vec3(10.0f));
  commandBuffer.pushConstants(sizeof(testPushConstant), &testPushConstant);

  auto &model = mModelManager.getStaticModel(mTestModel);
  model.bindIndexBuffer(commandBuffer);
  model.bindVertexBuffers(commandBuffer);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  model.submitForRender(commandBuffer);
}

}
