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

using SurfaceCreationProc = void(*)(
  struct VkInstance_T *instance,
  struct VkSurfaceKHR_T **surface,
  void *windowHandle);

/* To pass to graphics context */
struct WindowInfo {
  void *handle;
  Resolution resolution;
  const std::string_view title;
};

class Window {
public:
  Window(
    WindowMode mode,
    const std::string_view &title,
    const Resolution &resolution = {});

  void init(OnEventProc callback);
  void pollInput();

  static void initWindowAPI();

private:
  void keyCallback(int key, int scancode, int action, int mods);
  void mouseButtonCallback(int button, int action, int mods);
  void charCallback(unsigned int codePoint);
  void cursorMoveCallback(float x, float y);
  void resizeCallback(unsigned width, unsigned height);
  void scrollCallback(float x, float y);
  void closeCallback();

  static void createVulkanSurface(
    struct VkInstance_T *instance,
    struct VkSurfaceKHR_T **surface,
    void *windowHandle);

private:
  GLFWwindow *mHandle;
  Resolution mResolution;
  OnEventProc mEventCallback;
  WindowMode mWindowMode;
  const std::string_view mTitle;
};

}
