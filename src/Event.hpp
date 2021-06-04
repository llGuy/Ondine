#pragma once

#include <utility>
#include <stddef.h>
#include <stdint.h>

namespace Ondine {

enum class EventCategory {
  Input,
  Graphics,
  Gameplay
};

enum class EventType {
  Mouse,
  Keyboard,
  Resize,
  Close,
  ViewportResize,
  CursorDisplayChange,
  Invalid
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

  inline void operator()(Event *ev) const {
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

// Data needs to be some sort of union which doesn't call linear allocations
template <typename Params, typename Data, int MaxFunctors = 20>
class DeferredEventProcessor {
public:
  using ProcParams = Params;
  using EventData = Data;
  using FunctorProc = void (*)(ProcParams &, EventData &);

  DeferredEventProcessor()
    : mDeferredEventCount(0) {
    
  }

  void push(FunctorProc proc, const EventData &data) {
    mFunctors[mDeferredEventCount].proc = proc;
    mFunctors[mDeferredEventCount].data = data;
    ++mDeferredEventCount;
  }

  void process(ProcParams &params) {
    for (int i = 0; i < mDeferredEventCount; ++i) {
      mFunctors[i](params);
    }

    mDeferredEventCount = 0;
  }

private:
  struct DeferredFunctor {
    void (*proc)(ProcParams &params, EventData &eventData);
    EventData data;

    void operator()(ProcParams &params) {
      proc(params, data);
    }
  };

private:
  DeferredFunctor mFunctors[MaxFunctors];
  uint32_t mDeferredEventCount;
};

}
