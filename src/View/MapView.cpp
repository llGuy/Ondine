#include "IOEvent.hpp"
#include "MapView.hpp"
#include "Renderer3D.hpp"
#include "EditorEvent.hpp"
#include "GraphicsEvent.hpp"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::View {

MapView::MapView(
  Graphics::Renderer3D &renderer,
  Core::OnEventProc proc)
  : mMainRenderStage(renderer.mainRenderStage()),
    mDelegateResize3D(renderer),
    mOnEvent(proc),
    mTerrainTool(Core::TerrainTool::DensityPaintBrushAdd) {
#if 0
  auto *cursorChange = lnEmplaceAlloc<Core::EventCursorDisplayChange>();
  cursorChange->show = false;
  mOnEvent(cursorChange);

  mMapScene = renderer.createScene();
  mMapScene->debug.renderChunkOutlines = 0;
  mMapScene->debug.wireframeTerrain = 0;
  mMapScene->debug.renderQuadTree = 0;
  mMapScene->terrain.init();

  /*
  mMapScene->terrain.makeSphere(500.0f, glm::vec3(000.0f, 580.0f, 100.0f));
  mMapScene->terrain.makeSphere(250.0f, glm::vec3(-350.0f, 380.0f, 0.0f));

  mMapScene->terrain.makeSphere(500.0f, glm::vec3(1000.0f, 580.0f, 1100.0f));
  */
  
  mMapScene->terrain.makeIslands(
    100, 5, 0.1f, 1.4f, 20.0f, 0.8f,
    glm::ivec2(-250, -250) * 7,
    glm::ivec2(250, 250) * 7);

  // mMapScene->terrain.generateVoxelNormals();

  { // Set up scene objects
    auto handle1 = mMapScene->createSceneObject("TaurusModelRenderMethod"); 
    auto &sceneObj1 = mMapScene->getSceneObject(handle1);
    sceneObj1.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f) * 2.0f;
    sceneObj1.position = glm::vec3(1051.0f, 130.0f, 605.0f);
    sceneObj1.scale = glm::vec3(10.0f);
    sceneObj1.rotation = glm::angleAxis(
      glm::radians(30.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    sceneObj1.constructTransform();

    auto handle2 = mMapScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj2 = mMapScene->getSceneObject(handle2);
    sceneObj2.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj2.position = glm::vec3(1081.0f, 150.0f, 605.0f);
    sceneObj2.scale = glm::vec3(5.0f);
    sceneObj2.rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    sceneObj2.constructTransform();

    auto handle3 = mMapScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj3 = mMapScene->getSceneObject(handle3);
    sceneObj3.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj3.position = glm::vec3(1051.0f, 105.0f, 575.0f);
    sceneObj3.scale = glm::vec3(5.0f);
    sceneObj3.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    sceneObj3.constructTransform();

    auto handle4 = mMapScene->createSceneObject("SphereModelRenderMethod"); 
    auto &sceneObj4 = mMapScene->getSceneObject(handle4);
    sceneObj4.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
    sceneObj4.position = glm::vec3(951.0f, 90.0f, 705.0f);
    sceneObj4.scale = glm::vec3(20.0f);
    sceneObj4.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    sceneObj4.constructTransform();
  }

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
    mMapScene->lighting.data.waveProfiles[3] = {0.001f, 0.5f, 4.24f};
    mMapScene->lighting.rotationAngle = glm::radians(89.5f);
  }

  { // Set camera properties
    mMapScene->camera.fov = glm::radians(50.0f);
    mMapScene->camera.wPosition = glm::vec3(969.0f, 136.0f, 480.0f);
    mMapScene->camera.wViewDirection =
      glm::normalize(glm::vec3(0.415, -0.123f, 0.9f));
    mMapScene->camera.wUp = glm::vec3(0.0f, 1.0f, 0.0f);
  }
#endif
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

    case Core::EventCategory::Editor: {
      processEditorEvent(ev);
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

void MapView::processEditorEvent(Core::Event *ev) {
  // Forward events to game state and game renderer
  // Tell the game to tick
  switch (ev->type) {
  case Core::EventType::TerrainToolChange: {
    auto *toolChange = (Core::EventTerrainToolChange *)ev;
    mTerrainTool = toolChange->terrainTool;
    toolChange->isHandled = true;
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
    switch (mTerrainTool) {
    case Core::TerrainTool::DensityPaintBrushAdd: {
      mMapScene->terrain.queuePaint(
        mMapScene->camera.wPosition,
        mMapScene->camera.wViewDirection,
        180.0f,
        tick.dt);
    } break;

    case Core::TerrainTool::DensityPaintBrushDestroy: {
      mMapScene->terrain.queuePaint(
        mMapScene->camera.wPosition,
        mMapScene->camera.wViewDirection,
        180.0f,
        -tick.dt);
    } break;

    case Core::TerrainTool::ColorPaintBrush: {
      
    } break;
    }
  }
}

}
