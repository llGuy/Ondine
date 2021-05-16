#pragma once

#include <stdint.h>

namespace Yona {

class VulkanDevice;
class VulkanCommandBuffer;

/* Contains stuff like the render target, command buffers, etc... */
struct VulkanFrame {
  const VulkanDevice &device;
  const VulkanCommandBuffer &primaryCommandBuffer;
  uint32_t imageIndex;
  uint32_t frameInFlight;
};

}
