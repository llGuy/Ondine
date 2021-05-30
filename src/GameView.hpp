#pragma once

#include "View.hpp"
#include "RenderStage.hpp"
#include "DelegateResize.hpp"

namespace Yona {

class GameView : public View {
public:
  GameView(
    const RenderStage &gameRenderStage,
    DelegateResize &delegateResize3D);
  ~GameView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  const VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Event *ev);
  void processInputEvent(Event *ev);

private:
  const RenderStage &mGameRenderStage;
  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
};

}
