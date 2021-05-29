#include <string.h>
#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_vulkan.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"

namespace Yona {

void VulkanDevice::init(
  DeviceType requestedType,
  const VulkanInstance &instance,
  const VulkanSurface &surface,
  const DeviceRequestedFeatures &requiredFeatures) {
  enum {
    SwapchainExtIndex,
    DebugMarkerExtIndex,
    InvalidExtIndex
  };

  const char *requestedExtNames[InvalidExtIndex] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DEBUG_MARKER_EXTENSION_NAME
  };

  requestedExtNames[SwapchainExtIndex] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  requestedExtNames[DebugMarkerExtIndex] = VK_EXT_DEBUG_MARKER_EXTENSION_NAME;

  DeviceExtensions requestedExt = {};
  requestedExt.count = sizeof(requestedExtNames) / sizeof(requestedExtNames[0]);
  requestedExt.names = requestedExtNames;
  requestedExt.setNeeded(SwapchainExtIndex);

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance.mInstance, &deviceCount, NULL);

  VkPhysicalDevice *devices = STACK_ALLOC(VkPhysicalDevice, deviceCount);
  vkEnumeratePhysicalDevices(instance.mInstance, &deviceCount, devices);

  LOG_INFOV("Found %d graphics devices\n", (int)deviceCount);

  DeviceExtensions usedExt = {};
    
  for (uint32_t i = 0; i < deviceCount; ++i) {
    mPhysicalDevice = devices[i];

    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceInfo);
    LOG_INFOV("\tDevice name: %s\n", mPhysicalDeviceInfo.deviceName);
        
    if (verifyHardwareMeetsRequirements(
          requestedType, surface, requiredFeatures, requestedExt, usedExt)) {
      break;
    }
    else {
      mPhysicalDevice = VK_NULL_HANDLE;
    }

    memset(&usedExt, 0, sizeof(usedExt));
  }

  if (mPhysicalDevice == VK_NULL_HANDLE) {
    LOG_ERROR("Failed to find suitable graphics device\n");
    PANIC_AND_EXIT();
  }

  vkGetPhysicalDeviceMemoryProperties(
    mPhysicalDevice, &mPhysicalDeviceMemoryInfo);

  VkFormat formats[] = {
    VK_FORMAT_D32_SFLOAT, 
    VK_FORMAT_D32_SFLOAT_S8_UINT, 
    VK_FORMAT_D24_UNORM_S8_UINT
  };

  mDepthFormat = findSuitableDepthFormat(
    formats,
    3,
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  uint32_t uniqueQueueFamilyFinder = 0;
  uniqueQueueFamilyFinder |= 1 << mQueueFamilies.graphicsFamily;
  uniqueQueueFamilyFinder |= 1 << mQueueFamilies.presentFamily;
  uint32_t uniqueQueueFamilyCount = popCount(uniqueQueueFamilyFinder);

  uint32_t *uniqueFamilyIndices = STACK_ALLOC(
    uint32_t,
    uniqueQueueFamilyCount);

  VkDeviceQueueCreateInfo *uniqueFamilyInfos = STACK_ALLOC(
    VkDeviceQueueCreateInfo,
    uniqueQueueFamilyCount);

  for (
    uint32_t bit = 0, setBits = 0;
    bit < 32 && setBits < uniqueQueueFamilyCount;
    ++bit) {
    if (uniqueQueueFamilyFinder & (1 << bit)) {
      uniqueFamilyIndices[setBits++] = bit;
    }
  }

  float priority1 = 1.0f;
  for (uint32_t i = 0; i < uniqueQueueFamilyCount; ++i) {
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = NULL;
    queueInfo.flags = 0;
    queueInfo.queueFamilyIndex = uniqueFamilyIndices[i];
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority1;
        
    uniqueFamilyInfos[i] = queueInfo;
  }

  VkDeviceCreateInfo deviceInfo = {};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pNext = NULL;
  deviceInfo.flags = 0;
  deviceInfo.queueCreateInfoCount = uniqueQueueFamilyCount;
  deviceInfo.pQueueCreateInfos = uniqueFamilyInfos;
  deviceInfo.enabledLayerCount = instance.mLayers.size;
  deviceInfo.ppEnabledLayerNames = instance.mLayers.data;
  deviceInfo.enabledExtensionCount = usedExt.count;
  deviceInfo.ppEnabledExtensionNames = usedExt.names;
  deviceInfo.pEnabledFeatures = &requiredFeatures.features;

  VkResult result = vkCreateDevice(
    mPhysicalDevice, &deviceInfo, NULL, &mLogicalDevice);

  vkGetDeviceQueue(
    mLogicalDevice, mQueueFamilies.graphicsFamily, 0, &mGraphicsQueue.mQueue);
  vkGetDeviceQueue(
    mLogicalDevice, mQueueFamilies.presentFamily, 0, &mPresentQueue.mQueue);

  if (result == VK_SUCCESS) {
    LOG_INFO("Created Vulkan logical device:\n");
    LOG_INFOV("\t* Physical device name: %s\n", mPhysicalDeviceInfo.deviceName);
    LOG_INFOV("\t* Enabled %d features\n", (int)requiredFeatures.count);
    LOG_INFOV("\t* Enabled %d extensions:\n", usedExt.count);

    for (int i = 0; i < usedExt.count; ++i) {
      LOG_INFOV("\t\t- %s\n", usedExt.names[i]);
    }
  }
    
  if (usedExt.available & 1 << DebugMarkerExtIndex) {
    LOG_INFO("Initialising debug procs\n");
    // TODO once we have a scene rendering
    // initDebugExtProcs();
  }
}

