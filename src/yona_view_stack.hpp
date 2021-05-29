#pragma once

#include "yona_view.hpp"
#include "yona_buffer.hpp"

namespace Yona {

class EventQueue;
class Tick;
class VulkanFrame;

class ViewStack {
public:
  ViewStack();

  void processEvents(EventQueue &queue, const Tick &tick);
  /* Gets called inside the swapchain render pass */
  void render(
    const VulkanContext &graphicsContext,
    const VulkanFrame &frame, const Tick &tick);

  void push(View *view);
  View *pop();

private:
  static constexpr uint32_t MAX_VIEWS = 20;

  // Usually, at index 0, you would have the game world view
  Array<View *> mViews;
};

}
