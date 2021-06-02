#pragma once

#include "Utils.hpp"
#include "Buffer.hpp"
#include <vulkan/vulkan.h>

namespace Ondine {

class VulkanDevice;
class VulkanCommandPool;

enum class VulkanBufferFlag {
  VertexBuffer = BIT(0),
  IndexBuffer = BIT(1),
  UniformBuffer = BIT(2),
  Mappable = BIT(3),
  TransferSource = BIT(4),
  // By default, all buffers are transfer dst
};

using VulkanBufferFlagBits = uint32_t;

DEFINE_BIT_OPS_FOR_ENUM_CLASS(VulkanBufferFlag, VulkanBufferFlagBits, uint32_t);

class VulkanBuffer {
public:
  void init(
    const VulkanDevice &device,
    size_t size, VulkanBufferFlagBits type);

  void destroy(const VulkanDevice &device);

  void fillWithStaging(
    const VulkanDevice &device,
    const VulkanCommandPool &commandPool,
    const Buffer &data);

  void *map(const VulkanDevice &device, size_t size, size_t offset) const;
  void unmap(const VulkanDevice &device) const;

  VkBufferMemoryBarrier makeBarrier(
    VkPipelineStageFlags src,
    VkPipelineStageFlags dst,
    size_t offset, size_t size) const;

private:
  VkBuffer mBuffer;
  size_t mSize;
  VkDeviceMemory mMemory;
  VkBufferUsageFlags mUsage;
  VkPipelineStageFlags mUsedAtEarliest;
  VkPipelineStageFlags mUsedAtLatest;

  friend class VulkanCommandBuffer;
  friend class VulkanUniform;
};

}
