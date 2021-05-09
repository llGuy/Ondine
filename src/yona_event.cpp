#include <assert.h>
#include "yona_event.hpp"

namespace Yona {

EventQueue::EventQueue()
  : mProcessingCounter(0),
    mEventCount(0),
    mEvents{0} {
  
}

void EventQueue::push(Event *ev) {
  assert(mEventCount < MAX_EVENTS_PER_FRAME_COUNT - 1);
  mEvents[mEventCount++] = ev;
}

void EventQueue::beginProcessing() {
  mProcessingCounter = 0;
}

Event *EventQueue::getNextEvent() {
  return mEvents[mProcessingCounter++];
}

void EventQueue::endProcessing() {
  for (size_t i = 0; i < mEventCount; ++i) {
    if (mEvents[i]->isHandled) {
      mEvents[i] = mEvents[mEventCount - 1];
      --mEventCount;
    }
  }

  mProcessingCounter = 0;
}

void EventQueue::clearEvents() {
  mEventCount = 0;
}

}
