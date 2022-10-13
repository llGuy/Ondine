#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

GameView::GameView(
  Game::Game &game,
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mGameRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mGame(game),
    mOnEvent(proc) {
#if 0
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = false;
  mOnEvent(cursorChange);

  mGameScene = renderer.createScene();
  mGameScene->terrain.init();
  renderer.bindScene(mGameScene);
  
  { // Set up scene objects
    auto handle5 = mGameScene->createSceneObject("GlowingTaurusRenderMethod"); 
    auto &sceneObj5 = mGameScene->getSceneObject(handle5);
    sceneObj5.pushConstant.color = glm::vec3(1.8f, 0.9f, 2.85f) * 1.0f;
    sceneObj5.position = glm::vec3(0.0f, 240.0f, 0.0f);
    sceneObj5.scale = glm::vec3(10.0f);
    sceneObj5.rotation = glm::angleAxis(
      glm::radians(30.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    sceneObj5.constructTransform();
#endif

#if 0
    mRotAngle = 0.0f;

    auto handle1 = mGameScene->createSceneObject("CubeModelRenderMethod"); 
    auto &sceneObj1 = mGameScene->getSceneObject(handle1);
    sceneObj1.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj1.position = glm::vec3(0.0f, 140.0f, 0.0f);
    sceneObj1.scale = glm::vec3(10.0f);
    sceneObj1.rotation = glm::angleAxis(
      glm::radians(30.0f), glm::vec3(-1.0f, -0.3f, 0.0f));
    sceneObj1.constructTransform();
    mCubeHandle = handle1;

    auto handle2 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj2 = mGameScene->getSceneObject(handle2);
    sceneObj2.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj2.position = glm::vec3(30.0f, 150.0f, 0.0f);
    sceneObj2.scale = glm::vec3(5.0f);
    sceneObj2.rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    sceneObj2.constructTransform();

    auto handle3 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj3 = mGameScene->getSceneObject(handle3);
    sceneObj3.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj3.position = glm::vec3(0.0f, 105.0f, -30.0f);
    sceneObj3.scale = glm::vec3(5.0f);
    sceneObj3.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    sceneObj3.constructTransform();

    auto handle4 = mGameScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj4 = mGameScene->getSceneObject(handle4);
    sceneObj4.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj4.position = glm::vec3(-100.0f, 160.0f, 100.0f);
    sceneObj4.scale = glm::vec3(20.0f);
    sceneObj4.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    sceneObj4.constructTransform();
  }
#endif

#if 0
  { // Set lighting properties
    mGameScene->lighting.data.sunDirection =
      glm::normalize(glm::vec3(0.000001f, 0.1f, -1.00001f));
    mGameScene->lighting.data.moonDirection =
      glm::normalize(glm::vec3(1.000001f, 1.6f, +1.00001f));
    mGameScene->lighting.data.sunSize = glm::vec3(
      0.0046750340586467079f, 0.99998907220740285f, 0.0f);
    mGameScene->lighting.data.exposure = 20.0f;
    mGameScene->lighting.data.white = glm::vec3(2.0f);
    mGameScene->lighting.data.waterSurfaceColor =
      glm::vec3(8.0f, 54.0f, 76.0f) / 255.0f;
    mGameScene->lighting.pause = false;
    mGameScene->lighting.data.continuous = 0.0f;
    mGameScene->lighting.data.waveStrength = 0.54f;
    mGameScene->lighting.data.waterRoughness = 0.01f;
    // mMapScene->lighting.data.waterMetal = 0.7f;
    mGameScene->lighting.data.waterMetal = 0.95f;
    mGameScene->lighting.data.waveProfiles[0] = {0.01f, 1.0f, 0.75f};
    mGameScene->lighting.data.waveProfiles[1] = {0.005f, 1.0f, 1.0f};
    mGameScene->lighting.data.waveProfiles[2] = {0.008f, 0.3f, 1.668f};
    mGameScene->lighting.data.waveProfiles[3] = {0.001f, 0.5f, 4.24f};
    mGameScene->lighting.rotationAngle = glm::radians(86.5f);
  }
#endif

#if 0 
  { // Set camera properties
    mGameScene->camera.fov = glm::radians(50.0f);
    mGameScene->camera.wPosition = glm::vec3(100.0f, 140.0f, -180.0f);
    mGameScene->camera.wViewDirection =
      glm::normalize(glm::vec3(-0.3f, -0.1f, 1.0f));
    mGameScene->camera.wUp = glm::vec3(0.0f, 1.0f, 0.0f);
  }
#endif
}

void GameView::onPush(ViewPushParams &params) {
  params.renderer.bindScene(mGame.getScene());

  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = false;
  mOnEvent(cursorChange);
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
  mGame.trackInput(tick, tracker);

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
