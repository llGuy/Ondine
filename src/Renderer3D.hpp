#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "SkyRenderer.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "DeferredLighting.hpp"

namespace Ondine {

class Renderer3D :
  public DelegateResize,
  public DelegateTrackInput {
public:
  Renderer3D(
    VulkanContext &graphicsContext);

  void init();
  void tick(const Tick &tick, VulkanFrame &frame);

  void resize(Resolution newResolution) override;
  void trackInput(
    const Tick &tick,
    const InputTracker &inputTracker) override;

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
  DeferredLighting mDeferredLighting;

  Resolution mPipelineViewport;

  VulkanContext &mGraphicsContext;
};

}
