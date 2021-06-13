#include "WaterRenderer.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

void WaterRenderer::init(
  VulkanContext &graphicsContext,
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties,
  const LightingProperties *lightingProperties) {
  auto properties = graphicsContext.getProperties();
  mReflectionViewport = {
    (uint32_t)((float)properties.swapchainExtent.width * VIEWPORT_SCALE),
    (uint32_t)((float)properties.swapchainExtent.height * VIEWPORT_SCALE)
  };

  updateCameraInfo(sceneCamera, planetProperties);

  mGBuffer.init(
    graphicsContext,
    {mReflectionViewport.width, mReflectionViewport.height});

  mReflectionCamera.init(graphicsContext, &mCameraProperties);

  mLighting.init(
    graphicsContext,
    {mReflectionViewport.width, mReflectionViewport.height},
    lightingProperties);
}

void WaterRenderer::tick(
  VulkanFrame &frame,
  const PlanetRenderer &planet,
  const SkyRenderer &sky,
  SceneSubmitter &sceneSubmitter) {
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    sceneSubmitter.submit(mReflectionCamera, planet, frame);
  }
  mGBuffer.endRender(frame);

  mLighting.render(
    frame, mGBuffer, mReflectionCamera, planet, sky);
}

void WaterRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  mReflectionViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(vulkanContext, newResolution);
  mLighting.resize(vulkanContext, newResolution);

  mCameraProperties.aspectRatio =
    (float)mReflectionViewport.width / (float)mReflectionViewport.height;

  mCameraProperties.projection = glm::perspective(
    mCameraProperties.fov,
    mCameraProperties.aspectRatio,
    mCameraProperties.near,
    mCameraProperties.far);

  mCameraProperties.inverseProjection = glm::inverse(
    mCameraProperties.projection);
}

glm::vec3 WaterRenderer::reflectCameraPosition(
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  // Get distance between camera and planet
  auto diff = sceneCamera.wPosition / 1000.0f - planetProperties.wPlanetCenter;
  float diffDist = glm::length(diff);
  float groundToCam = diffDist - planetProperties.bottomRadius - 0.1f;
  glm::vec3 toCenter = -glm::normalize(diff);

  return sceneCamera.wPosition / 1000.0f + toCenter * 2.0f * groundToCam;
}

glm::vec3 WaterRenderer::reflectCameraDirection(
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  glm::vec3 normal = glm::normalize(
      sceneCamera.wPosition / 1000.0f - planetProperties.wPlanetCenter);

  glm::vec3 right = glm::normalize(
    glm::cross(sceneCamera.wViewDirection, normal));

  glm::vec3 refVector = glm::normalize(glm::cross(normal, right));

  return
    glm::normalize(-glm::reflect(sceneCamera.wViewDirection, refVector));
}

void WaterRenderer::updateCameraInfo(
  const CameraProperties &camera,
  const PlanetProperties &planet) {
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

  mCameraProperties.wPosition = reflectCameraPosition(
    camera, planet) * 1000.0f;
  mCameraProperties.wViewDirection = reflectCameraDirection(
    camera, planet);
  // mCameraProperties.wUp = glm::normalize(
  // planet.wPlanetCenter - camera.wPosition / 1000.0f);
  mCameraProperties.wUp = - camera.wUp;

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
    planet.bottomRadius + 0.1f;
}

void WaterRenderer::updateCameraUBO(const VulkanCommandBuffer &commandBuffer) {
  mReflectionCamera.updateData(commandBuffer, mCameraProperties);
}

const VulkanRenderPass &WaterRenderer::renderPass() const {
  return mGBuffer.renderPass();
}

const VulkanFramebuffer &WaterRenderer::framebuffer() const {
  return mGBuffer.framebuffer();
}

const VulkanUniform &WaterRenderer::uniform() const {
  return mLighting.uniform();
}

VkExtent2D WaterRenderer::extent() const {
  return {mReflectionViewport.width, mReflectionViewport.height};
}

}
