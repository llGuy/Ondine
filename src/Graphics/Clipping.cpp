#include "Clipping.hpp"

namespace Ondine::Graphics {

void Clipping::init(
  VulkanContext &graphicsContext,
  float clipFactor, float radius) {
  mUBO.init(
    graphicsContext.device(), sizeof(mData),
    (int)VulkanBufferFlag::UniformBuffer);

  uniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mUBO));

  mData.clipFactor = clipFactor;
  mData.clippingRadius = radius;

  mUBO.fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)&mData, sizeof(mData)});
}

}
