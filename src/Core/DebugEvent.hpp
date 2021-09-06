#pragma once

#include "Event.hpp"

namespace Ondine::Core {

struct EventBreakpoint : Event {
  EVENT_DEF(EventBreakpoint, Debug, Breakpoint);
};

}
