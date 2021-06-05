#include "VulkanSurface.hpp"
#include "VulkanInstance.hpp"

namespace Ondine::Graphics {

void VulkanSurface::init(
  const VulkanInstance &instance,
  const Core::WindowContextInfo &info) {
  info.surfaceCreateProc(
    instance.mInstance,
    &mSurface, info.handle);
}

}
