#include "IOEvent.hpp"
#include "MapView.hpp"
#include "Renderer3D.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

MapView::MapView(
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mMainRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mOnEvent(proc) {
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = false;
  mOnEvent(cursorChange);

  mMapScene = renderer.createScene();
  mMapScene->terrain.init();
  // mMapScene->terrain.makeSphere(150.0f, glm::vec3(121.0f, 40.0f, 194.0f));
  mMapScene->terrain.makeIslands(
    50, 4, 0.1f, 1.4f, 20.0f, 1.0f,
    glm::ivec2(-250, -250) * 12,
    glm::ivec2(250, 250) * 12);
  mMapScene->terrain.prepareForRender(renderer.mGraphicsContext);

  renderer.bindScene(mMapScene);

  { // Set lighting properties
    mMapScene->lighting.data.sunDirection =
      glm::normalize(glm::vec3(0.000001f, 2.0f, 0.1f));
    mMapScene->lighting.data.moonDirection =
      glm::normalize(glm::vec3(1.000001f, 1.6f, +1.00001f));
    mMapScene->lighting.data.sunSize = glm::vec3(
      0.0046750340586467079f, 0.99998907220740285f, 0.0f);
    mMapScene->lighting.data.exposure = 20.0f;
    mMapScene->lighting.data.white = glm::vec3(2.0f);
    mMapScene->lighting.data.waterSurfaceColor =
      glm::vec3(8.0f, 54.0f, 76.0f) / 255.0f;
    mMapScene->lighting.pause = false;
    mMapScene->lighting.data.continuous = 0.0f;
    mMapScene->lighting.data.waveStrength = 0.54f;
    mMapScene->lighting.data.waterRoughness = 0.01f;
    // mMapScene->lighting.data.waterMetal = 0.7f;
    mMapScene->lighting.data.waterMetal = 0.95f;
    mMapScene->lighting.data.waveProfiles[0] = {0.01f, 1.0f, 0.75f};
    mMapScene->lighting.data.waveProfiles[1] = {0.005f, 1.0f, 1.0f};
    mMapScene->lighting.data.waveProfiles[2] = {0.008f, 0.3f, 1.668f};
    mMapScene->lighting.data.waveProfiles[3] = {0.001f, 0.5f, 3.64f};
    mMapScene->lighting.rotationAngle = glm::radians(86.5f);

  }

  { // Set camera properties
    mMapScene->camera.fov = glm::radians(50.0f);
    mMapScene->camera.wPosition = glm::vec3(0.0f, 90.0f, 0.0f);
    mMapScene->camera.wViewDirection =
      glm::normalize(glm::vec3(0.5f, -0.1f, 0.8f));
    mMapScene->camera.wUp = glm::vec3(0.0f, 1.0f, 0.0f);
  }
}

void MapView::onPush(ViewPushParams &params) {
  params.renderer.bindScene(mMapScene);
}

MapView::~MapView() {
  
}

void MapView::processEvents(ViewProcessEventsParams &params) {
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

void MapView::processGraphicsEvent(Core::Event *ev) {
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

void MapView::processInputEvent(Core::Event *ev) {
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

void MapView::render(ViewRenderParams &params) {
  /* Doesn't actually render anything */
}

const Graphics::VulkanUniform &MapView::getOutput() const {
  return mMainRenderStage.uniform();
}

FocusedView MapView::trackInput(
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

void MapView::processGameInput(
  const Core::Tick &tick, const Core::InputTracker &inputTracker) {
  auto up = mMapScene->camera.wUp;
  auto right = glm::normalize(glm::cross(
    mMapScene->camera.wViewDirection, mMapScene->camera.wUp));
  auto forward = glm::normalize(glm::cross(up, right));

  float speedMultiplier = 30.0f;
  if (inputTracker.key(Core::KeyboardButton::R).isDown) {
    speedMultiplier *= 10.0f;
  }

  if (inputTracker.key(Core::KeyboardButton::W).isDown) {
    mMapScene->camera.wPosition +=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::A).isDown) {
    mMapScene->camera.wPosition -=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::S).isDown) {
    mMapScene->camera.wPosition -=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::D).isDown) {
    mMapScene->camera.wPosition +=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::Space).isDown) {
    mMapScene->camera.wPosition +=
      mMapScene->camera.wUp * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::LeftShift).isDown) {
    mMapScene->camera.wPosition -=
      mMapScene->camera.wUp * tick.dt * speedMultiplier;
  }

  const auto &cursor = inputTracker.cursor();
  if (cursor.didCursorMove) {
    static constexpr float SENSITIVITY = 15.0f;

    auto delta = glm::vec2(cursor.cursorPos) - glm::vec2(cursor.previousPos);
    auto res = mMapScene->camera.wViewDirection;

    float xAngle = glm::radians(-delta.x) * SENSITIVITY * tick.dt;
    float yAngle = glm::radians(-delta.y) * SENSITIVITY * tick.dt;
                
    res = glm::mat3(glm::rotate(xAngle, mMapScene->camera.wUp)) * res;
    auto rotateY = glm::cross(res, mMapScene->camera.wUp);
    res = glm::mat3(glm::rotate(yAngle, rotateY)) * res;

    res = glm::normalize(res);

    mMapScene->camera.wViewDirection = res;
  }

  if (cursor.didScroll) {
    mMapScene->lighting.rotateBy(
      glm::radians(30.0f * cursor.scroll.y * tick.dt));
  }

  if (inputTracker.mouseButton(Core::MouseButton::Right).isDown) {
    // mMapScene->terrain.makeSphere(100.0f, mMapScene->camera.wPosition);
    mMapScene->terrain.paint(
      mMapScene->camera.wPosition,
      mMapScene->camera.wViewDirection,
      100.0f,
      0.1f);
  }
}

}
