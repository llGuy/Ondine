#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

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
    case EventCategory::Graphics: {
      processGraphicsEvent(ev);
    } break;

    case EventCategory::Input: {
      processInputEvent(ev);
    } break;

    default:;
    }
  });
}

void GameView::processGraphicsEvent(Event *ev) {
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

void GameView::processInputEvent(Event *ev) {
  // Forward events to game state and game renderer
  // Tell the game to tick
  switch (ev->type) {
  case EventType::Resize: {
    auto *resizeEvent = (EventResize *)ev;
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
