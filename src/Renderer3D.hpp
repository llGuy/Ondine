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

namespace Ondine {

class Renderer3D : public DelegateResize {
public:
  Renderer3D(
    VulkanContext &graphicsContext,
    const InputTracker &inputTracker);

  void init();
  void tick(const Tick &tick, VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  const RenderStage &mainRenderStage() const;

private:
  // Temporary
  void tickCamera(const Tick &tick, VulkanFrame &frame);

private:
  Camera mCamera;

  /* Temporary */
  CameraProperties mCameraProperties;
  PlanetProperties mPlanetProperties;

  GBuffer mGBuffer;
  SkyRenderer mSkyRenderer;
  PlanetRenderer mPlanetRenderer;

  Resolution mPipelineViewport;

  VulkanContext &mGraphicsContext;
  const InputTracker &mInputTracker;
};

}
