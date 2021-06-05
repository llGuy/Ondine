#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "VulkanRenderPass.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine {

Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext) {
  
}

void Renderer3D::init() {
  mGBuffer.init(mGraphicsContext);
  mSkyRenderer.init(mGraphicsContext, mGBuffer);

  // Idle with all precomputation stuff
  mGraphicsContext.device().graphicsQueue().idle();

  auto properties = mGraphicsContext.getProperties();
  mPipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };

  { // Set planet properties
    /* Temporary - the world needs to define this */
    mPlanetProperties.solarIrradiance = glm::vec3(1.474f, 1.8504f, 1.91198f);
    // Angular radius of the Sun (radians)
    mPlanetProperties.solarAngularRadius = 0.004675f;
    mPlanetProperties.bottomRadius = 6360.0f;
    mPlanetProperties.topRadius = 6420.0f;

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
    mPlanetProperties.groundAlbedo = glm::vec3(0.100000f, 0.100000f, 0.100000f);
    mPlanetProperties.muSunMin = -0.207912f;
    mPlanetProperties.wPlanetCenter = glm::vec3(0.0f, -6360.0f, 0.0f);

    mPlanetRenderer.init(mGraphicsContext, mGBuffer, &mPlanetProperties);
  }

  { // Set camera properties
    mCameraProperties.fov = glm::radians(50.0f);
    mCameraProperties.aspectRatio =
      (float)mPipelineViewport.width / (float)mPipelineViewport.height;
    mCameraProperties.near = 1.0f;
    mCameraProperties.far = 10000000.0f;

    mCameraProperties.projection = glm::perspective(
      mCameraProperties.fov,
      mCameraProperties.aspectRatio,
      mCameraProperties.near,
      mCameraProperties.far);

    mCameraProperties.wPosition = glm::vec3(0.0f, 10.0f, 0.0f);
    mCameraProperties.wViewDirection =
      glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
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
  }

  mCamera.init(mGraphicsContext, &mCameraProperties);

  { // Set lighting properties
    mLightingProperties.sunDirection =
      glm::normalize(glm::vec3(0.000001f, 0.2f, -1.00001f));
    mLightingProperties.sunSize = glm::vec3(
      0.0046750340586467079f, 0.99998907220740285f, 0.0f);
    mLightingProperties.exposure = 20.0f;
    mLightingProperties.white = glm::vec3(1.0f);
  }

  mDeferredLighting.init(mGraphicsContext, &mLightingProperties);
}

void Renderer3D::tick(const Tick &tick, VulkanFrame &frame) {
  mCameraProperties.aspectRatio =
    (float)mPipelineViewport.width / (float)mPipelineViewport.height;

  mCamera.updateData(frame.primaryCommandBuffer, mCameraProperties);
  mDeferredLighting.updateData(frame.primaryCommandBuffer, mLightingProperties);
     
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    mPlanetRenderer.tick(tick, frame, mCamera);
  }
  mGBuffer.endRender(frame);

  mDeferredLighting.render(
    frame, mGBuffer, mCamera, mPlanetRenderer, mSkyRenderer);
}

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  mPipelineViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(mGraphicsContext, newResolution);
  mDeferredLighting.resize(mGraphicsContext, newResolution);
}

void Renderer3D::trackInput(
  const Tick &tick,
  const InputTracker &inputTracker) {
  auto right = glm::cross(
    mCameraProperties.wViewDirection, mCameraProperties.wUp);

  float speedMultiplier = 10.0f;
  if (inputTracker.key(KeyboardButton::R).isDown) {
    speedMultiplier *= 100.0f;
  }

  if (inputTracker.key(KeyboardButton::W).isDown) {
    mCameraProperties.wPosition +=
      mCameraProperties.wViewDirection * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(KeyboardButton::A).isDown) {
    mCameraProperties.wPosition -=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(KeyboardButton::S).isDown) {
    mCameraProperties.wPosition -=
      mCameraProperties.wViewDirection * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(KeyboardButton::D).isDown) {
    mCameraProperties.wPosition +=
      right * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(KeyboardButton::Space).isDown) {
    mCameraProperties.wPosition +=
      mCameraProperties.wUp * tick.dt * speedMultiplier;
  }
  if (inputTracker.key(KeyboardButton::LeftShift).isDown) {
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
