#pragma once

#include "IO.hpp"

namespace Yona {

class VulkanContext;

class DelegateResize {
public:
  virtual void resize(Resolution newResolution) = 0;
};

}
