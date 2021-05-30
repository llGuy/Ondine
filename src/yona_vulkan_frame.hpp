#pragma once

#include <stdint.h>
#include "yona_io.hpp"

namespace Yona {

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
