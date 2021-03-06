#pragma once

#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include "SceneObject.hpp"
#include "DynamicArray.hpp"
#include "ModelManager.hpp"
#include "PlanetRenderer.hpp"
#include "TerrainRenderer.hpp"
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

using SceneObjectHandle = int32_t;
constexpr SceneObjectHandle SCENE_OBJECT_HANDLE_INVALID = 0xFFFFFFFF;

struct SceneDebug {
  uint32_t renderChunkOutlines : 1;
  uint32_t renderQuadTree: 1;
  uint32_t wireframeTerrain: 1;
  uint32_t bits : 29;
};

class Scene {
public:
  Scene(ModelManager &modelManager, RenderMethodEntries &renderMethods);

  void init(
    const GBuffer &gbuffer,
    VulkanContext &graphicsContext);

  void submit(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    TerrainRenderer &terrainRenderer,
    VulkanFrame &frame);

  void submitDebug(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    TerrainRenderer &terrainRenderer,
    VulkanFrame &frame);

  SceneObjectHandle createSceneObject(const char *renderMethodName);
  void destroySceneObject(SceneObjectHandle handle);

  SceneObject &getSceneObject(SceneObjectHandle handle);

public:
  CameraProperties camera;
  LightingProperties lighting;
  Terrain terrain;
  SceneDebug debug;

private:
  static constexpr uint32_t MAX_SCENE_OBJECTS_COUNT = 5000;

  DynamicArray<SceneObject> mSceneObjects;

  ModelManager &mModelManager;
  RenderMethodEntries &mRenderMethods;
};

}
