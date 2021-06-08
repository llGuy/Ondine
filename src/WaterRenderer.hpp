#pragma once

#include "GBuffer.hpp"

namespace Ondine::Graphics {

class WaterRenderer {
public:
  void init();
  void tick(VulkanFrame &frame);

private:
  GBuffer mGBuffer;
};

}
