#pragma once

#include "yona_utils.hpp"
#include "yona_vulkan.hpp"
#include "yona_buffer.hpp"
#include <vulkan/vulkan.h>
#include "yona_vulkan_device.hpp"

namespace Yona {

struct VulkanDescriptorTypeInfo {
  size_t size;
  VkDescriptorType type;

  operator VkDescriptorPoolSize() const {
    VkDescriptorPoolSize str = {};
    str.descriptorCount = size;
    str.type = type;

    return str;
  }
};

class VulkanDescriptorPool {
public:
  template <typename ...Args>
  void init(
    const VulkanDevice &device,
    Args &&...types) {
    size_t count = sizeof...(Args);

    Array<VkDescriptorPoolSize, AllocationType::Linear> sizes =
      makeArray<VkDescriptorPoolSize, AllocationType::Linear>(types...);

    uint32_t maxSets = 0;
    for (int i = 0; i < sizes.size; ++i) {
      maxSets += sizes[i].descriptorCount;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = sizes.size;
    poolInfo.pPoolSizes = sizes.data;

    VK_CHECK(
      vkCreateDescriptorPool(
        device.mLogicalDevice,
        &poolInfo,
        NULL,
        &mDescriptorPool));
  }

private:
  VkDescriptorPool mDescriptorPool;

  friend class VulkanImgui;
};

class VulkanDescriptorSetLayoutCategory {
public:
  VulkanDescriptorSetLayoutCategory();
  void init(VkDescriptorType type);

  VkDescriptorSetLayout getDescriptorSetLayout(
    const VulkanDevice &device, size_t count);
  
private:
  static constexpr uint32_t MAX_DESCRIPTOR_SET_LAYOUTS_PER_TYPE = 20;

  VkDescriptorType mType;
  VkDescriptorSetLayout mLayouts[MAX_DESCRIPTOR_SET_LAYOUTS_PER_TYPE];
};

class VulkanDescriptorSetLayoutMaker {
public:
  void init();

  VkDescriptorSetLayout getDescriptorSetLayout(
    const VulkanDevice &device, VkDescriptorType type, size_t count);

private:
  VkDescriptorSetLayout getDescriptorSetLayout();

private:
  static constexpr uint32_t CATEGORY_COUNT =
    MIN((int)VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10);
  
  VulkanDescriptorSetLayoutCategory mCategories[CATEGORY_COUNT];
};

}
