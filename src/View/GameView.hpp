#pragma once

#include "View.hpp"
#include "Scene.hpp"
#include "Renderer3D.hpp"
#include "RenderStage.hpp"

namespace Ondine::View {

/* Forwards events to the game state */
class GameView : public View {
public:
  GameView(
    Graphics::Renderer3D &renderer,
    Core::OnEventProc proc);

  ~GameView() override;

  void onPush(ViewPushParams &params) override;
  void onPop(ViewPushParams &params) override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Core::Tick &tick, const Core::InputTracker &tracker) override;

  const Graphics::VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Core::Event *ev);
  void processInputEvent(Core::Event *ev);

  // For now process game input directly in the game view
  void processGameInput(
    const Core::Tick &tick, const Core::InputTracker &tracker);

private:
  Graphics::Scene *mGameScene;

  const Graphics::RenderStage &mGameRenderStage;

  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  Core::OnEventProc mOnEvent;
};

}
