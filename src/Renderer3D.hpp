#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Utils.hpp"
#include "Window.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "Pixelater.hpp"
#include "EditorView.hpp"
#include "SkyRenderer.hpp"
#include "RenderStage.hpp"
#include "ModelManager.hpp"
#include "RenderMethod.hpp"
#include "StarRenderer.hpp"
#include "WaterRenderer.hpp"
#include "VulkanContext.hpp"
#include "DeferredLighting.hpp"

namespace Ondine::Graphics {

class Renderer3D :
  public DelegateResize,
  public DelegateTrackInput {
public:
  Renderer3D(
    VulkanContext &graphicsContext);

  void init();
  void shutdown();

  void tick(const Core::Tick &tick, Graphics::VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  void trackInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker) override;

  void trackPath(Core::TrackPathID id, const char *path);

  const RenderStage &mainRenderStage() const;

public:
  Scene *createScene();
  void bindScene(Scene *scene);

private:
  Camera mCamera;

  /* Temporary while we don't have a proper scene system */
  CameraProperties mCameraProperties;
  PlanetProperties mPlanetProperties;
  LightingProperties mLightingProperties;

  GBuffer mGBuffer;
  StarRenderer mStarRenderer;
  SkyRenderer mSkyRenderer;
  PlanetRenderer mPlanetRenderer;
  WaterRenderer mWaterRenderer;
  DeferredLighting mDeferredLighting;
  Pixelater mPixelater;
  RenderMethodEntries mRenderMethods;
  RenderShaderEntries mShaderEntries;

  ModelManager mModelManager;
  Scene *mBoundScene;

  Resolution mPipelineViewport;

  VulkanContext &mGraphicsContext;

  // Only relevant in DEV builds
  friend class View::EditorView;
};

}
