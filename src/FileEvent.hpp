#pragma once

#include "Event.hpp"
#include "FileSystem.hpp"

namespace Ondine::Core {

struct EventPathChanged : Event {
  EVENT_DEF(EventPathChanged, File, PathChanged);

  const char *path;
  TrackPathID id;
};

}
