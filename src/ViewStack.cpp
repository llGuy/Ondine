#include <assert.h>
#include "Application.hpp"
#include "Event.hpp"
#include "ViewStack.hpp"
#include "FileSystem.hpp"

namespace Ondine {

ViewStack::ViewStack(
  VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext),
    mFocusedView(0) {
  
}

void ViewStack::init() {
  mViews.init(MAX_VIEWS);

  File vshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/TexturedQuad.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer vsh = vshFile.readBinary();

  File fshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/TexturedQuad.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer fsh = fshFile.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {mGraphicsContext.finalRenderPass(), 0},
    VulkanShader(
      mGraphicsContext.device(), vsh, VulkanShaderType::Vertex),
    VulkanShader(
      mGraphicsContext.device(), fsh, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0,
    VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mFinalRender.init(
    mGraphicsContext.device(),
    mGraphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void ViewStack::processEvents(EventQueue &queue, const Tick &tick) {
  for (int i = mViews.size - 1; i >= 0; --i) {
    ViewProcessEventsParams params {queue, tick, mGraphicsContext};
    mViews[i]->processEvents(params);
  }
}

void ViewStack::distributeInput(
  const Tick &tick, const InputTracker &inputTracker) {
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
  const VulkanFrame &frame, const Tick &tick) {
  VulkanUniform previousOutput = {};
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

void ViewStack::presentOutput(const VulkanFrame &frame)  {
  const auto &output = mViews[mViews.size - 1]->getOutput();

  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mFinalRender);
  commandBuffer.bindUniforms(output);
  commandBuffer.setViewport({frame.viewport.width, frame.viewport.height});
  commandBuffer.setScissor({}, {frame.viewport.width, frame.viewport.height});
  commandBuffer.draw(4, 1, 0, 0);
}

}
