#include "WaterRenderer.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

void WaterRenderer::init(
  VulkanContext &graphicsContext,
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  auto properties = graphicsContext.getProperties();
  mReflectionViewport = {
    (uint32_t)((float)properties.swapchainExtent.width * VIEWPORT_SCALE),
    (uint32_t)((float)properties.swapchainExtent.height * VIEWPORT_SCALE)
  };

  { // Set starting reflection camera properties
     // Set camera properties
    mCameraProperties.fov = glm::radians(50.0f);
    mCameraProperties.aspectRatio =
      (float)mReflectionViewport.width / (float)mReflectionViewport.height;
    mCameraProperties.near = 0.1f;
    mCameraProperties.far = 10000.0f;

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

    mCameraProperties.clipUnderPlanet = 1.0f;
    mCameraProperties.clippingRadius =
      planetProperties.bottomRadius;
  }

  mGBuffer.init(
    graphicsContext,
    {mReflectionViewport.width, mReflectionViewport.height});

  mReflectionCamera.init(graphicsContext, &mCameraProperties);
}

void WaterRenderer::tick(
  VulkanFrame &frame,
  SceneSubmitter &sceneSubmitter) {
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    // sceneSubmitter.submit(mReflectionCamera, frame);
  }
  mGBuffer.endRender(frame);
}

void WaterRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  
}

glm::vec3 WaterRenderer::reflectCameraPosition(
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  // Get distance between camera and planet
  auto diff = sceneCamera.wPosition - planetProperties.wPlanetCenter;
  float diffDist = glm::length(diff);
  float groundToCam = diffDist - planetProperties.bottomRadius;

  return planetProperties.wPlanetCenter +
    (sceneCamera.wPosition - planetProperties.wPlanetCenter) *
    (diffDist - 2.0f * (diffDist - planetProperties.bottomRadius));
}

glm::vec3 WaterRenderer::reflectCameraDirection(
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  glm::vec3 normal =
    glm::normalize(sceneCamera.wPosition - planetProperties.wPlanetCenter);

  glm::vec3 right = glm::cross(sceneCamera.wViewDirection, normal);
  glm::vec3 refVector = glm::cross(normal, right);

  return glm::reflect(sceneCamera.wViewDirection, refVector);
}

void WaterRenderer::updateCameraInfo(
  const CameraProperties &camera,
  const PlanetRenderer &planet) {
  
}

}
