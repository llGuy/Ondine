#pragma once

#include "yona_vulkan_frame.hpp"
#include "yona_vulkan_uniform.hpp"
#include "yona_vulkan_framebuffer.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

/* 
   Doesn't declare any state because the render stage may want
   to have multiple render passes / FBOs
*/
class RenderStage {
public:
  virtual const VulkanRenderPass &renderPass() const = 0;
  virtual const VulkanFramebuffer &framebuffer() const = 0;
  virtual const VulkanUniform &uniform() const = 0;
  virtual VkExtent2D extent() const = 0;
};

}
