#pragma once

#include <stdint.h>

namespace Yona {

struct Event {
  bool isHandled;
};

class EventQueue {
public:
  EventQueue();

private:
  static constexpr uint32_t MAX_EVENTS_PER_FRAME_COUNT = 30;
  /* Events allocated on linear allocator */
  Event *mEvents[MAX_EVENTS_PER_FRAME_COUNT];
};

}
