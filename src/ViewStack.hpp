#pragma once

#include "View.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::View {

class ViewStack {
public:
  ViewStack(
    Graphics::Renderer3D &renderer3D,
    Graphics::VulkanContext &graphicsContext);

  void init();

  void createView(const char *name, View *view);
  void setViewHierarchy(uint32_t count, const char **views);

  void processEvents(Core::EventQueue &queue, const Core::Tick &tick);
  void distributeInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker);

  /* Gets called inside the swapchain render pass */
  void render(
    const Graphics::VulkanFrame &frame, const Core::Tick &tick);

  void presentOutput(const Graphics::VulkanFrame &frame);

  void push(const char *name);
  View *pop();

private:
  static constexpr uint32_t MAX_VIEWS = 20;

  // Usually, at index 0, you would have the game world view
  FastMapStd<std::string, View *, MAX_VIEWS> mViews;
  FastMapHandle mViewStack[MAX_VIEWS];
  uint32_t mCurrentViewCount;

  Graphics::VulkanPipeline mFinalRender;
  Graphics::VulkanContext &mGraphicsContext;
  Graphics::Renderer3D &mRenderer3D;

  /* Determines until which view to forward input to */
  int mFocusedView;
};

}
