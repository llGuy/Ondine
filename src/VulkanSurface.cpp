#include "VulkanSurface.hpp"
#include "VulkanInstance.hpp"

namespace Ondine {

void VulkanSurface::init(
  const VulkanInstance &instance,
  const WindowContextInfo &info) {
  info.surfaceCreateProc(
    instance.mInstance,
    &mSurface, info.handle);
}

}
