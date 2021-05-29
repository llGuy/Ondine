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
    VulkanContext &graphicsContext);

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  const VulkanUniform &getOutput() const override;

private:
  void initRenderTarget(VulkanContext &graphicsContext);
  void initImguiContext(
    const WindowContextInfo &contextInfo, VulkanContext &graphicsContext);
  void tickMenuBar();

private:
  bool mIsDockLayoutInitialised;
  ImGuiID mDock;

  VulkanRenderPass mRenderPass;
  VulkanFramebuffer mFramebuffer;
  VulkanTexture mTarget;
  VulkanUniform mTargetUniform;
};

}
