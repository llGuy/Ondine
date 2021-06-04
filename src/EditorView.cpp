#include <imgui.h>
#include "Application.hpp"
#include "Utils.hpp"
#include <imgui_internal.h>
#include "IOEvent.hpp"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "FileSystem.hpp"
#include "EditorView.hpp"
#include "GraphicsEvent.hpp"
#include "VulkanContext.hpp"

namespace Ondine {

EditorView::EditorView(
  const WindowContextInfo &contextInfo,
  VulkanContext &graphicsContext,
  OnEventProc onEventProc)
  : mIsDockLayoutInitialised(false),
    mViewportResolution{0, 0},
    mFocusedWindow(EditorWindow::None),
    mOnEvent(onEventProc) {
  windowName(EditorWindow::Assets) = "Assets";
  windowName(EditorWindow::Viewport) = "Viewport";
  windowName(EditorWindow::Console) = "Console";
  windowName(EditorWindow::GameState) = "Game State";
  windowName(EditorWindow::General) = "General";
  windowName(EditorWindow::None) = "None";
  mChangedFocusToViewport = false;
  mChangedFocusToEditor = false;

  { // Create render pass
    VulkanRenderPassConfig config(1, 1);
    config.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      VK_FORMAT_R8G8B8A8_UNORM);
    config.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);
    mRenderPass.init(graphicsContext.device(), config);
  }

  initRenderTarget(graphicsContext);
  initViewportRendering(graphicsContext);
  initImguiContext(contextInfo, graphicsContext);
}

EditorView::~EditorView() {
  
}

void EditorView::processEvents(ViewProcessEventsParams &params) {
  params.queue.process([this, &params](Event *ev) {
    switch (ev->category) {
    case EventCategory::Input: {
      processInputEvent(ev, params);
    } break;

    default:;
    }
  });
}

void EditorView::render(ViewRenderParams &params) {
  auto &commandBuffer = params.frame.primaryCommandBuffer;

  commandBuffer.beginRenderPass(
    mRenderPass, mFramebuffer, {0, 0},
    {params.frame.viewport.width, params.frame.viewport.height});

  params.graphicsContext.imgui().beginRender();

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);

  uint32_t flags = 
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_MenuBar;

  ImGuiID dock;

  if (ImGui::Begin("Main Window", NULL, flags)) {
    mDock = dock = ImGui::GetID("Main Dockspace");

    if (!mIsDockLayoutInitialised) {
      mIsDockLayoutInitialised = true;

      ImGui::DockBuilderRemoveNode(dock);
      ImGui::DockBuilderAddNode(dock, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dock, viewport->Size);

      ImGuiID top;
      ImGuiID bottom = ImGui::DockBuilderSplitNode(
        mDock, ImGuiDir_Down, 0.3f, NULL, &top);

      ImGuiID console;
      ImGuiID assets = ImGui::DockBuilderSplitNode(
        bottom, ImGuiDir_Left, 0.5f, NULL, &console);

      ImGuiID viewport;
      ImGuiID general = ImGui::DockBuilderSplitNode(
        top, ImGuiDir_Left, 0.3f, NULL, &viewport);

      ImGuiID game = ImGui::DockBuilderSplitNode(
        general, ImGuiDir_Down, 0.5f, NULL, &general);

      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Assets), assets);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Viewport), viewport);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Console), console);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::GameState), game);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::General), general);
    }

    tickMenuBar();

    ImGui::DockSpace(
      dock, ImVec2(0.0f, 0.0f),
      ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
  }

  EditorWindow prevFocusedWindow = mFocusedWindow;

  if (ImGui::Begin(
    windowName(EditorWindow::Assets), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Assets;
    }
    
    ImGui::End();
  }

  bool renderViewport = false;
  ImVec2 viewportPos = {};
  ImVec2 viewportSize = {};
  if ((renderViewport = ImGui::Begin(
    windowName(EditorWindow::Viewport), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration))) {
    viewportPos = ImGui::GetWindowPos();
    viewportSize = ImGui::GetWindowSize();

    if (mViewportResolution.width == 0) {
      mViewportResolution.width = (uint32_t)viewportSize.x;
      mViewportResolution.height = (uint32_t)viewportSize.y;
      renderViewport = false;
    }
    else if (
      mViewportResolution.width != (uint32_t)viewportSize.x ||
      mViewportResolution.height != (uint32_t)viewportSize.y) {
      // Trigger viewport resize event
      mViewportResolution.width = (uint32_t)viewportSize.x;
      mViewportResolution.height = (uint32_t)viewportSize.y;

      auto *resizeEvent = lnEmplaceAlloc<EventViewportResize>();
      resizeEvent->newResolution = mViewportResolution;
      mOnEvent(resizeEvent);
      // renderViewport = false;
    }

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Viewport;
    }
  
    ImGui::End();
  }

  if (ImGui::Begin(
    windowName(EditorWindow::Console), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Console;
    }

    ImGui::End();
  }

  if (ImGui::Begin(
    windowName(EditorWindow::GameState), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::GameState;
    }

    ImGui::End();
  }

  if (ImGui::Begin(
    windowName(EditorWindow::General), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
    ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::General;
    }

    ImGui::End();
  }

  params.graphicsContext.imgui().endRender(params.frame);

  // Render the viewport
  if (renderViewport) {
    commandBuffer.bindPipeline(mRenderViewport);
    commandBuffer.bindUniforms(params.previousOutput);
    commandBuffer.setScissor(
      {(int32_t)viewportPos.x, (int32_t)viewportPos.y},
      {(uint32_t)viewportPos.x + (uint32_t)viewportSize.x,
       (uint32_t)viewportPos.y + (uint32_t)viewportSize.y});
    commandBuffer.setViewport(
      {(uint32_t)viewportSize.x, (uint32_t)viewportSize.y},
      {(uint32_t)viewportPos.x, (uint32_t)viewportPos.y});
    commandBuffer.draw(4, 1, 0, 0);
  }

  commandBuffer.endRenderPass();

  if (mFocusedWindow != prevFocusedWindow) {
    if (mFocusedWindow == EditorWindow::Viewport) {
      mChangedFocusToViewport = true;
    }
    else {
      mChangedFocusToEditor = true;
    }
  }
}

