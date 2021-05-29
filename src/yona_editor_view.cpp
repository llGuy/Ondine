#include <imgui.h>
#include "yona_utils.hpp"
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "yona_editor_view.hpp"
#include "yona_vulkan_context.hpp"

namespace Yona {

EditorView::EditorView(
  const WindowContextInfo &contextInfo,
  VulkanContext &graphicsContext)
  : mIsDockLayoutInitialised(false) {
  initRenderTarget(graphicsContext);
  initImguiContext(contextInfo, graphicsContext);
}

void EditorView::processEvents(ViewProcessEventsParams &params) {
  
}

void EditorView::render(ViewRenderParams &params) {
  params.frame.primaryCommandBuffer.beginRenderPass(
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
        mDock, ImGuiDir_Down, 0.15f, NULL, &top);

      ImGuiID console;
      ImGuiID assets = ImGui::DockBuilderSplitNode(
        bottom, ImGuiDir_Left, 0.5f, NULL, &console);

      ImGuiID viewport;
      ImGuiID general = ImGui::DockBuilderSplitNode(
        top, ImGuiDir_Left, 0.15f, NULL, &viewport);

      ImGuiID game = ImGui::DockBuilderSplitNode(
        general, ImGuiDir_Down, 0.5f, NULL, &general);

      ImGui::DockBuilderDockWindow("Assets", assets);
      ImGui::DockBuilderDockWindow("Viewport", viewport);
      ImGui::DockBuilderDockWindow("Console", console);
      ImGui::DockBuilderDockWindow("Game State", game);
      ImGui::DockBuilderDockWindow("General", general);
    }

    tickMenuBar();

    ImGui::DockSpace(
      dock, ImVec2(0.0f, 0.0f),
      ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
  }

  ImGui::Begin(
    "Assets", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  ImGui::End();

  ImGui::Begin(
    "Viewport", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  ImGui::End();

  ImGui::Begin(
    "Console", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  ImGui::End();

  ImGui::Begin(
    "Game State", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  ImGui::End();

  ImGui::Begin(
    "General", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();

  params.graphicsContext.imgui().endRender(params.frame);

  params.frame.primaryCommandBuffer.endRenderPass();
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
      if (ImGui::MenuItem("Game")) {
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void EditorView::initRenderTarget(VulkanContext &graphicsContext) {
  auto properties = graphicsContext.getProperties();

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
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
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

}
