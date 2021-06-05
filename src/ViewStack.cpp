#include <assert.h>
#include "Application.hpp"
#include "Event.hpp"
#include "ViewStack.hpp"
#include "FileSystem.hpp"

namespace Ondine::View {

ViewStack::ViewStack(
  Graphics::VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext),
    mFocusedView(0) {
  
}

void ViewStack::init() {
  mViews.init(MAX_VIEWS);

  Core::File vshFile = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    "res/spv/TexturedQuad.vert.spv",
    Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer vsh = vshFile.readBinary();

  Core::File fshFile = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    "res/spv/TexturedQuad.frag.spv",
    Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer fsh = fshFile.readBinary();

  Graphics::VulkanPipelineConfig pipelineConfig(
    {mGraphicsContext.finalRenderPass(), 0},
    Graphics::VulkanShader(
      mGraphicsContext.device(), vsh, Graphics::VulkanShaderType::Vertex),
    Graphics::VulkanShader(
      mGraphicsContext.device(), fsh, Graphics::VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0,
    Graphics::VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mFinalRender.init(
    mGraphicsContext.device(),
    mGraphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void ViewStack::processEvents(Core::EventQueue &queue, const Core::Tick &tick) {
  for (int i = mViews.size - 1; i >= 0; --i) {
    ViewProcessEventsParams params {queue, tick, mGraphicsContext};
    mViews[i]->processEvents(params);
  }
}

void ViewStack::distributeInput(
  const Core::Tick &tick, const Core::InputTracker &inputTracker) {
  for (int i = mViews.size - 1; i >= mFocusedView; --i) {
    View *view = mViews[i];
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
      if (mFocusedView < mViews.size - 1) {
        ++mFocusedView;
      }
    } break;
    }
  }
}

void ViewStack::render(
  const Graphics::VulkanFrame &frame, const Core::Tick &tick) {
  Graphics::VulkanUniform previousOutput = {};
  for (int i = 0; i < mViews.size; ++i) {
    ViewRenderParams params {
      mGraphicsContext,
      previousOutput,
      frame,
      tick
    };

    mViews[i]->render(params);
    previousOutput = mViews[i]->getOutput();
  }
}

void ViewStack::push(View *view) {
  mFocusedView = mViews.size;
  mViews[mViews.size++] = view;
}

View *ViewStack::pop() {
  assert(mViews.size > 0);
  return mViews[--mViews.size];
}

void ViewStack::presentOutput(const Graphics::VulkanFrame &frame)  {
  const auto &output = mViews[mViews.size - 1]->getOutput();

  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mFinalRender);
  commandBuffer.bindUniforms(output);
  commandBuffer.setViewport({frame.viewport.width, frame.viewport.height});
  commandBuffer.setScissor({}, {frame.viewport.width, frame.viewport.height});
  commandBuffer.draw(4, 1, 0, 0);
}

}
