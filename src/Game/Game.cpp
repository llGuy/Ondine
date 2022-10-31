#include "Game.hpp"

namespace Ondine::Game {

void Game::init(const DelegateGeometryManager &geometryManager) {
  mGeometryManager = &geometryManager;

  mSimulation.init();
  mEntityCamera.init();
  mEntityController.init();

  mEntityCamera.setCameraType(EntityCameraType::FirstPerson);

  { // Create main entity
    auto [primary, id] = mSimulation.createEntity();
    primary.position = glm::vec3(100.0f, 140.0f, -180.0f);
    primary.viewDirection = glm::normalize(glm::vec3(-0.3f, 0.1f, 1.0f));

    mEntityCamera.getAttachedEntity() = id;
    mEntityController.getControlledEntity() = id;
  }

  { // Create test scene
    auto [floor, floorID] = mSimulation.createEntity();
    floor.position = { 0.0f, 140.0f, 0.0f };
    floor.scale    = { 200.0f, 0.5f, 200.0f };
    floor.rotation = glm::angleAxis(glm::radians(0.0f), kGlobalUp);
    mFloor = floorID;

    auto cubeID = mGeometryManager->getGeometryID("Cube");

    mRotation = 0.0f;

    auto [a, aID] = mSimulation.createEntity();
    a.position = { 30.0f, 160.0f, 0.0f };
    a.scale    = glm::vec3(10.0f);
    a.rotation = glm::angleAxis(glm::radians(mRotation), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
    a.velocity = glm::vec3(-10.0f, 0.0f, 0.0f);
    a.aabb = AABB::unitCube();
    a.geometryID = cubeID;
    mCubeA = aID;

    auto [b, bID] = mSimulation.createEntity();
    b.position = { -30.0f, 160.0f, 0.0f };
    b.scale    = glm::vec3(10.0f);
    b.rotation = glm::angleAxis(glm::radians(-20.0f), glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));
    b.aabb = AABB::unitCube();
    b.geometryID = cubeID;
    mCubeB = bID;
  }

  mSimulation.registerBehavior<PhysicsBehavior>(20);
  // mSimulation.attachBehavior<PhysicsBehavior>(mFloor);
  mSimulation.attachBehavior<PhysicsBehavior>(mCubeA);
  mSimulation.attachBehavior<PhysicsBehavior>(mCubeB);
}

void Game::initGameRendering(Graphics::Renderer3D &gfx) {
  mScene = gfx.createScene();

  // Initialize scene lighting
  mScene->lighting.data.sunDirection = glm::normalize(glm::vec3(0.000001f, 0.1f, -1.00001f));
  mScene->lighting.data.moonDirection = glm::normalize(glm::vec3(1.000001f, 1.6f, +1.00001f));
  mScene->lighting.data.sunSize = glm::vec3(0.0046750340586467079f, 0.99998907220740285f, 0.0f);
  mScene->lighting.data.exposure = 20.0f;
  mScene->lighting.data.white = glm::vec3(2.0f);
  mScene->lighting.data.waterSurfaceColor = glm::vec3(8.0f, 54.0f, 76.0f) / 255.0f;
  mScene->lighting.pause = false;
  mScene->lighting.data.continuous = 0.0f;
  mScene->lighting.data.waveStrength = 0.54f;
  mScene->lighting.data.waterRoughness = 0.01f;
  mScene->lighting.data.waterMetal = 0.95f;
  mScene->lighting.data.waveProfiles[0] = {0.01f, 1.0f, 0.75f};
  mScene->lighting.data.waveProfiles[1] = {0.005f, 1.0f, 1.0f};
  mScene->lighting.data.waveProfiles[2] = {0.008f, 0.3f, 1.668f};
  mScene->lighting.data.waveProfiles[3] = {0.001f, 0.5f, 4.24f};
  mScene->lighting.rotationAngle = glm::radians(86.5f);

  // makeEntityRenderable(mSimulation.getEntity(mFloor), "CubeModelRenderMethod");
  makeEntityRenderable(mSimulation.getEntity(mCubeA), "CubeModelRenderMethod");
  makeEntityRenderable(mSimulation.getEntity(mCubeB), "CubeModelRenderMethod");
}

void Game::tick(const Core::Tick &tick) {
  mSimulation.tick(tick, *mGeometryManager);

  mEntityCamera.tick(tick, mSimulation);

  Entity &a = mSimulation.getEntity(mCubeA);
  a.rotation = glm::angleAxis(glm::radians(mRotation), glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));

  Entity &b = mSimulation.getEntity(mCubeB);
  b.rotation = glm::angleAxis(glm::radians(-20.0f - mRotation * 0.5f), glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));

  mRotation += tick.dt * 90.0f;

  updateEntitySceneObject(mSimulation.getEntity(mCubeA));
  updateEntitySceneObject(mSimulation.getEntity(mCubeB));

  // Update camera information
  Entity &controlledEntity = mSimulation.getEntity(mEntityCamera.getAttachedEntity());
  mScene->camera.wPosition = controlledEntity.position;
  mScene->camera.wUp = kGlobalUp;
  mScene->camera.wViewDirection = controlledEntity.viewDirection;
  mScene->camera.fov = glm::radians(50.0f);
}

void Game::trackInput(
  const Core::Tick &tick,
  const Core::InputTracker &inputTracker) {
  auto gameInput = mInputTranslator.translate(inputTracker);
  mEntityController.tick(tick, mSimulation, gameInput);

  if (gameInput.bDidScroll) {
    mScene->lighting.rotateBy(glm::radians(30.0f * gameInput.wheelAmount * tick.dt));
  }
}

void Game::makeEntityRenderable(Entity &entity, const char *renderMethod) {
  auto handle = mScene->createSceneObject(renderMethod);
  auto &obj = mScene->getSceneObject(handle);
  obj.pushConstant.color = glm::vec3(0.8f, 0.9f, 0.85f);
  obj.position = entity.position;
  obj.scale = entity.scale;
  obj.rotation = entity.rotation;
  obj.constructTransform();

  entity.draw = handle;
}

void Game::updateEntitySceneObject(Entity &entity) {
  auto &object = mScene->getSceneObject(entity.draw);
  object.position = entity.position;
  object.scale = entity.scale;
  object.rotation = entity.rotation;
  object.constructTransform();
}

Game::Viewer Game::getViewerInfo() {
  auto &ent = mSimulation.getEntity(mEntityController.getControlledEntity());

  Game::Viewer viewer = {};
  viewer.wPosition = ent.position;
  viewer.wViewDirection = ent.viewDirection;
  viewer.wUp = kGlobalUp;

  return viewer;
}

Graphics::Scene *Game::getScene() {
  return mScene;
}

}
