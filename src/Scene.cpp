#include "FileSystem.hpp"
#include "Application.hpp"
#include "Scene.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

Scene::Scene(ModelManager &modelManager, RenderMethodEntries &renderMethods)
  : mModelManager(modelManager),
    mRenderMethods(renderMethods),
    mSceneObjects(MAX_SCENE_OBJECTS_COUNT) {
  
}

void Scene::init(
  const GBuffer &gbuffer,
  VulkanContext &graphicsContext) {
  { // Create test model
    ModelConfig modelConfig;
    mTestModel = mModelManager.loadStaticModel(
      "res/model/UVSphere.fbx", graphicsContext, modelConfig);

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
    pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      
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
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  const RenderResources RESOURCES = {
    camera, planet
  };

  // Need to sort this to remove the need to repeat calls to binding resources
  for (int i = 0; i < mSceneObjects.size(); ++i) {
    auto &sceneObj = mSceneObjects[i];
    auto &renderMethod = mRenderMethods.getEntry(sceneObj.renderMethod);

    renderMethod.bindShader(frame);
    renderMethod.bindBuffers(frame);
    renderMethod.bindResources(frame, RESOURCES);
    renderMethod.pushConstant(frame, sceneObj);
    renderMethod.submit(frame);
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
