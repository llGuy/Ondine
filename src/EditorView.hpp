#pragma once

#include <imgui.h>
#include "View.hpp"
#include "Renderer3D.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"

namespace Ondine::Graphics {

class Renderer3D;

}

namespace Ondine::View {

enum class EditorWindow {
  Graphics,
  Viewport,
  Console,
  GameState,
  General,
  None,
  Count
};

enum class ViewportType {
  GameEditor,
  MapEditor
};

class EditorView : public View {
public:
  EditorView(
    const Core::WindowContextInfo &contextInfo,
    Graphics::VulkanContext &graphicsContext,
    Graphics::Renderer3D &renderer3D,
    Core::OnEventProc proc);

  ~EditorView() override;

  void onPush(ViewPushParams &params) override;

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
  void renderGeneralWindow();
  void renderGameStateWindow();
  void renderConsoleWindow();

  void processInputEvent(Core::Event *ev, ViewProcessEventsParams &params);
  void processDeferredEvents(Graphics::VulkanContext &graphicsContext);

  const char *&windowName(EditorWindow window);

  void renderGraphicsWindow();

private:
  static constexpr ImGuiWindowFlags WINDOW_FLAGS =
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse;

  ImGuiID mDock;
  bool mIsDockLayoutInitialised;
  Resolution mViewportResolution;
  Core::OnEventProc mOnEvent;
  Graphics::VulkanRenderPass mRenderPass;
  Graphics::VulkanFramebuffer mFramebuffer;
  Graphics::VulkanTexture mTarget;
  Graphics::VulkanUniform mTargetUniform;
  Graphics::VulkanPipeline mRenderViewport;
  Graphics::Renderer3D &mRenderer3D;

  EditorWindow mFocusedWindow;
  ViewportType mBoundViewport;
  uint8_t mChangedFocusToViewport : 4;
  uint8_t mChangedFocusToEditor : 4;
  const char *mWindowNames[(int)EditorWindow::Count];
};

}
