#include "VulkanSurface.hpp"
#include "VulkanInstance.hpp"

namespace Yona {

void VulkanSurface::init(
  const VulkanInstance &instance,
  const WindowContextInfo &info) {
  info.surfaceCreateProc(
    instance.mInstance,
    &mSurface, info.handle);
}

}
