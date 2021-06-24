#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "Application.hpp"
#include "RendererCache.hpp"
#include "VulkanRenderPass.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {


Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext),
    mScene(mModelManager) {
  
}

void Renderer3D::init() {
  auto properties = mGraphicsContext.getProperties();
  mPipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };

  mModelManager.init();

  mGBuffer.init(
    mGraphicsContext, {mPipelineViewport.width, mPipelineViewport.height});

  mSkyRenderer.init(mGraphicsContext, mGBuffer);

  // Idle with all precomputation stuff
  mGraphicsContext.device().graphicsQueue().idle();

  { // Set planet properties
    /* Temporary - the world needs to define this */
    mPlanetProperties.solarIrradiance = glm::vec3(1.474f, 1.8504f, 1.91198f);
    // Angular radius of the Sun (radians)
    mPlanetProperties.solarAngularRadius = 0.004695f;
    mPlanetProperties.bottomRadius = 6360.0f / 6.0f;
    mPlanetProperties.topRadius = 6420.0f / 6.0f;

    mPlanetProperties.rayleighDensity.layers[0] =
      DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    mPlanetProperties.rayleighDensity.layers[1] =
      DensityLayer { 0.0f, 1.0f, -0.125f, 0.0f, 0.0f };
    mPlanetProperties.rayleighScatteringCoef =
      glm::vec3(0.005802f, 0.013558f, 0.033100f);

    mPlanetProperties.mieDensity.layers[0] =
      DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    mPlanetProperties.mieDensity.layers[1] =
      DensityLayer { 0.0f, 1.0f, -0.833333f, 0.0f, 0.0f };
    mPlanetProperties.mieScatteringCoef = glm::vec3(
      0.003996f, 0.003996f, 0.003996f);
    mPlanetProperties.mieExtinctionCoef = glm::vec3(
      0.004440f, 0.004440f, 0.004440f);

    mPlanetProperties.miePhaseFunctionG = 0.8f;

    mPlanetProperties.absorptionDensity.layers[0] =
      DensityLayer { 25.000000f, 0.000000f, 0.000000f, 0.066667f, -0.666667f };
    mPlanetProperties.absorptionDensity.layers[1] =
      DensityLayer { 0.000000f, 0.000000f, 0.000000f, -0.066667f, 2.666667f };
    mPlanetProperties.absorptionExtinctionCoef =
      glm::vec3(0.000650f, 0.001881f, 0.000085f);
    mPlanetProperties.groundAlbedo = glm::vec3(0.05f, 0.05f, 0.05f);
    mPlanetProperties.muSunMin = -0.207912f;
    mPlanetProperties.wPlanetCenter =
      glm::vec3(0.0f, -mPlanetProperties.bottomRadius, 0.0f);

    mPlanetRenderer.init(mGraphicsContext, mGBuffer, &mPlanetProperties);
  }

  { // Set camera properties
    mCameraProperties.fov = glm::radians(50.0f);
    mCameraProperties.aspectRatio =
      (float)mPipelineViewport.width / (float)mPipelineViewport.height;
    mCameraProperties.near = 0.1f;
    mCameraProperties.far = 10000.0f;

    mCameraProperties.projection = glm::perspective(
      mCameraProperties.fov,
      mCameraProperties.aspectRatio,
      mCameraProperties.near,
      mCameraProperties.far);

    mCameraProperties.wPosition = glm::vec3(-50.0f, 90.0f, 180.0f);
    mCameraProperties.wViewDirection =
      glm::normalize(glm::vec3(0.3f, -0.1f, -1.0f));
    mCameraProperties.wUp = glm::vec3(0.0f, 1.0f, 0.0f);

    mCameraProperties.view = glm::lookAt(
      mCameraProperties.wPosition,
      mCameraProperties.wPosition + mCameraProperties.wViewDirection,
      mCameraProperties.wUp);

    mCameraProperties.inverseProjection = glm::inverse(
      mCameraProperties.projection);

    mCameraProperties.inverseView = glm::inverse(
      mCameraProperties.view);

    mCameraProperties.viewProjection =
      mCameraProperties.projection * mCameraProperties.view;

    mCameraProperties.clipUnderPlanet = 1.0f;
    mCameraProperties.clippingRadius =
      mPlanetProperties.bottomRadius;
  }

  mCamera.init(mGraphicsContext, &mCameraProperties);

  { // Set lighting properties
    mLightingProperties.sunDirection =
      glm::normalize(glm::vec3(0.000001f, 0.01f, -1.00001f));
    mLightingProperties.sunSize = glm::vec3(
      0.0046750340586467079f, 0.99998907220740285f, 0.0f);
    mLightingProperties.exposure = 20.0f;
    mLightingProperties.white = glm::vec3(2.0f);
  }

  mDeferredLighting.init(
    mGraphicsContext,
    {mPipelineViewport.width, mPipelineViewport.height},
    &mLightingProperties);

  mScene.init(mGBuffer, mGraphicsContext);
  {
    auto handle1 = mScene.createSceneObject(); 
    auto &sceneObj1 = mScene.getSceneObject(handle1);
    sceneObj1.position = glm::vec3(0.0f, 80.0f, 0.0f);
    sceneObj1.scale = glm::vec3(5.0f);
    sceneObj1.rotation = glm::angleAxis(
      glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    sceneObj1.constructTransform();

    auto handle2 = mScene.createSceneObject(); 
    auto &sceneObj2 = mScene.getSceneObject(handle2);
    sceneObj2.position = glm::vec3(30.0f, 100.0f, 0.0f);
    sceneObj2.scale = glm::vec3(5.0f);
    sceneObj2.rotation = glm::angleAxis(
      glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sceneObj2.constructTransform();
  }

  mWaterRenderer.init(
    mGraphicsContext, mCameraProperties,
    mPlanetProperties, &mLightingProperties);
}

void Renderer3D::shutdown() {
  if (!Core::gFileSystem->isPathValid(
        (Core::MountPoint)Core::ApplicationMountPoints::Application,
        DRAW_CACHE_DIRECTORY)) {
    Core::gFileSystem->makeDirectory(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      DRAW_CACHE_DIRECTORY);
  }

  mSkyRenderer.shutdown(mGraphicsContext);
}

void Renderer3D::tick(const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  mCameraProperties.aspectRatio =
    (float)mPipelineViewport.width / (float)mPipelineViewport.height;

  mLightingProperties.dt = tick.dt;
  mLightingProperties.time = tick.accumulatedTime;

  mCamera.updateData(frame.primaryCommandBuffer, mCameraProperties);
  mDeferredLighting.updateData(frame.primaryCommandBuffer, mLightingProperties);

  mWaterRenderer.updateCameraInfo(mCameraProperties, mPlanetProperties);
  mWaterRenderer.updateCameraUBO(frame.primaryCommandBuffer);
  mWaterRenderer.updateLightingUBO(
    mLightingProperties, frame.primaryCommandBuffer);
  mWaterRenderer.tick(frame, mPlanetRenderer, mSkyRenderer, mScene);
     
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    // mPlanetRenderer.tick(tick, frame, mCamera);
    mScene.submit(mCamera, mPlanetRenderer, frame);
    // mSkyRenderer.tick(tick, frame, mCameraProperties);
  }
  mGBuffer.endRender(frame);

  mDeferredLighting.render(
    frame, mGBuffer, mCamera, mPlanetRenderer, mWaterRenderer, mSkyRenderer);
}

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  mPipelineViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(mGraphicsContext, newResolution);
  mDeferredLighting.resize(mGraphicsContext, newResolution);

  mCameraProperties.aspectRatio =
    (float)mPipelineViewport.width / (float)mPipelineViewport.height;

  mCameraProperties.projection = glm::perspective(
    mCameraProperties.fov,
    mCameraProperties.aspectRatio,
    mCameraProperties.near,
    mCameraProperties.far);

  mCameraProperties.inverseProjection = glm::inverse(
    mCameraProperties.projection);

  mWaterRenderer.resize(mGraphicsContext, newResolution);
}

