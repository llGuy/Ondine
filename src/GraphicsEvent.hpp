#pragma once

#include "IO.hpp"
#include "Event.hpp"

namespace Ondine::Core {

struct EventViewportResize : Event {
  EVENT_DEF(EventViewportResize, Graphics, ViewportResize);

  Resolution newResolution;
};

}
