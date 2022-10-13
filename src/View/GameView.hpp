#pragma once

#include "View.hpp"
#include "Game.hpp"
#include "Scene.hpp"
#include "Delegate.hpp"
#include "Renderer3D.hpp"
#include "RenderStage.hpp"

namespace Ondine::View {

/* Forwards events to the game state */
class GameView : public View {
public:
  GameView(
    Game::Game &game,
    Graphics::Renderer3D &renderer,
    Core::OnEventProc proc);

  ~GameView() override;

  void onPush(ViewPushParams &params) override;

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

  Game::Game &mGame;

  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  Core::OnEventProc mOnEvent;
};

}
