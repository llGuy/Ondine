#pragma once

#include "yona_gbuffer.hpp"
#include "yona_renderer_sky.hpp"
#include "yona_render_stage.hpp"
#include "yona_vulkan_context.hpp"

namespace Yona {

class Renderer {
public:
  void init(VulkanContext &vulkanContext);

  void tick(const Tick &tick, VulkanFrame &frame);
  void resize(VulkanContext &vulkanContext);

  const RenderStage &mainRenderStage() const;

private:
  RendererSky mRendererSky;
  GBuffer mGBuffer;
};

}
