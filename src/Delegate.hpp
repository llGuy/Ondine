#pragma once

#include "IO.hpp"

namespace Yona {

class VulkanContext;

class DelegateResize {
public:
  virtual void resize(Resolution newResolution) = 0;
};

class DelegateRecvEvent {
public:
  virtual void recvEvent(Event *ev) = 0;
};

}
