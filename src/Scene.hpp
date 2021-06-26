#pragma once

#include "Camera.hpp"
#include "GBuffer.hpp"
#include "SceneObject.hpp"
#include "DynamicArray.hpp"
#include "ModelManager.hpp"
#include "PlanetRenderer.hpp"

namespace Ondine::Graphics {

using SceneObjectHandle = int32_t;
constexpr SceneObjectHandle SCENE_OBJECT_HANDLE_INVALID = 0xFFFFFFFF;

class Scene {
public:
  Scene(ModelManager &modelManager);

  void init(
    const GBuffer &gbuffer,
    VulkanContext &graphicsContext);

  void submit(
    const Camera &camera,
    const PlanetRenderer &planet,
    VulkanFrame &frame);

  SceneObjectHandle createSceneObject();
  void destroySceneObject(SceneObjectHandle handle);

  SceneObject &getSceneObject(SceneObjectHandle handle);

private:
  static constexpr uint32_t MAX_SCENE_OBJECTS_COUNT = 5000;

  DynamicArray<SceneObject> mSceneObjects;

  StaticModelHandle mTestModel;
  VulkanPipeline mTestPipeline;

  ModelManager &mModelManager;
};

}