void VulkanDevice::idle() const {
  vkDeviceWaitIdle(mLogicalDevice);
}

void VulkanDevice::updateSurfaceCapabilities(const VulkanSurface &surface) {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    mPhysicalDevice, surface.mSurface, &mSwapchainSupport.capabilities);
}

bool VulkanDevice::verifyHardwareMeetsRequirements(
  DeviceType requestedType,
  const VulkanSurface &surface,
  const DeviceRequestedFeatures &requiredFeatures,
  const DeviceExtensions &requestedExtensions,
  DeviceExtensions &usedExtensions) {
  // Get queue families
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    mPhysicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties *queueProperties = STACK_ALLOC(
    VkQueueFamilyProperties, queueFamilyCount);

  vkGetPhysicalDeviceQueueFamilyProperties(
    mPhysicalDevice, &queueFamilyCount, queueProperties);

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    if (queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        queueProperties[i].queueCount > 0) {
      mQueueFamilies.graphicsFamily = i;
    }

    VkBool32 presentSupport = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(
      mPhysicalDevice, i, surface.mSurface, &presentSupport);

    if (queueProperties[i].queueCount > 0 && presentSupport) {
      mQueueFamilies.presentFamily = i;
    }

    if (mQueueFamilies.isComplete()) {
      break;
    }
  }

  uint32_t availableExtensionCount;
  vkEnumerateDeviceExtensionProperties(
    mPhysicalDevice, NULL, &availableExtensionCount, NULL);

  VkExtensionProperties *extensionProperties = STACK_ALLOC(
    VkExtensionProperties, availableExtensionCount);
  vkEnumerateDeviceExtensionProperties(
    mPhysicalDevice, NULL, &availableExtensionCount, extensionProperties);

  uint32_t requiredExtBits = 0;

  uint32_t requiredExtensionsLeft = requestedExtensions.count;
  for (
    uint32_t i = 0;
    i < availableExtensionCount && requiredExtensionsLeft > 0;
    ++i) {

    for (uint32_t j = 0; j < requestedExtensions.count; ++j) {
      if (!strcmp(
            extensionProperties[i].extensionName,
            requestedExtensions.names[j])) {
        requiredExtBits |= 1 << j;
        usedExtensions.available |= 1 << j;
        --requiredExtensionsLeft;
      }
    }

  }
    
  bool canRun = 1;

  usedExtensions.count = requestedExtensions.count - requiredExtensionsLeft;
  usedExtensions.names = lnAllocv<const char *>(usedExtensions.count);

  uint32_t used_counter = 0;

  // Make sure it isn't a needed extension.
  for (uint32_t i = 0; i < requestedExtensions.count; ++i) {
    bool isFound = requiredExtBits & (1 << i);
    bool isNeeded = requestedExtensions.isNeeded(i);
    if (isFound) {
      usedExtensions.names[used_counter++] = requestedExtensions.names[i];
    }
    else if (isNeeded) {
      LOG_ERROR("Could not find a critical extension\n");
      canRun = 0;
    }
    else {
      LOG_WARNINGV(
        "Couldn't find device extension: %s\n",
        requestedExtensions.names[i]);
    }
  }

  bool isSwapchainSupported = canRun;

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(mPhysicalDevice, &deviceFeatures);

  bool isSwapchainUsable = 0;
  if (isSwapchainSupported) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      mPhysicalDevice, surface.mSurface, &mSwapchainSupport.capabilities);
        
    vkGetPhysicalDeviceSurfaceFormatsKHR(
      mPhysicalDevice,
      surface.mSurface,
      &mSwapchainSupport.availableFormatsCount,
      NULL);

    if (mSwapchainSupport.availableFormatsCount != 0) {
      mSwapchainSupport.availableFormats = flAllocv<VkSurfaceFormatKHR>(
        mSwapchainSupport.availableFormatsCount);

      vkGetPhysicalDeviceSurfaceFormatsKHR(
        mPhysicalDevice,
        surface.mSurface,
        &mSwapchainSupport.availableFormatsCount,
        mSwapchainSupport.availableFormats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(
      mPhysicalDevice, surface.mSurface,
      &mSwapchainSupport.availablePresentModesCount, NULL);

    if (mSwapchainSupport.availablePresentModesCount != 0) {
      mSwapchainSupport.availablePresentModes = flAllocv<VkPresentModeKHR>(
        mSwapchainSupport.availablePresentModesCount);

      vkGetPhysicalDeviceSurfacePresentModesKHR(
        mPhysicalDevice, surface.mSurface,
        &mSwapchainSupport.availablePresentModesCount,
        mSwapchainSupport.availablePresentModes);
    }

    isSwapchainUsable = mSwapchainSupport.availableFormatsCount &&
      mSwapchainSupport.availablePresentModesCount;
  }

  bool isDeviceTypeSatisfied = false;
  switch (requestedType) {
  case DeviceType::DiscreteGPU: {
    isDeviceTypeSatisfied =
      deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  } break;

  case DeviceType::Integrated: {
    isDeviceTypeSatisfied =
      deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
  } break;

  case DeviceType::Any: {
    isDeviceTypeSatisfied = true;
  } break;
  }

  return isSwapchainSupported && isSwapchainUsable &&
    isDeviceTypeSatisfied &&
    mQueueFamilies.isComplete() &&
    checkRequiredFeaturesSupport(requiredFeatures, deviceFeatures);
}

