#pragma once

#include <utility>
#include <stddef.h>
#include <stdint.h>

namespace Yona {

enum class EventCategory {
  Input,
  Gameplay
};

enum class EventType {
  Mouse,
  Keyboard,
  Resize,
  Close
};

#define EVENT_DEF(name, category, type)                                 \
  name() : Event::Event(EventType:: type, EventCategory:: category) {}  \
  const char *getName() override {return #name;}

struct Event {
  bool isHandled;
  EventType type;
  EventCategory category;

  Event(EventType evType, EventCategory evCategory)
    : isHandled(false), type(evType), category(evCategory) {
    
  }

#ifndef NDEBUG
  virtual const char *getName() = 0;
#endif
};

/* Avoid std::function */
class OnEventProc {
public:
  using Callback = void(*)(Event *ev, void *obj);

  OnEventProc() = default;

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

#define RECV_EVENT_PROC(proc) OnEventProc(proc, this)

class EventQueue {
public:
  EventQueue();

  void push(Event *ev);

  /* Background functions used in process function */
  Event *beginProcessing();
  Event *getNextEvent();
  void endProcessing();

  template <typename T>
  void process(const T &proc) {
    /* Go through all pushed events (in future go through layer stack) */
    Event *ev = beginProcessing();
    while (ev) {
      /* Handle */
      proc(ev);

      ev = getNextEvent();
    }
    endProcessing();
  }

  void clearEvents();
private:
  static constexpr uint32_t MAX_EVENTS_PER_FRAME_COUNT = 30;
  /* Events allocated on linear allocator */
  Event *mEvents[MAX_EVENTS_PER_FRAME_COUNT];
  uint32_t mEventCount;
  uint32_t mProcessingCounter;
};

}
