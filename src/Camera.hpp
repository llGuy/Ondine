#pragma once

#include <glm/glm.hpp>
#include "VulkanFrame.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanUniform.hpp"

namespace Ondine {

class VulkanContext;

struct CameraProperties {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 inverseProjection;
  glm::mat4 inverseView;
  glm::mat4 viewProjection;
  alignas(16) glm::vec3 wPosition;
  alignas(16) glm::vec3 wViewDirection;
  alignas(16) glm::vec3 wUp;
  float fov;
  float aspectRatio;
  float near;
  float far;
};

class Camera {
public:
  Camera() = default;

  void init(
    VulkanContext &graphicsContext,
    const CameraProperties *properties = nullptr);

  void updateData(
    const VulkanCommandBuffer commandBuffer,
    const CameraProperties &properties);

  const VulkanUniform &uniform() const;

private:
  VulkanBuffer mCameraBuffer;
  VulkanUniform mUniform;
};

}
