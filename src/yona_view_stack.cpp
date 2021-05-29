#include <assert.h>
#include "yona_app.hpp"
#include "yona_event.hpp"
#include "yona_view_stack.hpp"
#include "yona_filesystem.hpp"

namespace Yona {

void ViewStack::init(VulkanContext &graphicsContext) {
  mViews.init(MAX_VIEWS);

  File vshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/render_sampled_quad.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer vsh = vshFile.readBinary();

  File fshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/render_sampled_quad.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer fsh = fshFile.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {graphicsContext.finalRenderPass(), 0},
    VulkanShader(
      graphicsContext.device(), vsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), fsh, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0,
    VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mFinalRender.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void ViewStack::processEvents(EventQueue &queue, const Tick &tick) {
  for (int i = mViews.size - 1; i >= 0; --i) {
    ViewProcessEventsParams params {queue, tick};
    mViews[i]->processEvents(params);
  }
}

void ViewStack::render(
  VulkanContext &graphicsContext,
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
