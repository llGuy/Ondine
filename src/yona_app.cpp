#include "yona_log.hpp"
#include "yona_app.hpp"
#include "yona_memory.hpp"
#include "yona_io_event.hpp"
#include "yona_game_view.hpp"
#include "yona_filesystem.hpp"
#include "yona_editor_view.hpp"

namespace Yona {

Application::Application(int argc, char **argv)
  : mWindow(WindowMode::Windowed, "Yona"),
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

      case EventCategory::Renderer3D: {
        processRendererEvent(ev);
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
    if (kbEvent->keyboardEventType == KeyboardEventType::Press) {
      if (kbEvent->press.button == KeyboardButton::F11 &&
          !kbEvent->press.isRepeat) {
        mWindow.toggleFullscreen();
        mGraphicsContext.skipFrame();

        kbEvent->isHandled = true;
      }
    }
  } break;

  default:;
  }
}

void Application::processRendererEvent(Event *ev) {
  switch (ev->type) {
  case EventType::ViewportResize: {
    mGraphicsContext.device().idle();

    // Don't set it to handled - view stack will want to have a look at this
  } break;
  default:;
  }
}

void Application::setMaxFramerate(float fps) {
  mMaxFramerate = fps;
  mMinFrametime = 1.0f / mMaxFramerate;
}

}
