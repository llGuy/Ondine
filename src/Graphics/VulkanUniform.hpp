#pragma once

#include "Buffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanDescriptor.hpp"

namespace Ondine::Graphics {

/* 
   We call descriptor sets uniforms.
   Can only contain one type of descriptor (n buffers, or n images)
*/
class VulkanUniform {
public:
  void init(
    const VulkanDevice &device,
    const VulkanDescriptorPool &pool,
    VulkanDescriptorSetLayoutMaker &layouts,
    const Array<VulkanBuffer, AllocationType::Linear> &buffers);

  void init(
    const VulkanDevice &device,
    const VulkanDescriptorPool &pool,
    VulkanDescriptorSetLayoutMaker &layouts,
    const Array<VulkanTexture, AllocationType::Linear> &textures,
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  void destroy(
    const VulkanDevice &device, const VulkanDescriptorPool &pool);

private:
  VkDescriptorSet mDescriptorSet;

  friend class VulkanCommandBuffer;
};

}
