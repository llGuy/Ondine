#pragma once

#include <imgui.h>
#include "yona_view.hpp"

namespace Yona {

class EditorView : public View {
public:
  EditorView();

  void processEvents(EventQueue &queue, const Tick &tick) override;

  void render(
    const VulkanUniform &previousOutput,
    const VulkanFrame &frame, const Tick &tick) override;

  const VulkanUniform &getOutput() const override;

private:
  void tickMenuBar();

private:
  bool mIsDockLayoutInitialised;
  ImGuiID mDock;
};

}
