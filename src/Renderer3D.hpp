#pragma once

#include "Event.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "SkyRenderer.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"

namespace Yona {

class Renderer3D : public DelegateResize {
public:
  Renderer3D(VulkanContext &graphicsContext);

  void init();
  void tick(const Tick &tick, VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  const RenderStage &mainRenderStage() const;

private:
  Camera mCamera;
  /* Temporary */
  CameraProperties mCameraProperties;

  GBuffer mGBuffer;
  SkyRenderer mSkyRenderer;
  Resolution mPipelineViewport;
  VulkanContext &mGraphicsContext;
};

}
