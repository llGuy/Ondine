#pragma once

#include "IO.hpp"

namespace Ondine {

class VulkanContext;

class DelegateResize {
public:
  virtual void resize(Resolution newResolution) = 0;
};

class DelegateTrackInput {
public:
  virtual void trackInput(
    const Tick &tick,
    const InputTracker &inputTracker) = 0;
};

}
