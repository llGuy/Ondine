#pragma once

#include "yona_io.hpp"
#include "yona_event.hpp"

namespace Yona {

struct EventViewportResize : Event {
  EVENT_DEF(EventViewportResize, Renderer3D, ViewportResize);

  Resolution newResolution;
};

}
