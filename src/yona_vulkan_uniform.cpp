#include "yona_vulkan_uniform.hpp"

namespace Yona {

void VulkanUniform::init(
  const VulkanDevice &device,
  const VulkanDescriptorPool &pool,
  VulkanDescriptorSetLayoutMaker &layouts,
  const Array<VulkanBuffer, AllocationType::Linear> &buffers) {
  VkDescriptorSetLayout layout = layouts.getDescriptorSetLayout(
    device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffers.size);

  VkDescriptorSetAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool.mDescriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;

  vkAllocateDescriptorSets(
    device.mLogicalDevice, &allocateInfo, &mDescriptorSet);

  VkDescriptorBufferInfo *bufferInfos = STACK_ALLOC(
    VkDescriptorBufferInfo, buffers.size);

  VkWriteDescriptorSet *writes = STACK_ALLOC(
    VkWriteDescriptorSet, buffers.size);

  memset(bufferInfos, 0, sizeof(VkDescriptorBufferInfo) * buffers.size);
  memset(writes, 0, sizeof(VkWriteDescriptorSet) * buffers.size);

  for (int i = 0; i < buffers.size; ++i) {
    bufferInfos[i].buffer = buffers[i].mBuffer;
    bufferInfos[i].offset = 0;
    bufferInfos[i].range = buffers[i].mSize;

    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = mDescriptorSet;
    writes[i].dstBinding = i;
    writes[i].dstArrayElement = 0;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[i].pBufferInfo = &bufferInfos[i];
  }

  vkUpdateDescriptorSets(
    device.mLogicalDevice,
    buffers.size,
    writes,
    0, nullptr);
}

void VulkanUniform::init(
  const VulkanDevice &device,
  const VulkanDescriptorPool &pool,
  VulkanDescriptorSetLayoutMaker &layouts,
  const Array<VulkanTexture, AllocationType::Linear> &textures,
  VkDescriptorType type) {
  VkDescriptorSetLayout layout = layouts.getDescriptorSetLayout(
    device, type, textures.size);

  VkDescriptorSetAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool.mDescriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;

  vkAllocateDescriptorSets(
    device.mLogicalDevice, &allocateInfo, &mDescriptorSet);

  VkDescriptorImageInfo *imageInfos = STACK_ALLOC(
    VkDescriptorImageInfo, textures.size);

  VkWriteDescriptorSet *writes = STACK_ALLOC(
    VkWriteDescriptorSet, textures.size);

  memset(imageInfos, 0, sizeof(VkDescriptorImageInfo) * textures.size);
  memset(writes, 0, sizeof(VkWriteDescriptorSet) * textures.size);

  for (int i = 0; i < textures.size; ++i) {
    imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[i].imageView = textures[i].mImageViewSample;
    imageInfos[i].sampler = textures[i].mSampler;

    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = mDescriptorSet;
    writes[i].dstBinding = i;
    writes[i].dstArrayElement = 0;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = type;
    writes[i].pImageInfo = &imageInfos[i];
  }

  vkUpdateDescriptorSets(
    device.mLogicalDevice,
    textures.size,
    writes,
    0, nullptr);
}

void VulkanUniform::destroy(
  const VulkanDevice &device, const VulkanDescriptorPool &pool) {
  vkFreeDescriptorSets(
    device.mLogicalDevice, pool.mDescriptorPool, 1, &mDescriptorSet);
}

}
