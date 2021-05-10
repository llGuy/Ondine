#include "yona_log.hpp"
#include "yona_app.hpp"
#include "yona_memory.hpp"
#include "yona_filesystem.hpp"

namespace Yona {

Application::Application(int argc, char **argv)
  : mWindow(WindowMode::Windowed, "Yona") {
  /* Initialise graphics context, etc... */
}

Application::~Application() {
  /* Destroy graphics context, etc... */
}

void Application::run() {
  /* Change YONA_PROJECT_ROOT depending on build type */
  gFileSystem->addMountPoint(
    (MountPoint)ApplicationMountPoints::Application,
    YONA_PROJECT_ROOT);

  gFileSystem->addMountPoint(
    (MountPoint)ApplicationMountPoints::Raw,
    "");

  mWindow.init(RECV_EVENT_PROC(recvEvent));

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;

  while (mIsRunning) {
    mWindow.pollInput();

    tick();

    /* Go through all pushed events (in future go through layer stack) */
    mEventQueue.process([this](Event *ev) {
      LOG_INFO("Processing event\n");
    });

    mEventQueue.clearEvents();
    /* Clears global linear allocator */
    lnClear();
  }
}

void Application::recvEvent(Event *ev, void *obj) {
  Application *app = (Application *)obj;
  app->pushEvent(ev);
}

void Application::pushEvent(Event *ev) {
  mEventQueue.push(ev);
}

}
