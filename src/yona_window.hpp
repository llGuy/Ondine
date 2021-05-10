#pragma once

#include "yona_io.hpp"
#include <string_view>
#include <GLFW/glfw3.h>
#include "yona_event.hpp"

namespace Yona {

enum class WindowMode {
  Fullscreen,
  Windowed
};

class Window {
public:
  Window(
    WindowMode mode,
    const std::string_view &title,
    const Resolution &resolution = {});

  void init(OnEventProc callback);

  static void initWindowAPI();

private:
  void keyCallback(int key, int scancode, int action, int mods);
  void mouseButtonCallback(int button, int action, int mods);
  void charCallback(int button, int action, int mods);
  void cursorMoveCallback(int button, int action, int mods);

private:
  GLFWwindow *mHandle;
  Resolution mResolution;
  OnEventProc mEventCallback;
  WindowMode mWindowMode;
  const std::string_view mTitle;
};

}
