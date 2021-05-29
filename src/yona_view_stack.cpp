#include <assert.h>
#include "yona_event.hpp"
#include "yona_view_stack.hpp"

namespace Yona {

ViewStack::ViewStack()
  : mViews(MAX_VIEWS) {
  
}

void ViewStack::processEvents(EventQueue &queue, const Tick &tick) {
  for (int i = mViews.size - 1; i >= 0; --i) {
    ViewProcessEventsParams params {queue, tick};
    mViews[i]->processEvents(params);
  }
}

void ViewStack::render(
  const VulkanContext &graphicsContext,
  const VulkanFrame &frame, const Tick &tick) {
  VulkanUniform previousOutput = {};
  for (int i = 0; i < mViews.size; ++i) {
    ViewRenderParams params {
      graphicsContext,
      previousOutput,
      frame,
      tick
    };

    mViews[i]->render(params);
    previousOutput = mViews[i]->getOutput();
  }
}

void ViewStack::push(View *view) {
  mViews[mViews.size++] = view;
}

View *ViewStack::pop() {
  assert(mViews.size > 0);
  return mViews[--mViews.size];
}

}
