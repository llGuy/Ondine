#include "yona_app.hpp"

namespace Yona {

Application::Application() {
  /* Initialise graphics context, etc... */
}

Application::~Application() {
  /* Destroy graphics context, etc... */
}

void Application::run() {
  /* User-defined function which will be overriden */
  start();

  while (mIsRunning) {
    /* Go through all pushed events (in future go through layer stack) */
    mEventQueue.beginProcessing();
    Event *ev = mEventQueue.getNextEvent();
    while (ev) {
      /* Handle */

      ev = mEventQueue.getNextEvent();
    }
    mEventQueue.endProcessing();
  }

  mEventQueue.clearEvents();
}

void Application::recvEvent(Event *ev, void *obj) {
  Application *app = (Application *)obj;
  app->pushEvent(ev);
}

void Application::pushEvent(Event *ev) {
  mEventQueue.push(ev);
}

}
