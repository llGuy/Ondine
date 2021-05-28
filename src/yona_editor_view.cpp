#include <imgui.h>
#include "yona_utils.hpp"
#include <imgui_internal.h>
#include "yona_editor_view.hpp"

namespace Yona {

EditorView::EditorView()
  : mIsDockLayoutInitialised(false) {
  
}

void EditorView::processEvents(EventQueue &queue, const Tick &tick) {
  
}

void EditorView::render(
  const VulkanUniform &previousOutput,
  const VulkanFrame &frame, const Tick &tick) {
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

  ImGui::Begin("Main Window", NULL, flags);

  mDock = ImGui::GetID("Main Dockspace");

  if (!mIsDockLayoutInitialised) {
    mIsDockLayoutInitialised = true;

    ImGui::DockBuilderRemoveNode(mDock);
    ImGui::DockBuilderAddNode(mDock, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(mDock, viewport->Size);

    ImGuiID left = ImGui::DockBuilderSplitNode(
      mDock, ImGuiDir_Left, 0.5f, NULL, &mDock);

    ImGuiID left_split = ImGui::DockBuilderSplitNode(
      left, ImGuiDir_Up, 0.7f, NULL, &left);

    ImGui::DockBuilderDockWindow("Assets", left_split);
    ImGui::DockBuilderDockWindow("Viewport", mDock);
    ImGui::DockBuilderDockWindow("Console", left);
    ImGui::DockBuilderDockWindow("Game", left);
  }

  ImGui::DockSpace(
    mDock, ImVec2(0.0f, 0.0f),
    ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

  tickMenuBar();

  ImGui::End();

  ImGui::Begin(
    "Assets", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
  
  ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);

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
    "Game", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);

  ImGui::End();


  ImGui::Render();
}

const VulkanUniform &EditorView::getOutput() const {
  return {};
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

}
