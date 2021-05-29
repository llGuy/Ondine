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

  void processInputEvent(Event *ev);
  void processDeferredEvents(VulkanContext &graphicsContext);

private:
  static constexpr uint32_t MAX_EVENT_FUNCTORS = 20;

  struct DeferredEventProcParams {
    EditorView *editorView;
    VulkanContext &graphicsContext;
    void *data;
  };

  static void handleResize(DeferredEventProcParams &params);

  struct DeferredEventFunctor {
    void (*proc)(DeferredEventProcParams &params);
    void *data;
  };

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
  // Some events cannot be handled directly - need to wait for render to start
  DeferredEventFunctor mDeferredEvents[MAX_EVENT_FUNCTORS];
  uint32_t mDeferredEventCount;
};

}
