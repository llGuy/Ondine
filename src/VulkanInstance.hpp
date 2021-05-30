#pragma once

#include <string_view>
#include <vulkan/vulkan.h>
#include "Buffer.hpp"

namespace Yona {

class VulkanInstance {
public:
  VulkanInstance(bool enableValidation);

  void init();

private:
  void verifyValidationSupport(
    /* Reads from  */
    const Array<const char *> &requested,
    /* Writes to */
    Array<const char *> &enabled) const;

  void initDebugUtils();

  static VKAPI_ATTR VkBool32 VKAPI_PTR debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
    void *);

private:
  VkInstance mInstance;
  bool mIsValidationEnabled;
  uint32_t mValidationCount;
  Array<const char *> mLayers;
  VkDebugUtilsMessengerEXT mDebugMessenger;

  friend class VulkanContext;
  friend class VulkanDevice;
  friend class VulkanSurface;
  friend class VulkanImgui;
};

}
