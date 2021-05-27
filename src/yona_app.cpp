#include "yona_log.hpp"
#include "yona_app.hpp"
#include "yona_memory.hpp"
#include "yona_io_event.hpp"
#include "yona_filesystem.hpp"

namespace Yona {

Application::Application(int argc, char **argv)
  : mWindow(WindowMode::Windowed, "Yona") {
  /* Initialise graphics context, etc... */
  setMaxFramerate(60.0f);
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

  mVulkanContext.initInstance();

  Window::initWindowAPI();
  auto surfaceInfo = mWindow.init(RECV_EVENT_PROC(recvEvent));

  mVulkanContext.initContext(surfaceInfo);

  mRenderer.init(mVulkanContext);

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;
  mDt = 0.0f;

  while (mIsRunning) {
    Time::TimeStamp frameStart = Time::getCurrentTime();
    Tick currentTick = { mDt };

    mWindow.pollInput();

    tick();

    /* Go through all pushed events (in future go through layer stack) */
    mEventQueue.process([this](Event *ev) {
      switch (ev->category) {
      case EventCategory::Input: {
        processInputEvent(ev);
      } break;

      default:; /* Only handle the above */
      }
    });

    VulkanFrame frame = mVulkanContext.beginFrame();
    { // All rendering here
      mVulkanContext.beginSwapchainRender(frame);

      // Render will do final rendering to this backbuffer
      mRenderer.tick(currentTick, frame);

      mVulkanContext.endSwapchainRender(frame);
    }
    mVulkanContext.endFrame(frame);

    mEventQueue.clearEvents();
    /* Clears global linear allocator */
    lnClear();

    Time::TimeStamp frameEnd = Time::getCurrentTime();
    mDt = Time::getTimeDifference(frameEnd, frameStart);

    if (mDt < mMinFrametime) {
      Time::sleepSeconds(mMinFrametime - mDt);
      mDt = mMinFrametime;
    }
  }

  /* Shutdown */
}

void Application::recvEvent(Event *ev, void *obj) {
  Application *app = (Application *)obj;
  app->pushEvent(ev);
}

void Application::pushEvent(Event *ev) {
  mEventQueue.push(ev);
}

void Application::processInputEvent(Event *ev) {
  switch (ev->type) {
  case EventType::Close: {
    mIsRunning = false;
    ev->isHandled = true;
  } break;

    /* For debugging */
#if 0
  case EventType::Mouse: {
    EventMouse *mouseEv = (EventMouse *)ev;
    switch (mouseEv->mouseEventType) {
    case MouseEventType::Press: {
      LOG_INFOV("Mouse button press: %d\n", (int)mouseEv->press.button);
    } break;

    case MouseEventType::Release: {
      LOG_INFOV("Mouse button release: %d\n", (int)mouseEv->release.button);
    } break;

    case MouseEventType::Move: {
      LOG_INFOV("Mouse button move: %f %f\n", mouseEv->move.x, mouseEv->move.y);
    } break;

    case MouseEventType::Scroll: {
      LOG_INFOV
        ("Mouse button scroll: %f %f\n", mouseEv->scroll.x, mouseEv->scroll.y);
    } break;
    }
  } break;

  case EventType::Keyboard: {
    EventKeyboard *kbEv = (EventKeyboard *)ev;
    switch (kbEv->keyboardEventType) {
    case KeyboardEventType::Press: {
      LOG_INFOV("Keyboard button press: %d\n", (int)kbEv->press.button);
    } break;

    case KeyboardEventType::Release: {
      LOG_INFOV("Keyboard button release: %d\n", (int)kbEv->release.button);
    } break;

    case KeyboardEventType::Type:;
    }
  } break;
#endif

  default:;
  }
}

void Application::setMaxFramerate(float fps) {
  mMaxFramerate = fps;
  mMinFrametime = 1.0f / mMaxFramerate;
}

}
