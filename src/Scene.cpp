#include "FileSystem.hpp"
#include "Application.hpp"
#include "Scene.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

Scene::Scene(ModelManager &modelManager, RenderMethodEntries &renderMethods)
  : mModelManager(modelManager),
    mRenderMethods(renderMethods),
    mSceneObjects(MAX_SCENE_OBJECTS_COUNT) {
  memset(&lighting, 0, sizeof(lighting));
  memset(&camera, 0, sizeof(camera));
}

void Scene::init(
  const GBuffer &gbuffer,
  VulkanContext &graphicsContext) {

}

void Scene::submit(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  TerrainRenderer &terrainRenderer,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  const RenderResources RESOURCES = {
    camera, planet, clipping
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

  terrainRenderer.render(
    camera, planet, clipping, terrain, frame);
}

void Scene::submitDebug(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  TerrainRenderer &terrainRenderer,
  VulkanFrame &frame) {
  if (debug.renderChunkOutlines) {
    terrainRenderer.renderChunkOutlines(
      camera, planet, clipping, terrain, frame);
  }

  if (debug.renderQuadTree) {
    terrainRenderer.renderQuadTree(
      camera, planet, clipping, terrain, frame);
  }
}

SceneObjectHandle Scene::createSceneObject(const char *renderMethod) {
  auto handle = (SceneObjectHandle)mSceneObjects.add();
  mSceneObjects[handle].isInitialised = true;
  mSceneObjects[handle].renderMethod = mRenderMethods.getHandle(renderMethod);
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
