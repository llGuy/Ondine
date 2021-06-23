#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Utils.hpp"
#include "Window.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "SkyRenderer.hpp"
#include "RenderStage.hpp"
#include "ModelManager.hpp"
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
  void tick(const Core::Tick &tick, Graphics::VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  void trackInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker) override;

  const RenderStage &mainRenderStage() const;

private:
  Camera mCamera;

  /* Temporary while we don't have a proper scene system */
  CameraProperties mCameraProperties;
  PlanetProperties mPlanetProperties;
  LightingProperties mLightingProperties;

  GBuffer mGBuffer;
  SkyRenderer mSkyRenderer;
  PlanetRenderer mPlanetRenderer;
  WaterRenderer mWaterRenderer;
  DeferredLighting mDeferredLighting;

  ModelManager mModelManager;
  Scene mScene;

  Resolution mPipelineViewport;

  VulkanContext &mGraphicsContext;
};

}
