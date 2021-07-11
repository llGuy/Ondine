#include "RendererDebug.hpp"
#include "WaterRenderer.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

void WaterRenderer::init(
  VulkanContext &graphicsContext,
  const PlanetProperties &planetProperties) {
  auto properties = graphicsContext.getProperties();
  mReflectionViewport = {
    (uint32_t)((float)properties.swapchainExtent.width * VIEWPORT_SCALE),
    (uint32_t)((float)properties.swapchainExtent.height * VIEWPORT_SCALE)
  };

  mGBuffer.init(
    graphicsContext,
    {mReflectionViewport.width, mReflectionViewport.height});

  mReflectionCamera.init(graphicsContext, &mCameraProperties);

  mLighting.init(
    graphicsContext,
    {mReflectionViewport.width, mReflectionViewport.height});

  mClipping.init(
    graphicsContext, 1.0f,
    planetProperties.bottomRadius + OCEAN_HEIGHT);
}

void WaterRenderer::tick(
  VulkanFrame &frame,
  const PlanetRenderer &planet,
  const SkyRenderer &sky,
  const StarRenderer &stars,
  const TerrainRenderer &terrainRenderer,
  Scene &sceneSubmitter) {
  frame.primaryCommandBuffer.dbgBeginRegion("WaterStage", DBG_WATER_COLOR);

  mGBuffer.beginRender(frame);
  { // Render 3D scene
    sceneSubmitter.submit(
      mReflectionCamera, planet, mClipping, terrainRenderer, frame);
    stars.render(1.0f, mReflectionCamera, frame);
  }
  mGBuffer.endRender(frame);

  mLighting.render(
    frame, mGBuffer, mReflectionCamera, planet, sky);

  frame.primaryCommandBuffer.dbgEndRegion();
}

void WaterRenderer::resize(
  VulkanContext &vulkanContext, Resolution newResolution) {
  mReflectionViewport = {
    (uint32_t)((float)newResolution.width * VIEWPORT_SCALE),
    (uint32_t)((float)newResolution.height * VIEWPORT_SCALE)
  };

  mGBuffer.resize(vulkanContext, mReflectionViewport);
  mLighting.resize(vulkanContext, mReflectionViewport);

  mCameraProperties.mAspectRatio =
    (float)mReflectionViewport.width / (float)mReflectionViewport.height;

  mCameraProperties.mProjection = glm::perspective(
    mCameraProperties.fov,
    mCameraProperties.mAspectRatio,
    mCameraProperties.mNear,
    mCameraProperties.mFar);

  mCameraProperties.mInverseProjection = glm::inverse(
    mCameraProperties.mProjection);
}

glm::vec3 WaterRenderer::reflectCameraPosition(
  const CameraProperties &sceneCamera,
  const PlanetProperties &planetProperties) {
  // Get distance between camera and planet
  auto diff = sceneCamera.wPosition / 1000.0f - planetProperties.wPlanetCenter;
  float diffDist = glm::length(diff);
  float groundToCam = diffDist - planetProperties.bottomRadius - OCEAN_HEIGHT;
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
  mCameraProperties.mAspectRatio =
    (float)mReflectionViewport.width / (float)mReflectionViewport.height;
  mCameraProperties.mNear = 0.1f;
  mCameraProperties.mFar = 10000.0f;

  mCameraProperties.mProjection = glm::perspective(
    mCameraProperties.fov,
    mCameraProperties.mAspectRatio,
    mCameraProperties.mNear,
    mCameraProperties.mFar);

  mCameraProperties.wPosition = reflectCameraPosition(
    camera, planet) * 1000.0f;
  mCameraProperties.wViewDirection = reflectCameraDirection(
    camera, planet);
  // mCameraProperties.wUp = glm::normalize(
  // planet.wPlanetCenter - camera.wPosition / 1000.0f);
  mCameraProperties.wUp = - camera.wUp;

  mCameraProperties.mView = glm::lookAt(
    mCameraProperties.wPosition,
    mCameraProperties.wPosition + mCameraProperties.wViewDirection,
    mCameraProperties.wUp);

  mCameraProperties.mInverseProjection = glm::inverse(
    mCameraProperties.mProjection);

  mCameraProperties.mInverseView = glm::inverse(
    mCameraProperties.mView);

  mCameraProperties.mViewProjection =
    mCameraProperties.mProjection * mCameraProperties.mView;
}

void WaterRenderer::updateCameraUBO(const VulkanCommandBuffer &commandBuffer) {
  mReflectionCamera.updateData(commandBuffer, mCameraProperties);
}

void WaterRenderer::updateLightingUBO(
  const LightingProperties &properties,
  const VulkanCommandBuffer &commandBuffer) {
  mLighting.updateData(commandBuffer, properties);
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
