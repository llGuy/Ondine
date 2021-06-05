#include "VulkanDescriptor.hpp"

namespace Ondine::Graphics {

VulkanDescriptorSetLayoutCategory::VulkanDescriptorSetLayoutCategory()
  : mLayouts{} {
  
}

void VulkanDescriptorSetLayoutCategory::init(VkDescriptorType type) {
  mType = type;
}

VkDescriptorSetLayout VulkanDescriptorSetLayoutCategory::getDescriptorSetLayout(
  const VulkanDevice &device, size_t count) {
  if (mLayouts[count - 1] == VK_NULL_HANDLE) {
    Array<VkDescriptorSetLayoutBinding, AllocationType::Linear> bindings(count);
    bindings.zero();

    for (int i = 0; i < count; ++i) {
      bindings[i].binding = i;
      bindings[i].descriptorType = mType;
      bindings[i].descriptorCount = 1;
      bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = count;
    layoutInfo.pBindings = bindings.data;

    VK_CHECK(
      vkCreateDescriptorSetLayout(
        device.mLogicalDevice,
        &layoutInfo,
        NULL,
        &mLayouts[count - 1]))
  }

  return mLayouts[count - 1];
}

void VulkanDescriptorSetLayoutMaker::init() {
  for (int i = 0; i < CATEGORY_COUNT; ++i) {
    mCategories[i].init((VkDescriptorType)i);
  }
}

VkDescriptorSetLayout VulkanDescriptorSetLayoutMaker::getDescriptorSetLayout(
  const VulkanDevice &device, VkDescriptorType type, size_t count) {
  return mCategories[(int)type].getDescriptorSetLayout(device, count);
}

}
