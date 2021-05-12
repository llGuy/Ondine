#pragma once

#include "yona_vulkan_frame.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

class RenderStage {
public:
  void init(const VulkanDevice &device, const VulkanRenderPassConfig &config);

  void startStage(const VulkanFrame &frame);
  void nextSubpass(const VulkanFrame &frame);
  void endStage(const VulkanFrame &frame);

private:
  VulkanRenderPass mRenderPass;
};

}
