#pragma once

#include "yona_render_stage.hpp"
#include "yona_vulkan_context.hpp"

namespace Yona {

class Renderer {
public:
  void init(const VulkanContext &vulkanContext);

private:
  void initRenderPipelineStages(const VulkanContext &vulkanContext);

private:
};

}
