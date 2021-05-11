#include "yona_vulkan_context.hpp"

#ifndef DEBUG
constexpr bool ENABLE_VALIDATION = true;
#else
constexpr bool ENABLE_VALIDATION = false;
#endif

namespace Yona {

VulkanContext::VulkanContext()
  : mInstance(ENABLE_VALIDATION) {
  
}

void VulkanContext::initInstance() {
  /* Just instance */
  mInstance.init();
}

void VulkanContext::initContext(const WindowContextInfo &surfaceInfo) {
  /* Everything else */
  mSurface.init(mInstance, surfaceInfo);

  VkPhysicalDeviceFeatures requiredFeatures = {};
  requiredFeatures.geometryShader = VK_TRUE;

  mDevice.init(mInstance, mSurface, requiredFeatures);
}

VkInstance VulkanContext::instance() const {
  return mInstance.mInstance;
}

VkDevice VulkanContext::device() const {
  return mDevice.mLogicalDevice;
}

}
