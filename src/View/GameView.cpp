#include "GameView.hpp"
#include "IOEvent.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

GameView::GameView(
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mGameRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mRenderer3D(renderer),
    mOnEvent(proc) {
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = true;
  mOnEvent(cursorChange);

  mGameScene = renderer.createScene();
  mGameScene->terrain.init();
  renderer.bindScene(mGameScene);
  
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

  { // Set camera properties
    mGameScene->camera.fov = glm::radians(50.0f);
    mGameScene->camera.wPosition = glm::vec3(100.0f, 140.0f, -180.0f);
    mGameScene->camera.wViewDirection =
      glm::normalize(glm::vec3(-0.3f, -0.1f, 1.0f));
    mGameScene->camera.wUp = glm::vec3(0.0f, 1.0f, 0.0f);
  }
}

void GameView::onPush(ViewPushParams &params) {
  params.renderer.bindScene(mGameScene);

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
  processGameInput(tick, tracker);

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
  const Core::Tick &tick, const Core::InputTracker &inputTracker) {
  auto up = mGameScene->camera.wUp;
  auto right = glm::normalize(glm::cross(
    mGameScene->camera.wViewDirection, mGameScene->camera.wUp));
  auto forward = glm::normalize(glm::cross(up, right));

  float speedMultiplier = 30.0f;
  if (inputTracker.key(Core::KeyboardButton::R).isDown) {
    speedMultiplier *= 10.0f;
  }

  if (inputTracker.key(Core::KeyboardButton::W).isDown) {
    mGameScene->camera.wPosition +=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::A).isDown) {
    mGameScene->camera.wPosition -=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::S).isDown) {
    mGameScene->camera.wPosition -=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::D).isDown) {
    mGameScene->camera.wPosition +=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::Space).isDown) {
    mGameScene->camera.wPosition +=
      mGameScene->camera.wUp * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::LeftShift).isDown) {
    mGameScene->camera.wPosition -=
      mGameScene->camera.wUp * tick.dt * speedMultiplier;
  }

  const auto &cursor = inputTracker.cursor();

  if (inputTracker.mouseButton(Core::MouseButton::Left).didRelease) {
    glm::vec2 position = (glm::vec2)cursor.cursorPos /
      glm::vec2(mRenderer3D.pipelineViewport.width,
                mRenderer3D.pipelineViewport.height);

    position.y = 1.0f - position.y;

    position = position * 2.0f - glm::vec2(1.0f);

    mRenderer3D.mCircles.add(
      Graphics::Renderer3D::Circle{position, 1.0f});
  }
  
  if (cursor.didCursorMove) {
    static constexpr float SENSITIVITY = 15.0f;

    auto delta = glm::vec2(cursor.cursorPos) - glm::vec2(cursor.previousPos);
    auto res = mGameScene->camera.wViewDirection;

    float xAngle = glm::radians(-delta.x) * SENSITIVITY * tick.dt;
    float yAngle = glm::radians(-delta.y) * SENSITIVITY * tick.dt;
                
    res = glm::mat3(glm::rotate(xAngle, mGameScene->camera.wUp)) * res;
    auto rotateY = glm::cross(res, mGameScene->camera.wUp);
    res = glm::mat3(glm::rotate(yAngle, rotateY)) * res;

    res = glm::normalize(res);

    mGameScene->camera.wViewDirection = res;
  }

  if (cursor.didScroll) {
    mGameScene->lighting.rotateBy(
      glm::radians(30.0f * cursor.scroll.y * tick.dt));
  }
}

}
