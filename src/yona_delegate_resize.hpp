#pragma once

#include "yona_io.hpp"

namespace Yona {

class VulkanContext;

class DelegateResize {
public:
  virtual void resize(
    VulkanContext &vulkanContext, Resolution newResolution) = 0;
};

}