const VulkanUniform &EditorView::getOutput() const {
  return mTargetUniform;
}

void EditorView::tickMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("Assets")) {
      }
      if (ImGui::MenuItem("Viewport")) {
      }
      if (ImGui::MenuItem("Console")) {
      }
      if (ImGui::MenuItem("Game State")) {
      }
      if (ImGui::MenuItem("General")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Viewport")) {
      if (ImGui::MenuItem("Game")) {
      }
      if (ImGui::MenuItem("Level Editor")) {
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void EditorView::initRenderTarget(VulkanContext &graphicsContext) {
  auto properties = graphicsContext.getProperties();
  { // Create target texture + uniform
    mTarget.init(
      graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
      TextureContents::Color, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR,
      {properties.swapchainExtent.width, properties.swapchainExtent.height, 1},
      1, 1);
    mTargetUniform.init(
      graphicsContext.device(), graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<VulkanTexture, AllocationType::Linear>(mTarget));
  }

  { // Create framebuffer
    VulkanFramebufferConfig config(1, mRenderPass);
    config.addAttachment(mTarget);

    mFramebuffer.init(graphicsContext.device(), config);
  }
}

void EditorView::destroyRenderTarget(VulkanContext &graphicsContext) {
  mFramebuffer.destroy(graphicsContext.device());
  mTarget.destroy(graphicsContext.device());
}

void EditorView::initViewportRendering(VulkanContext &graphicsContext) {
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
    {mRenderPass, 0},
    VulkanShader(
      graphicsContext.device(), vsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), fsh, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0,
    VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mRenderViewport.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void EditorView::initImguiContext(
  const WindowContextInfo &contextInfo,
  VulkanContext &graphicsContext) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGuiStyle* style = &ImGui::GetStyle();
  ImVec4* colors = style->Colors;
  // ImGui::StyleColorsDark();
  colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
  colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.048f, 0.048f, 0.048f, 1.000f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
  colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
  colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
  colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
  colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
  colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_NavWindowingHighlight] =
    ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

  style->ChildRounding = 2.0f;
  style->FrameBorderSize = 1.0f;
  style->FrameRounding = 1.0f;
  style->GrabMinSize = 7.0f;
  style->PopupRounding = 1.0f;
  style->ScrollbarRounding = 6.0f;
  style->ScrollbarSize = 13.0f;
  style->TabBorderSize = 1.0f;
  style->TabRounding = 0.0f;
  style->WindowRounding = 2.0f;

  graphicsContext.initImgui(contextInfo, mRenderPass);
}

void EditorView::processInputEvent(Event *ev, ViewProcessEventsParams &params) {
  switch (ev->type) {
  case EventType::Resize: {
    auto *resizeEvent = (EventResize *)ev;
    
    destroyRenderTarget(params.graphicsContext);
    initRenderTarget(params.graphicsContext);

    /* 
       We don't want this event to propagate to further layers. Another
       event will be sent down - we need to know how big the viewport is.
    */
    resizeEvent->isHandled = true;
  } break;

  default:;
  }
}

FocusedView EditorView::trackInput(
  const Tick &tick, const InputTracker &tracker) {
  if (mChangedFocusToViewport) {
    mChangedFocusToViewport = false;

    auto *cursorChange = lnEmplaceAlloc<EventCursorDisplayChange>();
    cursorChange->show = false;
    mOnEvent(cursorChange);

    return FocusedView::Next;
  }
  else if (mChangedFocusToEditor) {
    mChangedFocusToEditor = false;

    auto *cursorChange = lnEmplaceAlloc<EventCursorDisplayChange>();
    cursorChange->show = true;
    mOnEvent(cursorChange);

    return FocusedView::Current;
  }
  else {
    return FocusedView::NoChange;
  }
}

const char *&EditorView::windowName(EditorWindow window) {
  return mWindowNames[(int)window];
}

}
