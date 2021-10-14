#include "Log.hpp"
#include "Utils.hpp"
#include "Vulkan.hpp"
#include "VulkanSync.hpp"
#include "VulkanDevice.hpp"

namespace Ondine::Graphics {

VkAccessFlags findAccessFlagsForStage(VkPipelineStageFlags stage) {
  switch (stage) {
  case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT;

  case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:
  case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

  case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:
    return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

  case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:
    return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

  case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
  case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
  case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
    return VK_ACCESS_UNIFORM_READ_BIT;

  case VK_PIPELINE_STAGE_TRANSFER_BIT:
    return VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;

  case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

  case VK_PIPELINE_STAGE_ALL_COMMANDS_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

  default: {
    LOG_ERROR("Didn't handle stage for finding access flags!\n");
    PANIC_AND_EXIT();
  } return 0;
  }
}

VkAccessFlags findAccessFlagsForImageLayout(VkImageLayout layout) {
  switch (layout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    return 0;

  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    return VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    return VK_ACCESS_TRANSFER_WRITE_BIT;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    return VK_ACCESS_SHADER_READ_BIT;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  default: {
    LOG_ERROR("Didn't handle image layout for finding access flags!\n");
    PANIC_AND_EXIT();
  } return 0;
  }
}

void VulkanSemaphore::init(const VulkanDevice &device) {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VK_CHECK(
    vkCreateSemaphore(
      device.mLogicalDevice,
      &semaphoreInfo,
      NULL,
      &mSemaphore));
}

VulkanFence::VulkanFence()
  : mFence(VK_NULL_HANDLE) {
  
}

void VulkanFence::init(const VulkanDevice &device, VkFenceCreateFlags flags) {
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = flags;

  VK_CHECK(
    vkCreateFence(
      device.mLogicalDevice,
      &fenceInfo,
      NULL,
      &mFence));
}

void VulkanFence::wait(const VulkanDevice &device) {
  vkWaitForFences(device.mLogicalDevice, 1, &mFence, VK_TRUE, UINT64_MAX);
}

void VulkanFence::reset(const VulkanDevice &device) {
  vkResetFences(device.mLogicalDevice, 1, &mFence);
}

}
