#pragma once

#include <imgui.h>
#include "View.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"

namespace Ondine {

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
  Resolution mViewportResolution;
  OnEventProc mOnEvent;
  VulkanRenderPass mRenderPass;
  VulkanFramebuffer mFramebuffer;
  VulkanTexture mTarget;
  VulkanUniform mTargetUniform;
  VulkanPipeline mRenderViewport;
};

}
