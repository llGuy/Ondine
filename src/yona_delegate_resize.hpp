#pragma once

#include "yona_io.hpp"

namespace Yona {

class VulkanContext;

class DelegateResize {
public:
  virtual void resize(Resolution newResolution) = 0;
};

}
