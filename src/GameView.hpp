#pragma once

#include "View.hpp"
#include "Delegate.hpp"
#include "RenderStage.hpp"

namespace Ondine::View {

/* Forwards events to the game state */
class GameView : public View {
public:
  GameView(
    const Graphics::RenderStage &gameRenderStage,
    DelegateResize &delegateResize3D,
    DelegateTrackInput &delegateTrackInput,
    Core::OnEventProc proc);
  ~GameView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Core::Tick &tick, const Core::InputTracker &tracker) override;

  const Graphics::VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Core::Event *ev);
  void processInputEvent(Core::Event *ev);

private:
  const Graphics::RenderStage &mGameRenderStage;
  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  DelegateTrackInput &mDelegateTrackInput;
  Core::OnEventProc mOnEvent;
};

}
