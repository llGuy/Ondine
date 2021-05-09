#pragma once

#include <utility>
#include <stddef.h>
#include <stdint.h>

namespace Yona {

struct Event {
  bool isHandled;
};

/* Avoid std::function */
class OnEventProc {
public:
  using Callback = void(*)(Event *ev, void *obj);

  OnEventProc(Callback proc, void *obj)
    : mProc(proc), mObj(obj) {
    
  }

  inline void operator()(Event *ev) {
    mProc(ev, mObj);
  }

private:
  Callback mProc;
  void *mObj;
};

class EventQueue {
public:
  EventQueue();

  void push(Event *ev);

  void beginProcessing();
  Event *getNextEvent();
  void endProcessing();

  void clearEvents();
private:
  static constexpr uint32_t MAX_EVENTS_PER_FRAME_COUNT = 30;
  /* Events allocated on linear allocator */
  Event *mEvents[MAX_EVENTS_PER_FRAME_COUNT];
  uint32_t mEventCount;
  uint32_t mProcessingCounter;
};

}
