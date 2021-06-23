#pragma once

#include <stdint.h>

namespace Ondine::Core {

struct Tick {
  float dt;
  // Reset? at some point
  float accumulatedTime;
};

}
