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
  gFileSystem->addMountPoint(
    (MountPoint)ApplicationMountPoints::Application,
    YONA_PROJECT_ROOT);

  gFileSystem->addMountPoint(
    (MountPoint)ApplicationMountPoints::Raw,
    "");

  File dummyFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/dummy.txt",
    FileOpenType::Text | FileOpenType::In);

  auto contents = dummyFile.readText();

  LOG_INFOV("Contents of res/dummy.txt:\n%s\n", contents.c_str());

  File otherDummyFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/other_dummy.txt",
    FileOpenType::Text | FileOpenType::Out);

  std::string otherDummyContents = "Here is the other dummy\nCoool!\n";
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
