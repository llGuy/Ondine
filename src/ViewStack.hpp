#pragma once

#include "View.hpp"
#include "Buffer.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::View {

class ViewStack {
public:
  ViewStack(
    Graphics::VulkanContext &graphicsContext);

  void init();

  void processEvents(Core::EventQueue &queue, const Core::Tick &tick);
  void distributeInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker);

  /* Gets called inside the swapchain render pass */
  void render(
    const Graphics::VulkanFrame &frame, const Core::Tick &tick);

  void presentOutput(const Graphics::VulkanFrame &frame);

  void push(View *view);
  View *pop();

private:
  static constexpr uint32_t MAX_VIEWS = 20;

  // Usually, at index 0, you would have the game world view
  Array<View *> mViews;
  Graphics::VulkanPipeline mFinalRender;
  Graphics::VulkanContext &mGraphicsContext;
  /* Determines until which view to forward input to */
  int mFocusedView;
};

}
