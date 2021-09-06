#pragma once

#include <glm/glm.hpp>
#include "VulkanFrame.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanUniform.hpp"
#include <glm/gtx/transform.hpp>

namespace Ondine::Graphics {

class VulkanContext;

struct CameraProperties {
private:
  glm::mat4 mProjection;
  glm::mat4 mView;
  glm::mat4 mInverseProjection;
  glm::mat4 mInverseView;
  glm::mat4 mViewProjection;

  /* Things which can be modified from outside */
public:
  alignas(16) glm::vec3 wPosition;
  alignas(16) glm::vec3 wViewDirection;
  alignas(16) glm::vec3 wUp;
  float fov;

private:
  float mAspectRatio;
  float mNear;
  float mFar;

  void tick(Resolution viewportRes) {
    mNear = 0.1f;
    mFar = 10000.0f;
    mAspectRatio = (float)viewportRes.width / (float)viewportRes.height;

    mProjection = glm::perspective(fov, mAspectRatio, mNear, mFar);

    mView = glm::lookAt(wPosition, wPosition + wViewDirection, wUp);
    mInverseProjection = glm::inverse(mProjection);
    mInverseView = glm::inverse(mView);

    mViewProjection = mProjection * mView;
  }

  friend class Renderer3D;
  friend class WaterRenderer;
};

class Camera {
public:
  Camera() = default;

  void init(
    VulkanContext &graphicsContext,
    const CameraProperties *properties = nullptr);

  void updateData(
    const VulkanCommandBuffer &commandBuffer,
    const CameraProperties &properties);

  const VulkanUniform &uniform() const;

private:
  VulkanBuffer mCameraBuffer;
  VulkanUniform mUniform;
};

}
