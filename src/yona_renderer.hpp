#pragma once

#include "yona_renderer_sky.hpp"
#include "yona_render_stage.hpp"
#include "yona_vulkan_context.hpp"

namespace Yona {

class Renderer {
public:
  void init(VulkanContext &vulkanContext);

private:
  void initRenderPipelineStages(VulkanContext &vulkanContext);

private:
  RendererSky mRendererSky;
};

}
