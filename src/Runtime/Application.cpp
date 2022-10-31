#include <signal.h>
#include "Log.hpp"
#include "Memory.hpp"
#include "IOEvent.hpp"
#include "MapView.hpp"
#include "GameView.hpp"
#include "Settings.hpp"
#include "FileEvent.hpp"
#include "FileSystem.hpp"
#include "EditorView.hpp"
#include "DebugEvent.hpp"
#include "ThreadPool.hpp"
#include "Application.hpp"

namespace Ondine::Runtime {

Application::Application(int argc, char **argv)
  : mWindow(Core::WindowMode::Windowed, "Ondine"),
    mRenderer3D(mGraphicsContext),
    mViewStack(mRenderer3D, mGraphicsContext) {
  /* Initialise graphics context, etc... */
  gSettings.setMaxFramerate(60.0f);
}

Application::~Application() {
  /* Destroy graphics context, etc... */
}

void Application::run() {
  /* Change YONA_PROJECT_ROOT depending on build type */
  Core::gFileSystem->addMountPoint(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    YONA_PROJECT_ROOT);

  Core::gFileSystem->addMountPoint(
    (Core::MountPoint)Core::ApplicationMountPoints::Raw,
    "");

  Core::Window::initWindowAPI();
  mGraphicsContext.initInstance();

  auto evProc = RECV_EVENT_PROC(recvEvent);
  auto surfaceInfo = mWindow.init(evProc);

  srand(time(nullptr));
  mGraphicsContext.initContext(surfaceInfo);
  mRenderer3D.init();
  mViewStack.init();

  mGame.init(mRenderer3D);
  mGame.initGameRendering(mRenderer3D);

  mViewStack.createView("GameView", new View::GameView(mGame, mRenderer3D, evProc));
  mViewStack.createView("MapView", new View::MapView(mRenderer3D, evProc));
  mViewStack.createView(
    "EditorView", new View::EditorView(
      surfaceInfo, mGraphicsContext, mRenderer3D, evProc));

  mViewStack.push("GameView");
  mViewStack.push("EditorView");

  /* User-defined function which will be overriden */
  start();
  mIsRunning = true;
  mDt = 0.0f;

  float accumulatedTime = 0.0f;

  while (mIsRunning) {
    Core::TimeStamp frameStart = Core::getCurrentTime();
    Core::Tick currentTick = { mDt, accumulatedTime };

    mInputTracker.tick(currentTick);

    mWindow.pollInput();
    Core::gFileSystem->trackFiles(evProc);
    Core::gThreadPool->tick();

    /* 
       Go through the core events (window resize, etc...), then offload 
       to the different views.
    */
    mEventQueue.process([this](Core::Event *ev) {
      switch (ev->category) {
      case Core::EventCategory::Input: {
        processInputEvent(ev);
      } break;

      case Core::EventCategory::Graphics: {
        processGraphicsEvent(ev);
      } break;

      case Core::EventCategory::Debug: {
        processDebugEvent(ev);
      } break;

      case Core::EventCategory::File: {
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

    mGame.tick(currentTick);

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

    Core::TimeStamp frameEnd = Core::getCurrentTime();
    mDt = Core::getTimeDifference(frameEnd, frameStart);

    if (mDt < gSettings.minFrametime) {
      Core::sleepSeconds(gSettings.minFrametime - mDt);
      mDt = gSettings.minFrametime;
    }

    accumulatedTime += mDt;
  }

  /* Shutdown */
  mRenderer3D.shutdown();
}

void Application::recvEvent(Core::Event *ev, void *obj) {
  Application *app = (Application *)obj;
  app->pushEvent(ev);
}

void Application::pushEvent(Core::Event *ev) {
  mEventQueue.push(ev);
}

void Application::processInputEvent(Core::Event *ev) {
  switch (ev->type) {
  case Core::EventType::Close: {
    mIsRunning = false;
    ev->isHandled = true;
  } break;

  case Core::EventType::Resize: {
    auto *resizeEvent = (Core::EventResize *)ev;
    mGraphicsContext.resize(resizeEvent->newResolution);

    // Don't set it to handled - view stack will want to have a look at this
  } break;

    /* Just for fullscreen toggling */
  case Core::EventType::Keyboard: {
    auto *kbEvent = (Core::EventKeyboard *)ev;
    mInputTracker.handleKeyboardEvent(kbEvent);

    if (kbEvent->keyboardEventType == Core::KeyboardEventType::Press) {
      if (kbEvent->press.button == Core::KeyboardButton::F11 &&
          !kbEvent->press.isRepeat) {
        mWindow.toggleFullscreen();
        mGraphicsContext.skipFrame();
      }
    }

    kbEvent->isHandled = true;
  } break;

  case Core::EventType::Mouse: {
    auto *event = (Core::EventMouse *)ev;
    mInputTracker.handleMouseEvent(event);

    event->isHandled = true;
  } break;

  case Core::EventType::CursorDisplayChange: {
    auto *event = (Core::EventCursorDisplayChange *)ev;
    mWindow.changeCursorDisplay(event->show);

    event->isHandled = true;
  } break;

  default:;
  }
}

void Application::processGraphicsEvent(Core::Event *ev) {
  switch (ev->type) {
  default:;
  }
}

void Application::processDebugEvent(Core::Event *ev) {
  switch (ev->type) {
  case Core::EventType::Breakpoint: {
    auto *event = (Core::EventBreakpoint *)ev;
    
#if _WIN32
    __debugbreak();
#elif defined(__APPLE__)
    raise(SIGTRAP);
#else
    asm("int $3");
#endif

    event->isHandled = true;
  } break;

  default:;
  }
}

void Application::processFileEvent(Core::Event *ev) {
  switch (ev->type) {
  case Core::EventType::PathChanged: {
    auto *event = (Core::EventPathChanged *)ev;

    /* Get renderer to update changed resources */
    mRenderer3D.trackPath(event->id, event->path);

    event->isHandled = true;
  } break;

  default:;
  }
}

}
