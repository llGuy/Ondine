#pragma once

#include "IO.hpp"
#include "Event.hpp"

namespace Ondine {

struct EventViewportResize : Event {
  EVENT_DEF(EventViewportResize, Graphics, ViewportResize);

  Resolution newResolution;
};

}