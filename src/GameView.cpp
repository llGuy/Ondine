#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

GameView::GameView(
  const Graphics::RenderStage &gameRenderStage,
  DelegateResize &delegateResize3D,
  DelegateTrackInput &delegateTrackInput,
  Core::OnEventProc proc)
  : mGameRenderStage(gameRenderStage),
    mDelegateResize3D(delegateResize3D),
    mDelegateTrackInput(delegateTrackInput),
    mOnEvent(proc) {
  
}

GameView::~GameView() {
  
}

void GameView::processEvents(ViewProcessEventsParams &params) {
  params.queue.process([this](Core::Event *ev) {
    switch (ev->category) {
    case Core::EventCategory::Graphics: {
      processGraphicsEvent(ev);
    } break;

    case Core::EventCategory::Input: {
      processInputEvent(ev);
    } break;

    default:;
    }
  });
}

void GameView::processGraphicsEvent(Core::Event *ev) {
  // Forward events to game state and game renderer
  // Tell the game to tick
  switch (ev->type) {
  case Core::EventType::ViewportResize: {
    auto *resizeEvent = (Core::EventViewportResize *)ev;
    mDelegateResize3D.resize(resizeEvent->newResolution);
    resizeEvent->isHandled = true;
  } break;

  default:;
  }
}

void GameView::processInputEvent(Core::Event *ev) {
  // Forward events to game state and game renderer
  // Tell the game to tick
  switch (ev->type) {
  case Core::EventType::Resize: {
    auto *resizeEvent = (Core::EventResize *)ev;
    mDelegateResize3D.resize(resizeEvent->newResolution);
    resizeEvent->isHandled = true;
  } break;

  default:;
  }
}

void GameView::render(ViewRenderParams &params) {
  /* Doesn't actually render anything */
}

const Graphics::VulkanUniform &GameView::getOutput() const {
  return mGameRenderStage.uniform();
}

FocusedView GameView::trackInput(
  const Core::Tick &tick, const Core::InputTracker &tracker) {
  mDelegateTrackInput.trackInput(tick, tracker);

  if (tracker.key(Core::KeyboardButton::Escape).didInstant) {
    auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
    cursorChange->show = true;
    mOnEvent(cursorChange);

    return FocusedView::Previous;
  }
  else {
    return FocusedView::Current;
  }
}

}
