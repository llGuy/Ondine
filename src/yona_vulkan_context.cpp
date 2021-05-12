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

  DeviceRequestedFeatures requiredFeatures = {};
  requiredFeatures.count = 1;
  requiredFeatures.features.geometryShader = VK_TRUE;

  mDevice.init(DeviceType::Any, mInstance, mSurface, requiredFeatures);
}

const VulkanDevice &VulkanContext::device() const {
  return mDevice;
}

VulkanContextProperties VulkanContext::getProperties() const {
  VulkanContextProperties properties = {};
  properties.swapchainFormat = mSwapchain.mFormat;
  properties.depthFormat = mDevice.mDepthFormat;
  return properties;
}

}
