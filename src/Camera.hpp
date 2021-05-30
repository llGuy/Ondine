#pragma once

#include <glm/glm.hpp>
#include "VulkanFrame.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanUniform.hpp"

namespace Yona {

class VulkanContext;

struct CameraProperties {
  glm::mat4 mProjection;
  glm::mat4 mView;
  glm::mat4 mInverseProjection;
  glm::mat4 mInverseView;
  glm::mat4 mViewProjection;
  alignas(16) glm::vec3 mWPosition;
  alignas(16) glm::vec3 mWViewDirection;
  float mFOV;
  float mAspectRatio;
  float mNear;
  float mFar;
};

class Camera {
public:
  Camera() = default;

  void init(VulkanContext &graphicsContext);
  void updateData(
    const VulkanFrame &frame,
    const CameraProperties &properties);

  const VulkanUniform &uniform() const;

private:
  VulkanBuffer mCameraBuffer;
  VulkanUniform mUniform;
};

}
