#include "Camera.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Graphics {

void Camera::init(
  VulkanContext &graphicsContext,
  const CameraProperties *properties) {
  mCameraBuffer.init(
    graphicsContext.device(),
    sizeof(CameraProperties),
    (int)VulkanBufferFlag::UniformBuffer);

  mUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mCameraBuffer));

  if (properties) {
    mCameraBuffer.fillWithStaging(
      graphicsContext.device(),
      graphicsContext.commandPool(),
      {(uint8_t *)properties, sizeof(CameraProperties)});
  }
}

void Camera::updateData(
  const VulkanCommandBuffer &commandBuffer,
  const CameraProperties &properties) {
  commandBuffer.updateBuffer(
    mCameraBuffer, 0, sizeof(properties), &properties);
}

const VulkanUniform &Camera::uniform() const {
  return mUniform;
}

}
