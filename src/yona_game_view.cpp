#include "yona_game_view.hpp"
#include "yona_event_renderer.hpp"

namespace Yona {

GameView::GameView(
  const RenderStage &gameRenderStage,
  DelegateResize &delegateResize3D)
  : mGameRenderStage(gameRenderStage),
    mDelegateResize3D(delegateResize3D) {
  
}

GameView::~GameView() {
  
}

void GameView::processEvents(ViewProcessEventsParams &params) {
  params.queue.process([this](Event *ev) {
    switch (ev->category) {
    case EventCategory::Renderer3D: {
      processRendererEvent(ev);
    } break;

    default:;
    }
  });
}

void GameView::processRendererEvent(Event *ev) {
  // Forward events to game state and game renderer
  // Tell the game to tick
  switch (ev->type) {
  case EventType::ViewportResize: {
    auto *resizeEvent = (EventViewportResize *)ev;
    mDelegateResize3D.resize(resizeEvent->newResolution);
    resizeEvent->isHandled = true;
  } break;

  default:;
  }
}

void GameView::render(ViewRenderParams &params) {

}

const VulkanUniform &GameView::getOutput() const {
  return mGameRenderStage.uniform();
}

}
