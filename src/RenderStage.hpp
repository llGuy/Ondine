#pragma once

#include "VulkanFrame.hpp"
#include "VulkanUniform.hpp"
#include "VulkanFramebuffer.hpp"
#include "VulkanRenderPass.hpp"

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
