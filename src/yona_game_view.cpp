#include "yona_game_view.hpp"
#include "yona_event_renderer.hpp"

namespace Yona {

GameView::GameView(
  const RenderStage &gameRenderStage,
  DelegateResize &delegateResize3D)
  : mGameRenderStage(gameRenderStage),
    mDelegateResize3D(delegateResize3D),
    mDeferredEventCount(0) {
  
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
    Resolution *resolution = lnEmplaceAlloc<Resolution>(
      resizeEvent->newResolution);

    mDeferredEvents[mDeferredEventCount++] = {
      handleResize,
      resolution
    };

    resizeEvent->isHandled = true;
  } break;

  default:;
  }
}

void GameView::processDeferredEvents(VulkanContext &graphicsContext) {
  DeferredEventProcParams params = {
    this, graphicsContext, nullptr
  };

  for (int i = 0; i < mDeferredEventCount; ++i) {
    params.data = mDeferredEvents[i].data;
    mDeferredEvents[i].proc(params);
  }

  mDeferredEventCount = 0;
}

void GameView::render(ViewRenderParams &params) {
  // Doesn't need to do any rendering
  processDeferredEvents(params.graphicsContext);
}

const VulkanUniform &GameView::getOutput() const {
  return mGameRenderStage.uniform();
}

void GameView::handleResize(DeferredEventProcParams &params) {
  params.gameView->mDelegateResize3D.resize(
    params.graphicsContext, *(Resolution *)params.data);
}

}
