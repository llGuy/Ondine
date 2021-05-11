#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"

namespace Yona {

void VulkanSurface::init(
  const VulkanInstance &instance,
  const WindowContextInfo &info) {
  info.surfaceCreateProc(
    instance.mInstance,
    &mSurface, info.handle);
}

}
