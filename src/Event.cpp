#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Utils.hpp"
#include "Event.hpp"

namespace Ondine {

EventQueue::EventQueue()
  : mProcessingCounter(0),
    mEventCount(0),
    mEvents{0} {
  
}

void EventQueue::push(Event *ev) {
  // assert(mEventCount < MAX_EVENTS_PER_FRAME_COUNT - 1);
  if (mEventCount < MAX_EVENTS_PER_FRAME_COUNT - 1) {
    mEvents[mEventCount++] = ev;
  }
}

Event *EventQueue::beginProcessing() {
  mProcessingCounter = 0;
  mEvents[mEventCount] = nullptr;
  return mEvents[0];
}

Event *EventQueue::getNextEvent() {
  return mEvents[++mProcessingCounter];
}

void EventQueue::endProcessing() {
  Event **tmp = STACK_ALLOC(Event *, mEventCount);
  size_t eventsLeft = 0;

  for (size_t i = 0; i < mEventCount; ++i) {
    if (!mEvents[i]->isHandled) {
      tmp[eventsLeft++] = mEvents[i];
    }
  }

  memcpy(mEvents, tmp, sizeof(Event *) * eventsLeft);
  mEventCount = eventsLeft;

  mProcessingCounter = 0;
}

void EventQueue::clearEvents() {
  mEventCount = 0;
}

}
