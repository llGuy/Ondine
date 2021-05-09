#pragma once

#include "yona_event.hpp"

namespace Yona {

class Application {
public:
  Application();
  virtual ~Application();

  void run();

  static void recvEvent(Event *ev, void *app);

private:
  /* Need to be defined in a Client / Editor application */
  virtual void start() = 0;
  virtual void tick() = 0;

  void pushEvent(Event *ev);

private:
  bool mIsRunning;
  EventQueue mEventQueue;
};

}
