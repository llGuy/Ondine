#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine {

GameView::GameView(
  const RenderStage &gameRenderStage,
  DelegateResize &delegateResize3D,
  DelegateTrackInput &delegateTrackInput,
  OnEventProc proc)
  : mGameRenderStage(gameRenderStage),
    mDelegateResize3D(delegateResize3D),
    mDelegateTrackInput(delegateTrackInput),
    mOnEvent(proc) {
  
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
  /* Doesn't actually render anything */
}

const VulkanUniform &GameView::getOutput() const {
  return mGameRenderStage.uniform();
}

FocusedView GameView::trackInput(
  const Tick &tick, const InputTracker &tracker) {
  mDelegateTrackInput.trackInput(tick, tracker);

  if (tracker.key(KeyboardButton::Escape).didInstant) {
    auto *cursorChange = lnEmplaceAlloc<EventCursorDisplayChange>();
    cursorChange->show = true;
    mOnEvent(cursorChange);

    return FocusedView::Previous;
  }
  else {
    return FocusedView::Current;
  }
}

}
