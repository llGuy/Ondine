#pragma once

#include "yona_tick.hpp"
#include "yona_event.hpp"
#include "yona_vulkan_context.hpp"
#include "yona_vulkan_uniform.hpp"

namespace Yona {

/* 
   These structures are here so that when changing params, don't need to
   worry about changing it for all child classes
*/

struct ViewProcessEventsParams {
  EventQueue &queue;
  const Tick &tick;
};

struct ViewRenderParams {
  VulkanContext &graphicsContext;
  const VulkanUniform &previousOutput;
  const VulkanFrame &frame;
  const Tick &tick;
};

class View {
public:
  View() = default;
  virtual ~View() {};

  virtual void processEvents(ViewProcessEventsParams &) = 0;
  virtual void render(ViewRenderParams &) = 0;

  virtual const VulkanUniform &getOutput() const = 0;
};

}
