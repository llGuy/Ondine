#include "Log.hpp"
#include "Memory.hpp"
#include "IOEvent.hpp"
#include "GameView.hpp"
#include "FileEvent.hpp"
#include "FileSystem.hpp"
#include "EditorView.hpp"
#include "DebugEvent.hpp"
#include "Application.hpp"

namespace Ondine::Core {

Application::Application(int argc, char **argv)
  : mWindow(WindowMode::Windowed, "Ondine"),
    mRenderer3D(mGraphicsContext),
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
  auto evProc = RECV_EVENT_PROC(recvEvent);
  auto surfaceInfo = mWindow.init(evProc);

  mGraphicsContext.initContext(surfaceInfo);
  mRenderer3D.init();
  mViewStack.init();

  mViewStack.push(
    new View::GameView(
      mRenderer3D.mainRenderStage(), mRenderer3D, mRenderer3D, evProc));

  mViewStack.push(
    new View::EditorView(
      surfaceInfo, mGraphicsContext, mRenderer3D, evProc));

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;
  mDt = 0.0f;

  float accumulatedTime = 0.0f;

  while (mIsRunning) {
    TimeStamp frameStart = getCurrentTime();
    Tick currentTick = { mDt, accumulatedTime };

    mInputTracker.tick(currentTick);

    mWindow.pollInput();
    gFileSystem->trackFiles(evProc);

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

      case EventCategory::Debug: {
        processDebugEvent(ev);
      } break;

      case EventCategory::File: {
        processFileEvent(ev);
      } break;

      default:; /* Only handle the above */
      }
    });

    mViewStack.processEvents(mEventQueue, currentTick);
    mEventQueue.clearEvents();

    mViewStack.distributeInput(currentTick, mInputTracker);

    /* Clears global linear allocator */
    lnClear();

    // Defined in client
    tick();

    Graphics::VulkanFrame frame = mGraphicsContext.beginFrame();
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

    TimeStamp frameEnd = getCurrentTime();
    mDt = getTimeDifference(frameEnd, frameStart);

    if (mDt < mMinFrametime) {
      sleepSeconds(mMinFrametime - mDt);
      mDt = mMinFrametime;
    }

    accumulatedTime += mDt;
  }

  /* Shutdown */
  mRenderer3D.shutdown();
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

  case EventType::CursorDisplayChange: {
    auto *event = (EventCursorDisplayChange *)ev;
    mWindow.changeCursorDisplay(event->show);

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

void Application::processDebugEvent(Event *ev) {
  switch (ev->type) {
  case EventType::Breakpoint: {
    auto *event = (EventBreakpoint *)ev;
    
#if _WIN32
    __debugbreak();
#else
    asm("int $3");
#endif

    event->isHandled = true;
  } break;

  default:;
  }
}

void Application::processFileEvent(Event *ev) {
  switch (ev->type) {
  case EventType::PathChanged: {
    auto *event = (EventPathChanged *)ev;

    /* Get renderer to update changed resources */
    mRenderer3D.trackPath(event->id, event->path);

    event->isHandled = true;
  } break;

  default:;
  }
}

void Application::setMaxFramerate(float fps) {
  mMaxFramerate = fps;
  mMinFrametime = 1.0f / mMaxFramerate;
}

}
