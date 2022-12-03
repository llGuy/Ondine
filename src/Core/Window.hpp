#pragma once

#include "IO.hpp"
#include <string_view>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Event.hpp"
#include "Time.hpp"
#include "Utils.hpp"

namespace Ondine::Core {

enum class WindowMode {
  Fullscreen,
  Maximized,
  Windowed
};

using SurfaceCreationProc = void(*)(
  struct VkInstance_T *instance,
  struct VkSurfaceKHR_T **surface,
  void *windowHandle);

/* To pass to graphics context */
struct WindowContextInfo {
  void *handle;
  Resolution resolution;
  SurfaceCreationProc surfaceCreateProc;
};

class Window {
public:
  Window(
    WindowMode mode,
    const std::string_view &title,
    const Resolution &resolution = {});

  WindowContextInfo init(OnEventProc callback);
  void pollInput();
  void toggleFullscreen();

  void changeCursorDisplay(bool show);

  static void initWindowAPI();

private:
  void keyCallback(int key, int scancode, int action, int mods) const;
  void mouseButtonCallback(int button, int action, int mods) const;
  void charCallback(unsigned int codePoint) const;
  void cursorMoveCallback(float x, float y) const;
  void resizeCallback(unsigned width, unsigned height);
  void scrollCallback(float x, float y) const;
  void closeCallback() const;

  static void createVulkanSurface(
    struct VkInstance_T *instance,
    struct VkSurfaceKHR_T **surface,
    void *windowHandle);

private:
  GLFWwindow *mHandle;
  Resolution mResolution;
  Resolution mPreviousWindowedResolution;
  OnEventProc mEventCallback;
  WindowMode mWindowMode;
  bool mIsFullscreen;
  bool mResized;
  glm::ivec2 mPreviousWindowedPosition;
  const std::string_view mTitle;
};

}
