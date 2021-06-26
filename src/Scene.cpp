#include "FileSystem.hpp"
#include "Application.hpp"
#include "Scene.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

Scene::Scene(ModelManager &modelManager)
  : mModelManager(modelManager),
    mSceneObjects(MAX_SCENE_OBJECTS_COUNT) {
  
}

void Scene::init(
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
      sizeof(glm::mat4),
      // Camera UBO
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      // Planet UBO
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    modelConfig.configureVertexInput(pipelineConfig);
      
    mTestPipeline.init(
      graphicsContext.device(),
      graphicsContext.descriptorLayouts(),
      pipelineConfig);
  }
}

void Scene::submit(
  const Camera &camera,
  const PlanetRenderer &planet,
  VulkanFrame &frame) {
  // Render test model
  auto &commandBuffer = frame.primaryCommandBuffer;
  commandBuffer.bindPipeline(mTestPipeline);
  commandBuffer.bindUniforms(camera.uniform(), planet.uniform());

  // For now just render one model
  auto &model = mModelManager.getStaticModel(mTestModel);
  model.bindIndexBuffer(commandBuffer);
  model.bindVertexBuffers(commandBuffer);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  for (int i = 0; i < mSceneObjects.size(); ++i) {
    auto &sceneObj = mSceneObjects[i];
    if (sceneObj.isInitialised) {
      commandBuffer.pushConstants(sizeof(glm::mat4), &sceneObj.transform);
      model.submitForRender(commandBuffer);
    }
  }
}

SceneObjectHandle Scene::createSceneObject() {
  auto handle = (SceneObjectHandle)mSceneObjects.add();
  mSceneObjects[handle].isInitialised = true;
  return handle;
}

void Scene::destroySceneObject(SceneObjectHandle handle) {
  assert(handle != SCENE_OBJECT_HANDLE_INVALID);
  mSceneObjects[handle].isInitialised = false;
  mSceneObjects.remove(handle);
}

SceneObject &Scene::getSceneObject(SceneObjectHandle handle) {
  assert(handle != SCENE_OBJECT_HANDLE_INVALID);
  return mSceneObjects[handle];
}

}
