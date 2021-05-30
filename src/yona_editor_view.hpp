#pragma once

#include <imgui.h>
#include "yona_view.hpp"
#include "yona_vulkan_render_pass.hpp"
#include "yona_vulkan_framebuffer.hpp"

namespace Yona {

class VulkanContext;
class WindowContextInfo;

class EditorView : public View {
public:
  EditorView(
    const WindowContextInfo &contextInfo,
    VulkanContext &graphicsContext,
    OnEventProc proc);

  ~EditorView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  const VulkanUniform &getOutput() const override;

private:
  void initRenderTarget(VulkanContext &graphicsContext);
  void destroyRenderTarget(VulkanContext &graphicsContext);
  void initViewportRendering(VulkanContext &graphicsContext);
  void initImguiContext(
    const WindowContextInfo &contextInfo, VulkanContext &graphicsContext);
  void tickMenuBar();

  void processInputEvent(Event *ev, ViewProcessEventsParams &params);
  void processDeferredEvents(VulkanContext &graphicsContext);

private:
  ImGuiID mDock;
  bool mIsDockLayoutInitialised;
  OnEventProc mOnEvent;
  VulkanRenderPass mRenderPass;
  VulkanFramebuffer mFramebuffer;
  VulkanTexture mTarget;
  VulkanUniform mTargetUniform;
  Resolution mViewportResolution;
  VulkanPipeline mRenderViewport;
};

}
