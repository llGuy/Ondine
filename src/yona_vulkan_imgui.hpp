#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

class VulkanImgui {
public:
  void init();

private:
  static void imguiCallback(VkResult result);

private:
  // Store debug procs
};

}
