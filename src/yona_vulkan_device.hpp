#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

enum DeviceType {
  DiscreteGPU,
  Integrated,
  Any
};

struct QueueFamilies {
  int graphicsFamily;
  int presentFamily;

  inline bool isComplete() {
    return graphicsFamily >= 0 && presentFamily >= 0;
  }
};

struct DeviceExtensions {
  /* If the needed extensions aren't available, PANIC_AND_EXIT */
  uint32_t needed;
  uint32_t available;

  uint32_t count;
  const char **names;

  inline void setNeeded(uint32_t i) {
    needed |= 1 << i;
  }

  inline uint32_t isNeeded(uint32_t i) const {
    return needed & (1 << i);
  }
};

struct DeviceRequestedFeatures {
  VkPhysicalDeviceFeatures features;
  size_t count;
};

struct DeviceSwapchainSupport {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t availableFormatsCount;
  VkSurfaceFormatKHR *availableFormats;
  uint32_t availablePresentModesCount;
  VkPresentModeKHR *availablePresentModes;
};

class VulkanInstance;
class VulkanSurface;

class VulkanDevice {
public:
  VulkanDevice() = default;

  void init(
    DeviceType requestedType,
    const VulkanInstance &instance,
    const VulkanSurface &surface,
    const DeviceRequestedFeatures &features);

private:
  bool verifyHardwareMeetsRequirements(
    DeviceType requestedType,
    const VulkanSurface &surface,
    const DeviceRequestedFeatures &requiredFeatures,
    const DeviceExtensions &requestedExtensions,
    DeviceExtensions &availableExtensions);

  VkFormat findSuitableDepthFormat(
    VkFormat *formats,
    uint32_t formatCount,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) const;

  bool checkRequiredFeaturesSupport(
    const DeviceRequestedFeatures &required,
    const VkPhysicalDeviceFeatures &available);

private:
  VkDevice mLogicalDevice;
  VkPhysicalDevice mPhysicalDevice;
  QueueFamilies mQueueFamilies;
  DeviceSwapchainSupport mSwapchainSupport;
  VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryInfo;
  VkPhysicalDeviceProperties mPhysicalDeviceInfo;
  VkFormat mDepthFormat;
  VkQueue mGraphicsQueue;
  VkQueue mPresentQueue;

  friend class VulkanContext;
};

}
