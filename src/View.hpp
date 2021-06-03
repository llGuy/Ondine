#pragma once

#include "Tick.hpp"
#include "Event.hpp"
#include "VulkanContext.hpp"
#include "VulkanUniform.hpp"

namespace Ondine {

/* 
   These structures are here so that when changing params, don't need to
   worry about changing it for all child classes
*/

struct ViewProcessEventsParams {
  EventQueue &queue;
  const Tick &tick;
  VulkanContext &graphicsContext;
};

struct ViewRenderParams {
  VulkanContext &graphicsContext;
  const VulkanUniform &previousOutput;
  const VulkanFrame &frame;
  const Tick &tick;
};

enum class FocusedView {
  Previous,
  Current,
  Next,
  NoChange
};

class View {
public:
  View() = default;
  virtual ~View() {};

  /* Used for processing events like viewport resize, etc... */
  virtual void processEvents(ViewProcessEventsParams &) = 0;

  virtual void render(ViewRenderParams &) = 0;

  /* Tracking input will return which view to focus in the next frame */
  virtual FocusedView trackInput(
    const Tick &tick, const InputTracker &tracker) = 0;

  /* Each view will yield an output image as a result of rendering */
  virtual const VulkanUniform &getOutput() const = 0;
};

}
