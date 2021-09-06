#pragma once

#include "VulkanContext.hpp"
#include "VulkanUniform.hpp"

namespace Ondine::Graphics {

class Clipping {
public:
  Clipping() = default;

  // For clipping everything above, set clipFactor to -1.0f
  void init(VulkanContext &graphicsContext, float clipFactor, float radius);

public:
  VulkanUniform uniform;

private:
  struct {
    alignas(4) float clipFactor;
    alignas(4) float clippingRadius;
  } mData;

  VulkanBuffer mUBO;
};

}
