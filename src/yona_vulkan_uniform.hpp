#pragma once

#include "yona_buffer.hpp"
#include "yona_vulkan_buffer.hpp"
#include "yona_vulkan_texture.hpp"
#include "yona_vulkan_descriptor.hpp"

namespace Yona {

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

private:
  VkDescriptorSet mDescriptorSet;

  friend class VulkanCommandBuffer;
};

}
