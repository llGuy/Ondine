#include "yona_log.hpp"
#include "yona_app.hpp"
#include "yona_memory.hpp"
#include "yona_filesystem.hpp"

namespace Yona {

Application::Application(int argc, char **argv) {
  /* Initialise graphics context, etc... */
}

Application::~Application() {
  /* Destroy graphics context, etc... */
}

void Application::run() {
  /* Change YONA_PROJECT_ROOT depending on build type */
  gFileSystem->addMountPoint(APPLICATION_MOUNT_POINT, YONA_PROJECT_ROOT);

  File dummyFile = gFileSystem->createFile(
    APPLICATION_MOUNT_POINT,
    "res/dummy.txt",
    FileOpenType::Text | FileOpenType::In);

  auto contents = dummyFile.readText();

  File otherDummyFile = gFileSystem->createFile(
    APPLICATION_MOUNT_POINT,
    "res/other_dummy.txt",
    FileOpenType::Text | FileOpenType::Out);

  std::string otherDummyContents = "Here is the other dummy\nCool!\n";
  otherDummyFile.write(otherDummyContents.c_str(), otherDummyContents.length());

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;

  auto submitEventProc = RECV_EVENT_PROC(recvEvent);

  while (mIsRunning) {
    tick();

    submitEventProc(
      lnEmplaceAlloc<Event>(false, EventType::Dummy0, "Event0"));

    submitEventProc(
      lnEmplaceAlloc<Event>(false, EventType::Dummy1, "Event1"));

    /* Go through all pushed events (in future go through layer stack) */
    mEventQueue.process([this](Event *ev) {
      LOG_INFOV("Processing event %s\n", ev->name);

      if (ev->type == EventType::Dummy0)
        ev->isHandled = 1;
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
