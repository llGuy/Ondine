#pragma once

#include "IO.hpp"
#include "Time.hpp"
#include "Game.hpp"
#include "Event.hpp"
#include "Window.hpp"
#include "Renderer3D.hpp"
#include "ViewStack.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Runtime {

class Application {
public:
  Application(int argc, char **argv);
  virtual ~Application();

  /* Core */
  void run();

  static void recvEvent(Core::Event *ev, void *app);

private:
  /* Need to be defined in a Client / Editor application */
  virtual void start() = 0;
  virtual void tick() = 0;

  void pushEvent(Core::Event *ev);
  void processInputEvent(Core::Event *ev);
  void processGraphicsEvent(Core::Event *ev);
  void processDebugEvent(Core::Event *ev);
  void processFileEvent(Core::Event *ev);
  void setMaxFramerate(float fps);

private:
  bool mIsRunning;
  Core::EventQueue mEventQueue;
  Core::Window mWindow;
  Graphics::VulkanContext mGraphicsContext;
  Graphics::Renderer3D mRenderer3D;
  View::ViewStack mViewStack;
  Core::InputTracker mInputTracker;
  Game::Game mGame;

  float mDt;
};

}
