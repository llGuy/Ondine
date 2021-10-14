#include "IOEvent.hpp"
#include "DemoView.hpp"
#include "GraphicsEvent.hpp"

namespace Ondine::View {

DemoView::DemoView(
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mGameRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mRenderer3D(renderer),
    mOnEvent(proc) {
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = true;
  mOnEvent(cursorChange);

  mDemoScene = renderer.createScene();
  mDemoScene->terrain.init();
  mDemoScene->particleDemo.init(10000, renderer.mGBuffer, renderer.mGraphicsContext);
  renderer.bindScene(mDemoScene);
  
  { // Set lighting properties
    mDemoScene->lighting.data.sunDirection =
      glm::normalize(glm::vec3(0.000001f, 0.1f, -1.00001f));
    mDemoScene->lighting.data.moonDirection =
      glm::normalize(glm::vec3(1.000001f, 1.6f, +1.00001f));
    mDemoScene->lighting.data.sunSize = glm::vec3(
      0.0046750340586467079f, 0.99998907220740285f, 0.0f);
    mDemoScene->lighting.data.exposure = 20.0f;
    mDemoScene->lighting.data.white = glm::vec3(2.0f);
    mDemoScene->lighting.data.waterSurfaceColor =
      glm::vec3(8.0f, 54.0f, 76.0f) / 255.0f;
    mDemoScene->lighting.pause = false;
    mDemoScene->lighting.data.continuous = 0.0f;
    mDemoScene->lighting.data.waveStrength = 0.54f;
    mDemoScene->lighting.data.waterRoughness = 0.01f;
    // mMapScene->lighting.data.waterMetal = 0.7f;
    mDemoScene->lighting.data.waterMetal = 0.95f;
    mDemoScene->lighting.data.waveProfiles[0] = {0.01f, 1.0f, 0.75f};
    mDemoScene->lighting.data.waveProfiles[1] = {0.005f, 1.0f, 1.0f};
    mDemoScene->lighting.data.waveProfiles[2] = {0.008f, 0.3f, 1.668f};
    mDemoScene->lighting.data.waveProfiles[3] = {0.001f, 0.5f, 4.24f};
    mDemoScene->lighting.data.enableLighting = false;
    mDemoScene->lighting.rotationAngle = glm::radians(86.5f);

    mDemoScene->debug.enableParticleDemo = 1;
  }

  { // Set camera properties
    mDemoScene->camera.fov = glm::radians(50.0f);
    mDemoScene->camera.wPosition = glm::vec3(100.0f, 140.0f, -180.0f);
    mDemoScene->camera.wViewDirection =
      glm::normalize(glm::vec3(-0.3f, -0.1f, 1.0f));
    mDemoScene->camera.wUp = glm::vec3(0.0f, 1.0f, 0.0f);
  }
}

void DemoView::onPush(ViewPushParams &params) {
  params.renderer.bindScene(mDemoScene);

  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = true;
  mOnEvent(cursorChange);
}

void DemoView::onPop(ViewPushParams &params) {

}

DemoView::~DemoView() {
  
}

void DemoView::processEvents(ViewProcessEventsParams &params) {
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

void DemoView::processGraphicsEvent(Core::Event *ev) {
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

void DemoView::processInputEvent(Core::Event *ev) {
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

void DemoView::render(ViewRenderParams &params) {
  /* Doesn't actually render anything */
}

const Graphics::VulkanUniform &DemoView::getOutput() const {
  return mGameRenderStage.uniform();
}

FocusedView DemoView::trackInput(
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

void DemoView::processGameInput(
  const Core::Tick &tick, const Core::InputTracker &inputTracker) {
  auto up = mDemoScene->camera.wUp;
  auto right = glm::normalize(glm::cross(
    mDemoScene->camera.wViewDirection, mDemoScene->camera.wUp));
  auto forward = glm::normalize(glm::cross(up, right));

  float speedMultiplier = 30.0f;
  if (inputTracker.key(Core::KeyboardButton::R).isDown) {
    speedMultiplier *= 10.0f;
  }

  if (inputTracker.key(Core::KeyboardButton::W).isDown) {
    mDemoScene->camera.wPosition +=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::A).isDown) {
    mDemoScene->camera.wPosition -=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::S).isDown) {
    mDemoScene->camera.wPosition -=
      forward * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::D).isDown) {
    mDemoScene->camera.wPosition +=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::Space).isDown) {
    mDemoScene->camera.wPosition +=
      mDemoScene->camera.wUp * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::LeftShift).isDown) {
    mDemoScene->camera.wPosition -=
      mDemoScene->camera.wUp * tick.dt * speedMultiplier;
  }

  const auto &cursor = inputTracker.cursor();

  if (inputTracker.mouseButton(Core::MouseButton::Left).didRelease) {
    glm::vec2 position = (glm::vec2)cursor.cursorPos /
      glm::vec2(mRenderer3D.pipelineViewport.width,
                mRenderer3D.pipelineViewport.height);

    position.y = 1.0f - position.y;

    position = position * 2.0f - glm::vec2(1.0f);

    mDemoScene->particleDemo.mCircles.add(
      glm::vec2(position));
  }
  
  if (cursor.didCursorMove) {
    static constexpr float SENSITIVITY = 15.0f;

    auto delta = glm::vec2(cursor.cursorPos) - glm::vec2(cursor.previousPos);
    auto res = mDemoScene->camera.wViewDirection;

    float xAngle = glm::radians(-delta.x) * SENSITIVITY * tick.dt;
    float yAngle = glm::radians(-delta.y) * SENSITIVITY * tick.dt;
                
    res = glm::mat3(glm::rotate(xAngle, mDemoScene->camera.wUp)) * res;
    auto rotateY = glm::cross(res, mDemoScene->camera.wUp);
    res = glm::mat3(glm::rotate(yAngle, rotateY)) * res;

    res = glm::normalize(res);

    mDemoScene->camera.wViewDirection = res;
  }

  if (cursor.didScroll) {
    mDemoScene->lighting.rotateBy(
      glm::radians(30.0f * cursor.scroll.y * tick.dt));
  }
}

}
