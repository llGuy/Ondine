#pragma once

#include <stdint.h>
#include "Utils.hpp"

namespace Ondine::Graphics {

class VulkanContext;
class VulkanCommandBuffer;

/* Contains stuff like the render target, command buffers, etc... */
struct VulkanFrame {
  VulkanCommandBuffer &primaryCommandBuffer;
  uint32_t imageIndex;
  uint32_t frameInFlight;
  Resolution viewport;
  bool skipped;
};

}
