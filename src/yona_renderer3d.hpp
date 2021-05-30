#pragma once

#include "yona_event.hpp"
#include "yona_window.hpp"
#include "yona_gbuffer.hpp"
#include "yona_renderer_sky.hpp"
#include "yona_render_stage.hpp"
#include "yona_vulkan_context.hpp"
#include "yona_delegate_resize.hpp"

namespace Yona {

class Renderer3D : public DelegateResize {
public:
  Renderer3D(VulkanContext &graphicsContext);

  void init();
  void tick(const Tick &tick, VulkanFrame &frame);

  void resize(Resolution newResolution) override;

  const RenderStage &mainRenderStage() const;

private:
  RendererSky mRendererSky;
  GBuffer mGBuffer;
  Resolution mPipelineViewport;
  VulkanContext &mGraphicsContext;
};

}
