#include "yona_vulkan_instance.hpp"

namespace Yona {

VulkanInstance::VulkanInstance(bool enableValidation)
  : mIsValidationEnabled(enableValidation) {
  if (mIsValidationEnabled) {
    static const std::string_view VALIDATION_LAYERS[] = {
      "VK_LAYER_KHRONOS_validation"
    };

    mLayers.init(sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]));
    for (int i = 0; i < mLayers.count; ++i) {
      mLayers[i] = VALIDATION_LAYERS[i];
    }
  }
}

void VulkanInstance::init(const std::string_view &appName) {
  
}

}
