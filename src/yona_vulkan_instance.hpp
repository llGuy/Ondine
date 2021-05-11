#pragma once

#include <string_view>
#include <vulkan/vulkan.h>
#include "yona_buffer.hpp"

namespace Yona {

class VulkanInstance {
public:
  VulkanInstance(bool enableValidation);

  void init(const std::string_view &appName);

private:
  VkInstance mInstance;
  bool mIsValidationEnabled;
  uint32_t mValidationCount;
  Array<std::string_view> mLayers;

  friend class VulkanContext;
};

}
