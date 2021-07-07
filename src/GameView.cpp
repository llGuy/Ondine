#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

GameView::GameView(
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mGameRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mDelegateTrackInput(renderer),
    mOnEvent(proc) {
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = false;
  mOnEvent(cursorChange);

  mGameScene = renderer.createScene();
  renderer.bindScene(mGameScene);
  
  auto handle1 = mGameScene->createSceneObject("TaurusModelRenderMethod"); 
  auto &sceneObj1 = mGameScene->getSceneObject(handle1);
  sceneObj1.position = glm::vec3(0.0f, 80.0f, 0.0f);
  sceneObj1.scale = glm::vec3(10.0f);
  sceneObj1.rotation = glm::angleAxis(
    glm::radians(30.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
  sceneObj1.constructTransform();

  auto handle2 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
  auto &sceneObj2 = mGameScene->getSceneObject(handle2);
  sceneObj2.position = glm::vec3(30.0f, 100.0f, 0.0f);
  sceneObj2.scale = glm::vec3(5.0f);
  sceneObj2.rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  sceneObj2.constructTransform();

  auto handle3 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
  auto &sceneObj3 = mGameScene->getSceneObject(handle3);
  sceneObj3.position = glm::vec3(0.0f, 55.0f, -30.0f);
  sceneObj3.scale = glm::vec3(5.0f);
  sceneObj3.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  sceneObj3.constructTransform();

  auto handle4 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
  auto &sceneObj4 = mGameScene->getSceneObject(handle4);
  sceneObj4.position = glm::vec3(-100.0f, 40.0f, 100.0f);
  sceneObj4.scale = glm::vec3(20.0f);
  sceneObj4.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  sceneObj4.constructTransform();
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

void GameView::processGameInput(
  const Core::Tick &tick, const Core::InputTracker &tracker) {
  
}

}
