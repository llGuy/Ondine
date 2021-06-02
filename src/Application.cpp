#include "Log.hpp"
#include "Application.hpp"
#include "Memory.hpp"
#include "IOEvent.hpp"
#include "GameView.hpp"
#include "FileSystem.hpp"
#include "EditorView.hpp"

namespace Yona {

Application::Application(int argc, char **argv)
  : mWindow(WindowMode::Windowed, "Yona"),
    mRenderer3D(mGraphicsContext, mInputTracker),
    mViewStack(mGraphicsContext) {
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

  mGraphicsContext.initInstance();

  Window::initWindowAPI();
  auto surfaceInfo = mWindow.init(RECV_EVENT_PROC(recvEvent));

  mGraphicsContext.initContext(surfaceInfo);
  mRenderer3D.init();
  mViewStack.init();

  mViewStack.push(
    new GameView(mRenderer3D.mainRenderStage(), mRenderer3D));
  mViewStack.push(
    new EditorView(surfaceInfo, mGraphicsContext, RECV_EVENT_PROC(recvEvent)));

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;
  mDt = 0.0f;

  while (mIsRunning) {
    Time::TimeStamp frameStart = Time::getCurrentTime();
    Tick currentTick = { mDt };

    mInputTracker.tick(currentTick);
    mWindow.pollInput();

    /* 
       Go through the core events (window resize, etc...), then offload 
       to the different views.
    */
    mEventQueue.process([this](Event *ev) {
      switch (ev->category) {
      case EventCategory::Input: {
        processInputEvent(ev);
      } break;

      case EventCategory::Graphics: {
        processGraphicsEvent(ev);
      } break;

      default:; /* Only handle the above */
      }
    });

    mViewStack.processEvents(mEventQueue, currentTick);
    mEventQueue.clearEvents();

    /* Clears global linear allocator */
    lnClear();

    // Defined in client
    tick();

    VulkanFrame frame = mGraphicsContext.beginFrame();
    if (!frame.skipped) { // All rendering here
      mRenderer3D.tick(currentTick, frame);
      mViewStack.render(frame, currentTick);

      mGraphicsContext.beginSwapchainRender(frame);
      {
        // Grab output of the view stack
        mViewStack.presentOutput(frame);
      }
      mGraphicsContext.endSwapchainRender(frame);

      mGraphicsContext.endFrame(frame);
    }

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

  case EventType::Resize: {
    auto *resizeEvent = (EventResize *)ev;
    mGraphicsContext.resize(resizeEvent->newResolution);

    // Don't set it to handled - view stack will want to have a look at this
  } break;

    /* Just for fullscreen toggling */
  case EventType::Keyboard: {
    auto *kbEvent = (EventKeyboard *)ev;
    mInputTracker.handleKeyboardEvent(kbEvent);

    if (kbEvent->keyboardEventType == KeyboardEventType::Press) {
      if (kbEvent->press.button == KeyboardButton::F11 &&
          !kbEvent->press.isRepeat) {
        mWindow.toggleFullscreen();
        mGraphicsContext.skipFrame();
      }
    }

    kbEvent->isHandled = true;
  } break;

  case EventType::Mouse: {
    auto *event = (EventMouse *)ev;
    mInputTracker.handleMouseEvent(event);

    event->isHandled = true;
  } break;

  default:;
  }
}

void Application::processGraphicsEvent(Event *ev) {
  switch (ev->type) {
  default:;
  }
}

void Application::setMaxFramerate(float fps) {
  mMaxFramerate = fps;
  mMinFrametime = 1.0f / mMaxFramerate;
}

}