void Renderer3D::trackInput(
  const Core::Tick &tick,
  const Core::InputTracker &inputTracker) {
  auto right = glm::cross(
    mCameraProperties.wViewDirection, mCameraProperties.wUp);

  float speedMultiplier = 30.0f;
  if (inputTracker.key(Core::KeyboardButton::R).isDown) {
    speedMultiplier *= 10.0f;
  }

  if (inputTracker.key(Core::KeyboardButton::W).isDown) {
    mCameraProperties.wPosition +=
      mCameraProperties.wViewDirection * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::A).isDown) {
    mCameraProperties.wPosition -=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::S).isDown) {
    mCameraProperties.wPosition -=
      mCameraProperties.wViewDirection * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::D).isDown) {
    mCameraProperties.wPosition +=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::Space).isDown) {
    mCameraProperties.wPosition +=
      mCameraProperties.wUp * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(Core::KeyboardButton::LeftShift).isDown) {
    mCameraProperties.wPosition -=
      mCameraProperties.wUp * tick.dt * speedMultiplier;
  }

  const auto &cursor = inputTracker.cursor();
  if (cursor.didCursorMove) {
    static constexpr float SENSITIVITY = 15.0f;

    auto delta = glm::vec2(cursor.cursorPos) - glm::vec2(cursor.previousPos);
    auto res = mCameraProperties.wViewDirection;

    float xAngle = glm::radians(-delta.x) * SENSITIVITY * tick.dt;
    float yAngle = glm::radians(-delta.y) * SENSITIVITY * tick.dt;
                
    res = glm::mat3(glm::rotate(xAngle, mCameraProperties.wUp)) * res;
    auto rotateY = glm::cross(res, mCameraProperties.wUp);
    res = glm::mat3(glm::rotate(yAngle, rotateY)) * res;

    res = glm::normalize(res);

    mCameraProperties.wViewDirection = res;
  }

  if (cursor.didScroll) {
    glm::mat3 rotation = glm::mat3(
      glm::rotate(
        glm::radians(30.0f * cursor.scroll.y) * tick.dt,
        glm::vec3(1.0f, 0.0f, 0.0f)));
    mLightingProperties.sunDirection = rotation *
      mLightingProperties.sunDirection;
  }

  mCameraProperties.view = glm::lookAt(
    mCameraProperties.wPosition,
    mCameraProperties.wPosition + mCameraProperties.wViewDirection,
    mCameraProperties.wUp);

  mCameraProperties.inverseView = glm::inverse(
    mCameraProperties.view);

  mCameraProperties.viewProjection =
    mCameraProperties.projection * mCameraProperties.view;
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mDeferredLighting;
}

}
