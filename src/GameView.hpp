#pragma once

#include "View.hpp"
#include "Delegate.hpp"
#include "RenderStage.hpp"

namespace Ondine {

/* Forwards events to the game state */
class GameView : public View {
public:
  GameView(
    const RenderStage &gameRenderStage,
    DelegateResize &delegateResize3D,
    DelegateTrackInput &delegateTrackInput,
    OnEventProc proc);
  ~GameView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Tick &tick, const InputTracker &tracker) override;

  const VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Event *ev);
  void processInputEvent(Event *ev);

private:
  const RenderStage &mGameRenderStage;
  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  DelegateTrackInput &mDelegateTrackInput;
  OnEventProc mOnEvent;
};

}
