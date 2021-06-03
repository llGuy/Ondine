#pragma once

#include "View.hpp"
#include "Buffer.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine {

class ViewStack {
public:
  ViewStack(
    VulkanContext &graphicsContext);

  void init();

  void processEvents(EventQueue &queue, const Tick &tick);
  void distributeInput(
    const Tick &tick,
    const InputTracker &inputTracker);

  /* Gets called inside the swapchain render pass */
  void render(
    const VulkanFrame &frame, const Tick &tick);

  void presentOutput(const VulkanFrame &frame);

  void push(View *view);
  View *pop();

private:
  static constexpr uint32_t MAX_VIEWS = 20;

  // Usually, at index 0, you would have the game world view
  Array<View *> mViews;
  VulkanPipeline mFinalRender;
  VulkanContext &mGraphicsContext;
  /* Determines until which view to forward input to */
  int mFocusedView;
};

}
