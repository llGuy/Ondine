#pragma once

#include "Event.hpp"
#include "Window.hpp"
#include "GBuffer.hpp"
#include "SkyRenderer.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "DelegateResize.hpp"

namespace Yona {

class Renderer3D : public DelegateResize {
public:
  Renderer3D(VulkanContext &graphicsContext);

  void init();
  void tick(const Tick &tick, VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  const RenderStage &mainRenderStage() const;

private:
  SkyRenderer mSkyRenderer;
  GBuffer mGBuffer;
  Resolution mPipelineViewport;
  VulkanContext &mGraphicsContext;
};

}
