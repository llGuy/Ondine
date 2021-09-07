#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Utils.hpp"
#include "Scene.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "MapView.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "Clipping.hpp"
#include "Pixelater.hpp"
#include "EditorView.hpp"
#include "SkyRenderer.hpp"
#include "ToneMapping.hpp"
#include "RenderStage.hpp"
#include "ModelManager.hpp"
#include "RenderMethod.hpp"
#include "StarRenderer.hpp"
#include "WaterRenderer.hpp"
#include "VulkanContext.hpp"
#include "TerrainRenderer.hpp"
#include "DeferredLighting.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::Graphics {

class Renderer3D :
  public DelegateResize {
public:
  Renderer3D(
    VulkanContext &graphicsContext);

  void init();
  void shutdown();

  void tick(const Core::Tick &tick, Graphics::VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  void trackPath(Core::TrackPathID id, const char *path);

  const RenderStage &mainRenderStage() const;

public:
  Scene *createScene();
  void bindScene(Scene *scene);

  inline PlanetProperties &planet() {
    return mPlanetProperties;
  }

public:
  Resolution pipelineViewport;

private:
  Camera mCamera;
  PlanetProperties mPlanetProperties;
  GBuffer mGBuffer;
  Clipping mClipping;
  StarRenderer mStarRenderer;
  SkyRenderer mSkyRenderer;
  PlanetRenderer mPlanetRenderer;
  WaterRenderer mWaterRenderer;
  TerrainRenderer mTerrainRenderer;
  DeferredLighting mDeferredLighting;
  Pixelater mPixelater;
  ToneMapping mToneMapping;
  RenderMethodEntries mRenderMethods;
  RenderShaderEntries mShaderEntries;
  ModelManager mModelManager;
  Scene *mBoundScene;
  VulkanContext &mGraphicsContext;

  // Only relevant in DEV builds
  friend class View::EditorView;
  friend class View::MapView;
};

}
