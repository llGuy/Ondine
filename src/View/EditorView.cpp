#include <imgui.h>
#include "DebugEvent.hpp"
#include "Utils.hpp"
#include "IOEvent.hpp"
#include "Settings.hpp"
#include "FileSystem.hpp"
#include "EditorView.hpp"
#include "EditorEvent.hpp"
#include "Renderer3D.hpp"
#include <imgui_internal.h>
#include "RendererDebug.hpp"
#include "GraphicsEvent.hpp"
#include "VulkanContext.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::View {

EditorView::EditorView(
  const Core::WindowContextInfo &contextInfo,
  Graphics::VulkanContext &graphicsContext,
  Graphics::Renderer3D &renderer3D,
  Core::OnEventProc onEventProc)
  : mIsDockLayoutInitialised(false),
    mViewportResolution{0, 0},
    mFocusedWindow(EditorWindow::None),
    mBoundViewport(ViewportType::GameEditor),
    mRenderer3D(renderer3D),
    mOnEvent(onEventProc) {
  windowName(EditorWindow::Graphics) = "Graphics";
  windowName(EditorWindow::Viewport) = "Viewport";
  windowName(EditorWindow::Console) = "Console";
  windowName(EditorWindow::GameState) = "Game State";
  windowName(EditorWindow::Tools) = "Tools";
  windowName(EditorWindow::Terrain) = "Terrain";
  windowName(EditorWindow::General) = "General";
  windowName(EditorWindow::None) = "None";
  mChangedFocusToViewport = false;
  mChangedFocusToEditor = false;

  { // Create render pass
    Graphics::VulkanRenderPassConfig config(1, 1);
    config.addAttachment(
      Graphics::LoadAndStoreOp::ClearThenStore,
      Graphics::LoadAndStoreOp::DontCareThenDontCare,
      Graphics::OutputUsage::FragmentShaderRead, Graphics::AttachmentType::Color,
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

#if 0
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = true;
  mOnEvent(cursorChange);
#endif
}

void EditorView::onPush(ViewPushParams &params) {
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = true;
  mOnEvent(cursorChange);
}

EditorView::~EditorView() {
  
}

void EditorView::processEvents(ViewProcessEventsParams &params) {
  params.queue.process([this, &params](Core::Event *ev) {
    switch (ev->category) {
    case Core::EventCategory::Input: {
      processInputEvent(ev, params);
    } break;

    default:;
    }
  });
}

void EditorView::render(ViewRenderParams &params) {
  auto &commandBuffer = params.frame.primaryCommandBuffer;

  commandBuffer.dbgBeginRegion("EditorStage", Graphics::DBG_EDITOR_COLOR);

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
        top, ImGuiDir_Right, 0.3f, NULL, &viewport);

      ImGuiID game = ImGui::DockBuilderSplitNode(
        general, ImGuiDir_Down, 0.5f, NULL, &general);

      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Graphics), assets);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Viewport), viewport);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Console), console);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::GameState), game);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Terrain), game);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::Tools), game);
      ImGui::DockBuilderDockWindow(windowName(EditorWindow::General), general);
    }

    tickMenuBar();

    ImGui::DockSpace(
      dock, ImVec2(0.0f, 0.0f),
      ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
  }

  EditorWindow prevFocusedWindow = mFocusedWindow;

  renderGraphicsWindow();

  bool renderViewport = false;
  ImVec2 viewportPos = {};
  ImVec2 viewportSize = {};
  if ((renderViewport = ImGui::Begin(
    windowName(EditorWindow::Viewport), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration))) {
    viewportPos = ImGui::GetWindowPos();
    viewportSize = ImGui::GetWindowSize();

#if 1
#ifdef __APPLE__
    viewportPos.x *= 1, viewportPos.y *= 2;
    viewportSize.x *= 2, viewportSize.y *= 2;
#endif
#endif

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

      auto *resizeEvent = lnEmplaceAlloc<Core::EventViewportResize>();
      resizeEvent->newResolution = mViewportResolution;
      mOnEvent(resizeEvent);
      // renderViewport = false;
    }

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Viewport;
    }
  
    ImGui::End();
  }

  renderConsoleWindow();

  if (mBoundViewport == ViewportType::GameEditor) {
    renderGameStateWindow();
  }
  else {
    renderTerrainWindow();
    renderToolsWindow();
  }

  renderGeneralWindow();

  params.graphicsContext.imgui().endRender(params.frame);

  // Render the viewport
  if (renderViewport) {
    commandBuffer.dbgInsertMarker(
      "EditorViewport", Graphics::DBG_VIEWPORT_COLOR);

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

  commandBuffer.dbgEndRegion();

  if (mFocusedWindow != prevFocusedWindow) {
    if (mFocusedWindow == EditorWindow::Viewport) {
      mChangedFocusToViewport = true;
    }
    else {
      mChangedFocusToEditor = true;
    }
  }
}

