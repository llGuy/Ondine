#pragma once

#include <imgui.h>
#include "View.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"

namespace Ondine::View {

class VulkanContext;
class WindowContextInfo;

enum class EditorWindow {
  Assets,
  Viewport,
  Console,
  GameState,
  General,
  None,
  Count
};

class EditorView : public View {
public:
  EditorView(
    const Core::WindowContextInfo &contextInfo,
    Graphics::VulkanContext &graphicsContext,
    Core::OnEventProc proc);

  ~EditorView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Core::Tick &tick, const Core::InputTracker &tracker) override;

  const Graphics::VulkanUniform &getOutput() const override;

private:
  void initRenderTarget(Graphics::VulkanContext &graphicsContext);
  void destroyRenderTarget(Graphics::VulkanContext &graphicsContext);
  void initViewportRendering(Graphics::VulkanContext &graphicsContext);
  void initImguiContext(
    const Core::WindowContextInfo &contextInfo,
    Graphics::VulkanContext &graphicsContext);
  void tickMenuBar();

  void processInputEvent(Core::Event *ev, ViewProcessEventsParams &params);
  void processDeferredEvents(Graphics::VulkanContext &graphicsContext);

  const char *&windowName(EditorWindow window);

private:
  ImGuiID mDock;
  bool mIsDockLayoutInitialised;
  Resolution mViewportResolution;
  Core::OnEventProc mOnEvent;
  Graphics::VulkanRenderPass mRenderPass;
  Graphics::VulkanFramebuffer mFramebuffer;
  Graphics::VulkanTexture mTarget;
  Graphics::VulkanUniform mTargetUniform;
  Graphics::VulkanPipeline mRenderViewport;

  EditorWindow mFocusedWindow;
  uint8_t mChangedFocusToViewport : 4;
  uint8_t mChangedFocusToEditor : 4;
  const char *mWindowNames[(int)EditorWindow::Count];
};

}
