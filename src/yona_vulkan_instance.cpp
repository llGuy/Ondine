#include <string.h>
#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_vulkan.hpp"
#include "yona_vulkan_instance.hpp"

namespace Yona {

VulkanInstance::VulkanInstance(bool enableValidation)
  : mIsValidationEnabled(enableValidation) {

}

void VulkanInstance::init() {
  if (mIsValidationEnabled) {
    static const char *VALIDATION_LAYERS[] = {
      "VK_LAYER_KHRONOS_validation"
    };

    size_t requestedCount = sizeof(VALIDATION_LAYERS) /
      sizeof(VALIDATION_LAYERS[0]);

    mLayers.init(requestedCount);
    verifyValidationSupport(
      Array(VALIDATION_LAYERS, requestedCount),
      mLayers);
  }
  else {
    mLayers.init(nullptr, 0);
  }

  /* Required extensions */
  uint32_t extensionCount = 4;
  const char *extensions[] = {
#if defined(_WIN32)
    "VK_KHR_win32_surface",
#elif defined(__ANDROID__)
    "VK_KHR_android_surface"
#else
    "VK_KHR_xcb_surface",
#endif
    "VK_KHR_surface",
#ifndef NDEBUG
    "VK_EXT_debug_utils",
    "VK_EXT_debug_report"
#endif
  };

  VkApplicationInfo applicationInfo = {};
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pNext = nullptr;
  applicationInfo.pApplicationName = "NULL";
  applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInfo.pEngineName = "";
  applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instanceInfo = {};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pNext = nullptr;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledLayerCount = mLayers.size;
  instanceInfo.ppEnabledLayerNames = mLayers.data;
  instanceInfo.enabledExtensionCount = extensionCount;
  instanceInfo.ppEnabledExtensionNames = extensions;

  VkResult result = vkCreateInstance(&instanceInfo, NULL, &mInstance);
  if (result == VK_SUCCESS) {
    LOG_INFO("Created Vulkan instance:\n");
    LOG_INFO("\tEnabled validation layers:\n");
    for (int i = 0; i < mLayers.size; ++i) {
      LOG_INFOV("\t\t- %s\n", mLayers.data[i]);
    }
    LOG_INFO("\tUsing instance extensions:\n");
    for (int i = 0; i < extensionCount; ++i) {
      LOG_INFOV("\t\t- %s\n", extensions[i]);
    }
    
    if (mIsValidationEnabled) {
      initDebugMessengerCallback();
    }
  }
  else {
    LOG_ERRORV("Failed to create Vulkan instance with code: %d\n", (int)result);
    PANIC_AND_EXIT();
  }
}

void VulkanInstance::verifyValidationSupport(
  const Array<const char *> &requestedLayers,
  Array<const char *> &enabled) const {
  /* Get supported validation layers */
  uint32_t supportedLayerCount = 0;
  vkEnumerateInstanceLayerProperties(&supportedLayerCount, NULL);
  auto *supportedLayers = STACK_ALLOC(VkLayerProperties, supportedLayerCount);
  vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers);
    
  uint32_t requestedVerifiedCount = 0;

  for (uint32_t requested = 0; requested < requestedLayers.size; ++requested) {
    bool found = false;

    for (uint32_t avail = 0; avail < supportedLayerCount; ++avail) {
      if (!strcmp(
            supportedLayers[avail].layerName,
            requestedLayers[requested])) {
        enabled[requestedVerifiedCount++] = requestedLayers[requested];
        found = true;
        break;
      }
    }

    if (!found) {
      LOG_ERRORV(
        "Validation layer %s not supported\n",
        requestedLayers[requested]);
    }
  }

  enabled.size = requestedVerifiedCount;
}

void VulkanInstance::initDebugMessengerCallback() {
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
  debugMessengerInfo.sType = 
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  debugMessengerInfo.pNext = NULL;
  debugMessengerInfo.flags = 0;

  debugMessengerInfo.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  debugMessengerInfo.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  debugMessengerInfo.pfnUserCallback = &debugMessengerCallback;
  debugMessengerInfo.pUserData = NULL;

  auto ptr_vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");

  if (ptr_vkCreateDebugUtilsMessenger(
        mInstance,
        &debugMessengerInfo,
        NULL,
        &mDebugMessenger)) {
    LOG_ERROR("Failed to create debug utils messenger\n");
  }
}

VKAPI_ATTR VkBool32 VKAPI_PTR VulkanInstance::debugMessengerCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
  void *) {
  if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ||
      messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    LOG_ERRORV(
      "Validation layer (%d;%d): %s\n",
      messageType,
      severity,
      callbackData->pMessage);
  }
  return 0;
}

}
