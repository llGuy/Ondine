#pragma once

#include "Tick.hpp"
#include "Event.hpp"
#include "VulkanContext.hpp"
#include "VulkanUniform.hpp"

namespace Ondine::View {

/* 
   These structures are here so that when changing params, don't need to
   worry about changing it for all child classes
*/

struct ViewProcessEventsParams {
  Core::EventQueue &queue;
  const Core::Tick &tick;
  Graphics::VulkanContext &graphicsContext;
};

struct ViewRenderParams {
  Graphics::VulkanContext &graphicsContext;
  const Graphics::VulkanUniform &previousOutput;
  const Graphics::VulkanFrame &frame;
  const Core::Tick &tick;
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
    const Core::Tick &tick, const Core::InputTracker &tracker) = 0;

  /* Each view will yield an output image as a result of rendering */
  virtual const Graphics::VulkanUniform &getOutput() const = 0;
};

}
