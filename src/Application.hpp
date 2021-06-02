#pragma once

#include "IO.hpp"
#include "Time.hpp"
#include "Event.hpp"
#include "Window.hpp"
#include "Renderer3D.hpp"
#include "ViewStack.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

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
  void processInputEvent(Event *ev);
  void processGraphicsEvent(Event *ev);
  void setMaxFramerate(float fps);

private:
  bool mIsRunning;
  EventQueue mEventQueue;
  Window mWindow;
  VulkanContext mGraphicsContext;
  Renderer3D mRenderer3D;
  ViewStack mViewStack;
  InputTracker mInputTracker;

  float mDt;
  float mMaxFramerate;
  float mMinFrametime;
};

}
