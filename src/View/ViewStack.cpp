#include <assert.h>
#include "Event.hpp"
#include "ViewStack.hpp"
#include "FileSystem.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

ViewStack::ViewStack(
  Graphics::Renderer3D &renderer,
  Graphics::VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext),
    mRenderer3D(renderer),
    mFocusedView(0) {

}

void ViewStack::init() {
  mViews.init();
  mCurrentViewCount = 0;

  Graphics::VulkanPipelineConfig pipelineConfig(
    {mGraphicsContext.finalRenderPass(), 0},
    Graphics::VulkanShader(
      mGraphicsContext.device(), "res/spv/TexturedQuad.vert.spv"),
    Graphics::VulkanShader(
      mGraphicsContext.device(), "res/spv/TexturedQuad.frag.spv"));

  pipelineConfig.configurePipelineLayout(
    0,
    Graphics::VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mFinalRender.init(
    mGraphicsContext.device(),
    mGraphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void ViewStack::createView(const char *name, View *view) {
  mViews.insert(name, view);
}

void ViewStack::setViewHierarchy(uint32_t count, const char **views) {
  mCurrentViewCount = 0;
  for (int i = 0; i < count; ++i) {
    push(views[i]);
  }
}

void ViewStack::processEvents(Core::EventQueue &queue, const Core::Tick &tick) {
  // Process some events first
  queue.process([this](Core::Event *ev) {
    switch (ev->type) {
    case Core::EventType::ViewHierarchyChange: {
      auto *hierarchyEvent = (Core::EventViewHierarchyChange *)ev;
      setViewHierarchy(hierarchyEvent->views.size, hierarchyEvent->views.data);
      hierarchyEvent->isHandled = true;
    } break;

    default:;
    }
  });

  for (int i = mCurrentViewCount - 1; i >= 0; --i) {
    ViewProcessEventsParams params {queue, tick, mGraphicsContext};
    FastMapHandle handle = mViewStack[i];
    View *&view = mViews.getEntry(handle);
    view->processEvents(params);
  }
}

void ViewStack::distributeInput(
  const Core::Tick &tick, const Core::InputTracker &inputTracker) {
  for (int i = mCurrentViewCount - 1; i >= mFocusedView; --i) {
    FastMapHandle handle = mViewStack[i];
    View *&view = mViews.getEntry(handle);
    FocusedView res = view->trackInput(tick, inputTracker);

    switch (res) {
    case FocusedView::Next: {
      /* No action required */
      if (mFocusedView > 0) {
        --mFocusedView;
      }
    } break;

    case FocusedView::Current: {
      /* Break out of the loop */
      mFocusedView = i;
    } break;

    case FocusedView::Previous: {
      if (mFocusedView < mCurrentViewCount - 1) {
        ++mFocusedView;
      }
    } break;
    }
  }
}

void ViewStack::render(
  const Graphics::VulkanFrame &frame, const Core::Tick &tick) {
  Graphics::VulkanUniform previousOutput = {};
  for (int i = 0; i < mCurrentViewCount; ++i) {
    ViewRenderParams params {
      mGraphicsContext,
      previousOutput,
      frame,
      tick
    };

    FastMapHandle handle = mViewStack[i];
    View *&view = mViews.getEntry(handle);

    view->render(params);
    previousOutput = view->getOutput();
  }
}

void ViewStack::push(const char *name) {
  auto handle = mViews.getHandle(name);
  View *view = mViews.getEntry(handle);

  mFocusedView = mCurrentViewCount;
  mViewStack[mCurrentViewCount++] = handle;

  ViewPushParams params = {
    mRenderer3D
  };
 
  view->onPush(params);
}

View *ViewStack::pop() {
  assert(mCurrentViewCount > 0);
  return mViews.getEntry(mViewStack[--mCurrentViewCount]);
}

void ViewStack::presentOutput(const Graphics::VulkanFrame &frame)  {
  const auto &output = mViews.getEntry(
    mViewStack[mCurrentViewCount - 1])->getOutput();

  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mFinalRender);
  commandBuffer.bindUniforms(output);
  commandBuffer.setViewport({frame.viewport.width, frame.viewport.height});
  commandBuffer.setScissor({}, {frame.viewport.width, frame.viewport.height});
  commandBuffer.draw(4, 1, 0, 0);
}

}
