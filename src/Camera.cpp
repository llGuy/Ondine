#include "Camera.hpp"
#include "VulkanContext.hpp"

namespace Yona {

void Camera::init(VulkanContext &graphicsContext) {
  mCameraBuffer.init(
    graphicsContext.device(),
    sizeof(CameraProperties),
    (int)VulkanBufferFlag::UniformBuffer);

  mUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mCameraBuffer));
}

void Camera::updateData(
  const VulkanFrame &frame,
  const CameraProperties &properties) {
  frame.primaryCommandBuffer.updateBuffer(
    mCameraBuffer, 0, sizeof(properties), &properties);
}

const VulkanUniform &Camera::uniform() const {
  return mUniform;
}

}