const Graphics::VulkanUniform &EditorView::getOutput() const {
  return mTargetUniform;
}

void EditorView::tickMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("Graphics")) {
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
      if (ImGui::MenuItem("Game Editor", "Alt+Shift+1")) {
        auto *hierarchyChange = lnEmplaceAlloc<Core::EventViewHierarchyChange>();
        hierarchyChange->views = makeArray<const char *, AllocationType::Linear>(
          "GameView", "EditorView");
        mOnEvent(hierarchyChange);

        mBoundViewport = ViewportType::GameEditor;
      }

      if (ImGui::MenuItem("Map Editor", "Alt+Shift+2")) {
        auto *hierarchyChange = lnEmplaceAlloc<Core::EventViewHierarchyChange>();
        hierarchyChange->views = makeArray<const char *, AllocationType::Linear>(
          "MapView", "EditorView");
        mOnEvent(hierarchyChange);

        mBoundViewport = ViewportType::MapEditor;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void EditorView::initRenderTarget(Graphics::VulkanContext &graphicsContext) {
  auto properties = graphicsContext.getProperties();
  { // Create target texture + uniform
    mTarget.init(
      graphicsContext.device(),
      Graphics::TextureType::T2D | Graphics::TextureType::Attachment,
      Graphics::TextureContents::Color,
      VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR,
      {properties.swapchainExtent.width, properties.swapchainExtent.height, 1},
      1, 1);
    mTargetUniform.init(
      graphicsContext.device(), graphicsContext.descriptorPool(),
      graphicsContext.descriptorLayouts(),
      makeArray<Graphics::VulkanTexture, AllocationType::Linear>(mTarget));
  }

  { // Create framebuffer
    Graphics::VulkanFramebufferConfig config(1, mRenderPass);
    config.addAttachment(mTarget);

    mFramebuffer.init(graphicsContext.device(), config);
  }
}

void EditorView::destroyRenderTarget(Graphics::VulkanContext &graphicsContext) {
  mFramebuffer.destroy(graphicsContext.device());
  mTarget.destroy(graphicsContext.device());
}

void EditorView::initViewportRendering(
  Graphics::VulkanContext &graphicsContext) {
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
    {mRenderPass, 0},
    Graphics::VulkanShader(
      graphicsContext.device(), vsh, Graphics::VulkanShaderType::Vertex),
    Graphics::VulkanShader(
      graphicsContext.device(), fsh, Graphics::VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0,
    Graphics::VulkanPipelineDescriptorLayout{
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

  mRenderViewport.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void EditorView::initImguiContext(
  const Core::WindowContextInfo &contextInfo,
  Graphics::VulkanContext &graphicsContext) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGuiStyle* style = &ImGui::GetStyle();
  ImVec4* colors = style->Colors;
  ImGui::StyleColorsDark();

  colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
  // colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.0f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
  // colors[ImGuiCol_ChildBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.0f);
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

void EditorView::processInputEvent(Core::Event *ev, ViewProcessEventsParams &params) {
  switch (ev->type) {
  case Core::EventType::Resize: {
    auto *resizeEvent = (Core::EventResize *)ev;
    
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
  const Core::Tick &tick, const Core::InputTracker &tracker) {
  // Keybindings
  if (tracker.key(Core::KeyboardButton::LeftAlt).isDown) {
    if (tracker.key(Core::KeyboardButton::LeftShift).isDown) {
      if (tracker.key(Core::KeyboardButton::One).didInstant) {
        auto *hierarchyChange = lnEmplaceAlloc<Core::EventViewHierarchyChange>();
        hierarchyChange->views = makeArray<const char *, AllocationType::Linear>(
          "GameView", "EditorView");
        mOnEvent(hierarchyChange);
        mBoundViewport = ViewportType::GameEditor;

        mChangedFocusToEditor = true;
        mChangedFocusToViewport = false;
      }
      else if (tracker.key(Core::KeyboardButton::Two).didInstant) {
        auto *hierarchyChange = lnEmplaceAlloc<Core::EventViewHierarchyChange>();
        hierarchyChange->views = makeArray<const char *, AllocationType::Linear>(
          "MapView", "EditorView");
        mOnEvent(hierarchyChange);
        mBoundViewport = ViewportType::MapEditor;

        mChangedFocusToEditor = true;
        mChangedFocusToViewport = false;
      }
    }
  }

  if (mChangedFocusToViewport) {
    mChangedFocusToViewport = false;

    auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
    cursorChange->show = false;
    mOnEvent(cursorChange);

    return FocusedView::Next;
  }
  else if (mChangedFocusToEditor) {
    mChangedFocusToEditor = false;

    auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
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

void EditorView::renderGeneralWindow() {
  if (ImGui::Begin(
    windowName(EditorWindow::General), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
    ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);
    int current = gSettings.maxFramerate;
    ImGui::SliderInt("Max Framerate", &current, 30, 120);
    gSettings.setMaxFramerate((float)current);

    switch (mBoundViewport) {
    case ViewportType::GameEditor: {
      ImGui::Text("3D Viewport: Game Editor");
    } break;

    case ViewportType::MapEditor: {
      ImGui::Text("3D Viewport: Map Editor");
    } break;

    default:;
    }

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::General;
    }

    if (ImGui::Button("Breakpoint")) {
      auto *breakpointEvent = lnEmplaceAlloc<Core::EventBreakpoint>();
      mOnEvent(breakpointEvent);
    }
  }

  ImGui::End();
}

void EditorView::renderGameStateWindow() {
  if (ImGui::Begin(
    windowName(EditorWindow::GameState), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::GameState;
    }
  }

  ImGui::End();
}

void EditorView::renderTerrainWindow() {
  if (ImGui::Begin(
    windowName(EditorWindow::Terrain), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Terrain;
    }

    const char *options[] = {
      "Density Paint Brush (Add)",
      "Density Paint Brush (Destroy)",
      "Color Paint Brush"
    };

    static int currentItem = 0;

    if (ImGui::Combo(
          "Tool", &currentItem, options, sizeof(options) / sizeof(options[0]))) {
      auto *toolChange = lnEmplaceAlloc<Core::EventTerrainToolChange>();
      toolChange->terrainTool = (Core::TerrainTool)currentItem;
      mOnEvent(toolChange);
    }

    static glm::vec3 paintColor = glm::vec3(0.0f);

    ImGui::ColorEdit3("Paint Color", &paintColor[0]);
  }

  ImGui::End();
}

void EditorView::renderToolsWindow() {
  if (ImGui::Begin(
    windowName(EditorWindow::Tools), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Tools;
    }

    auto *boundScene = mRenderer3D.mBoundScene;
    auto &terrainRenderer = mRenderer3D.mTerrainRenderer;

    if (ImGui::TreeNodeEx("Quad Tree", ImGuiTreeNodeFlags_SpanFullWidth)) {
      auto &arena = terrainRenderer.mIsosurface.mGPUVerticesAllocator;

      uint32_t totalFreeBlockCount = 0;
      uint32_t freeSectionsCount = 0;

      uint32_t index = arena.mFirstFreeBlock.next;
      auto *freeBlock = arena.getBlock(arena.mFirstFreeBlock.next);
  
      while (freeBlock) {
        ImGui::Text(
          "%d free blocks blocks at %p\n",
          freeBlock->blockCount, (void *)((uint64_t)index));

        totalFreeBlockCount += freeBlock->blockCount;
        freeSectionsCount++;

        index = freeBlock->next;
        freeBlock = arena.getBlock(freeBlock->next);
      }

      ImGui::Separator();

      ImGui::LabelText("Blocks free", "%u", totalFreeBlockCount);
      auto bytesFree =
        totalFreeBlockCount * Graphics::VulkanArenaAllocator::POOL_BLOCK_SIZE;
      auto kilobytesFree = bytesFree / 1024u;
      auto megabytesFree = kilobytesFree / 1024u;

      ImGui::LabelText("Bytes free", "%u", bytesFree);
      ImGui::LabelText("Kilobytes free", "%u", kilobytesFree);
      ImGui::LabelText("Megabytes free", "%u", megabytesFree);
      ImGui::LabelText("Number of contiguous free segments", "%u", freeSectionsCount);

      ImGui::Separator();

      ImGui::LabelText(
        "Allocated nodes", "%d",
        terrainRenderer.mQuadTree.mAllocatedNodeCount);

      auto *node = terrainRenderer.mQuadTree.getDeepestNode(
        terrainRenderer.mIsosurface.worldToQuadTreeCoords(
          terrainRenderer.mQuadTree,
          glm::vec2(
            boundScene->camera.wPosition.x,
            boundScene->camera.wPosition.z)));

      ImGui::Separator();
      if (node) {
        ImGui::LabelText("Inside child node", "%d", node->index);
        ImGui::LabelText("Node level", "%d", node->level);

        ImGui::Separator();
      }

      ImGui::LabelText(
        "Allocated chunk groups", "%d",
        terrainRenderer.mIsosurface.mIsoGroups.size());

      glm::vec2 camPos = terrainRenderer.mIsosurface.worldToQuadTreeCoords(
        terrainRenderer.mQuadTree,
        glm::vec2(boundScene->camera.wPosition.x,
                  boundScene->camera.wPosition.z));

      ImGui::LabelText(
        "In node", "%s", glm::to_string(camPos).c_str());

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Visual Debug", ImGuiTreeNodeFlags_SpanFullWidth)) {
      bool renderChunkOutlines = boundScene->debug.renderChunkOutlines;
      if (ImGui::Checkbox(
            "Render Chunk Outlines",
            &renderChunkOutlines));
      boundScene->debug.renderChunkOutlines = renderChunkOutlines;

      bool renderQuadTree = boundScene->debug.renderQuadTree;
      if (ImGui::Checkbox(
            "Render Quad Tree",
            &renderQuadTree));
      boundScene->debug.renderQuadTree = renderQuadTree;

      bool wireframeMode = boundScene->debug.wireframeTerrain;
      if (ImGui::Checkbox(
            "Wireframe Terrain",
            &wireframeMode));
      boundScene->debug.wireframeTerrain = wireframeMode;

      bool shouldUpdateQuadTree = terrainRenderer.mUpdateQuadTree;
      if (ImGui::Checkbox(
            "Update Quad Tree",
            &shouldUpdateQuadTree));
      terrainRenderer.setUpdateQuadTree(shouldUpdateQuadTree);

      ImGui::TreePop();
    }
  }

  ImGui::End();
}

void EditorView::renderConsoleWindow() {
  if (ImGui::Begin(
    windowName(EditorWindow::Console), nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {

    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Console;
    }
  }

  ImGui::End();
}

void EditorView::renderGraphicsWindow() {
  if (ImGui::Begin(windowName(EditorWindow::Graphics), nullptr, WINDOW_FLAGS)) {
    if (ImGui::IsWindowFocused()) {
      mFocusedWindow = EditorWindow::Graphics;
    }

    auto *boundScene = mRenderer3D.mBoundScene;

    if (ImGui::TreeNodeEx("Lighting", ImGuiTreeNodeFlags_SpanFullWidth)) {
      ImGui::SliderFloat(
        "Exposure",
        &mRenderer3D.mToneMapping.mProperties.exposure,
        1.0f, 50.0f);

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_SpanFullWidth)) {
      ImGui::DragFloat3("Position", &boundScene->camera.wPosition[0]);
      ImGui::DragFloat3("View Direction", &boundScene->camera.wViewDirection[0]);
      ImGui::DragFloat3("Up Vector", &boundScene->camera.wUp[0]);

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Post Process", ImGuiTreeNodeFlags_SpanFullWidth)) {
      ImGui::SliderFloat(
        "Pixelation Strength",
        &mRenderer3D.mPixelater.pixelationStrength,
        1.0f, 5.0f);

      static float bloomIntensity = 0.024f;
      ImGui::SliderFloat(
        "Bloom Intensity",
        &bloomIntensity,
        0.0f, 1.0f);

      mRenderer3D.mBloomRenderer.mProperties.intensity =
        glm::vec4(bloomIntensity);

      ImGui::SliderFloat(
        "Threshold",
        &mRenderer3D.mBloomRenderer.mProperties.threshold,
        0.0f, 1.0f);

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Water", ImGuiTreeNodeFlags_SpanFullWidth)) {
      ImGui::ColorEdit3(
        "Water Surface Color",
        &boundScene->lighting.data.waterSurfaceColor.r);

      ImGui::SliderFloat(
        "Water Roughness",
        &boundScene->lighting.data.waterRoughness,
        0.0f, 1.0f);

      ImGui::SliderFloat(
        "Water Metalness",
        &boundScene->lighting.data.waterMetal,
        0.0f, 1.0f);

      ImGui::SliderFloat(
        "Wave Strength",
        &boundScene->lighting.data.waveStrength,
        0.0f, 1.0f);

      for (int i = 0; i < 4; ++i) {
        char buffer[] = "Wave Profile X";
        buffer[13] = (char)i + '0';

        if (ImGui::TreeNodeEx(
              buffer,
              ImGuiTreeNodeFlags_SpanFullWidth)) {
          auto &waveProfile =
            boundScene->lighting.data.waveProfiles[i];

          ImGui::SliderFloat(
            "Zoom", &waveProfile.zoom, 0.0f, 0.05f);
          ImGui::SliderFloat(
            "Speed", &waveProfile.displacementSpeed, 0.0f, 2.0f);
          ImGui::SliderFloat(
            "Strength", &waveProfile.strength, 0.0f, 5.0f);

          ImGui::TreePop();
        }
      }

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Day/Night", ImGuiTreeNodeFlags_SpanFullWidth)) {
      const char *times[] = {
        "Sunset", "Midday", "Midnight", "Sunrise"
      };

      using Graphics::LightingProperties;

      LightingProperties::FastForwardDst timesEnum[] = {
        LightingProperties::FastForwardDst::Sunset,
        LightingProperties::FastForwardDst::Midday,
        LightingProperties::FastForwardDst::Midnight,
        LightingProperties::FastForwardDst::Sunrise,
      };

      static int currentItem = 0;

      if (ImGui::Combo(
            "Jump to", &currentItem, times, sizeof(times) / sizeof(times[0]))) {
        boundScene->lighting.fastForwardTo(timesEnum[currentItem]);
      }

      if (ImGui::Checkbox(
            "Pause Day/Night Cycle",
            &boundScene->lighting.pause)) {
      
      }

      ImGui::TreePop();
    }
  }

  ImGui::End();
}

}