VkFormat VulkanDevice::findSuitableDepthFormat(
  VkFormat *formats,
  uint32_t formatCount,
  VkImageTiling tiling,
  VkFormatFeatureFlags features) const {
  for (uint32_t i = 0; i < formatCount; ++i) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, formats[i], &properties);

    if (tiling == VK_IMAGE_TILING_LINEAR && 
        (properties.linearTilingFeatures & features) == features) {
      return(formats[i]);
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && 
             (properties.optimalTilingFeatures & features) == features) {
      return(formats[i]);
    }
  }

  LOG_ERROR("Found no depth formats!\n");
  PANIC_AND_EXIT();

  return VkFormat(0);
}

bool VulkanDevice::checkRequiredFeaturesSupport(
  const DeviceRequestedFeatures &required,
  const VkPhysicalDeviceFeatures &available) {
  uint32_t featureCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
  VkBool32 *startRequired = (VkBool32 *)&required;
  VkBool32 *startAvailable = (VkBool32 *)&available;
  
  uint32_t foundFeatures = 0;
  for (int i = 0; i < featureCount && foundFeatures < required.count; ++i) {
    if (startRequired[i]) {
      if (startAvailable[i]) {
        ++foundFeatures;
      }
      else {
        return false;
      }
    }
  }
  
  return true;
}

uint32_t VulkanDevice::findMemoryType(
  VkMemoryPropertyFlags properties,
  VkMemoryRequirements &memoryRequirements) const {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if (
      memoryRequirements.memoryTypeBits & (1 << i) &&
      (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return(i);
    }
  }

  LOG_ERROR("Unable to find memory type!\n");
  PANIC_AND_EXIT();

  return 0;
}

VkDeviceMemory VulkanDevice::allocateImageMemory(
  VkImage image, VkMemoryPropertyFlags properties) const {
  VkMemoryRequirements requirements = {};
  vkGetImageMemoryRequirements(mLogicalDevice, image, &requirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = requirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(properties, requirements);

  VkDeviceMemory memory;
  vkAllocateMemory(mLogicalDevice, &allocInfo, nullptr, &memory);

  vkBindImageMemory(mLogicalDevice, image, memory, 0);

  return memory;
}

VkDeviceMemory VulkanDevice::allocateBufferMemory(
  VkBuffer buffer, VkMemoryPropertyFlags properties) const {
  VkMemoryRequirements requirements = {};
  vkGetBufferMemoryRequirements(mLogicalDevice, buffer, &requirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = requirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(properties, requirements);

  VkDeviceMemory memory;
  vkAllocateMemory(mLogicalDevice, &allocInfo, nullptr, &memory);

  vkBindBufferMemory(mLogicalDevice, buffer, memory, 0);

  return memory;
}

const VulkanQueue &VulkanDevice::graphicsQueue() const {
  return mGraphicsQueue;
}

const VulkanQueue &VulkanDevice::presentQueue() const {
  return mPresentQueue;
}

}
