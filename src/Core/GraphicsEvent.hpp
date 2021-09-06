#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Buffer.hpp"

namespace Ondine::Core {

struct EventViewportResize : Event {
  EVENT_DEF(EventViewportResize, Graphics, ViewportResize);

  Resolution newResolution;
};

struct EventViewHierarchyChange : Event {
  EVENT_DEF(EventViewHierarchyChange, Graphics, ViewHierarchyChange);

  Array<const char *, AllocationType::Linear> views;
};

}
