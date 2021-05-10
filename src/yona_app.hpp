#pragma once

#include "yona_event.hpp"
#include "yona_window.hpp"
#include "yona_filesystem.hpp"

namespace Yona {

enum class ApplicationMountPoints : uint8_t {
  Application,
  Raw
};

class Application {
public:
  Application(int argc, char **argv);
  virtual ~Application();

  /* Core */
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
  Window mWindow;
};

}
