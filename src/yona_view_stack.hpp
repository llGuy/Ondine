#pragma once

#include "yona_view.hpp"
#include "yona_buffer.hpp"

namespace Yona {

class EventQueue;
class Tick;
class VulkanFrame;

class ViewStack {
public:
  ViewStack() = default;

  void processEvents(EventQueue &queue, const Tick &tick);
  /* Gets called inside the swapchain render pass */
  void render(const VulkanFrame &frame, const Tick &tick);

  void push(View *view);
  View *pop();

private:
  Array<View *> mViews;
};

}
