#define GLFW_INCLUDE_VULKAN
#include <stdlib.h>
#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_window.hpp"
#include "yona_memory.hpp"
#include "yona_io_event.hpp"

namespace Yona {

Window::Window(
  WindowMode mode,
  const std::string_view &title,
  const Resolution &resolution)
  : mResolution(resolution),
    mTitle(title),
    mWindowMode(mode) {
  
}

WindowContextInfo Window::init(OnEventProc callback) {
  mEventCallback = callback;
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
  GLFWmonitor *monitor = nullptr;

  switch (mWindowMode) {

  case WindowMode::Fullscreen: {
    monitor = primaryMonitor;
    const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
    mResolution.width = vidmode->width;
    mResolution.height = vidmode->height;
  } break;

  case WindowMode::Windowed: {
    const GLFWvidmode *vidmode = glfwGetVideoMode(primaryMonitor);
    if (!mResolution.width) {
      mResolution.width = vidmode->width / 1.5f;
      mResolution.height = vidmode->height / 1.5f;
    }
  } break;

  }

  if (!(mHandle = glfwCreateWindow(
    mResolution.width, mResolution.height,
    mTitle.data(),
    monitor, nullptr))) {
    LOG_ERROR("Failed to initialise window\n");
    PANIC_AND_EXIT();
  }

  glfwSetWindowUserPointer(mHandle, this);

  glfwSetKeyCallback(
    mHandle,
    [](GLFWwindow *win, int key, int sc, int action, int mods) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->keyCallback(key, sc, action, mods);
    });

  glfwSetMouseButtonCallback(
    mHandle,
    [](GLFWwindow *win, int button, int action, int mods) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->mouseButtonCallback(button, action, mods);
    });

  glfwSetCharCallback(
    mHandle,
    [](GLFWwindow *win, unsigned int codepoint) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->charCallback(codepoint);
    });

  glfwSetCursorPosCallback(
    mHandle,
    [](GLFWwindow *win, double x, double y) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->cursorMoveCallback((float)x, (float)y);
    });

  glfwSetWindowSizeCallback(
    mHandle,
    [](GLFWwindow *win, int width, int height) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->resizeCallback((uint32_t)width, (uint32_t)height);
    });

  glfwSetScrollCallback(
    mHandle,
    [](GLFWwindow *win, double x, double y) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->scrollCallback((float)x, (float)y);
    });

  glfwSetWindowCloseCallback(
    mHandle,
    [](GLFWwindow *win) {
      Window *userPtr = (Window *)glfwGetWindowUserPointer(win);
      userPtr->closeCallback();
    });

  WindowContextInfo info = {};
  info.handle = mHandle;
  info.resolution = mResolution;
  info.surfaceCreateProc = createVulkanSurface;

  return info;
}

void Window::pollInput() {
  glfwPollEvents();
}

void Window::initWindowAPI() {
  if (!glfwInit()) {
    LOG_ERROR("Failed to initialise GLFW\n");
    PANIC_AND_EXIT();
  }
}

void Window::keyCallback(int key, int scancode, int action, int mods) const {
  KeyboardButton button;

  switch(key) {
  case GLFW_KEY_A: { button = KeyboardButton::A; } break;
  case GLFW_KEY_B: { button = KeyboardButton::B; } break;
  case GLFW_KEY_C: { button = KeyboardButton::C; } break;
  case GLFW_KEY_D: { button = KeyboardButton::D; } break;
  case GLFW_KEY_E: { button = KeyboardButton::E; } break;
  case GLFW_KEY_F: { button = KeyboardButton::F; } break;
  case GLFW_KEY_G: { button = KeyboardButton::G; } break;
  case GLFW_KEY_H: { button = KeyboardButton::H; } break;
  case GLFW_KEY_I: { button = KeyboardButton::I; } break;
  case GLFW_KEY_J: { button = KeyboardButton::J; } break;
  case GLFW_KEY_K: { button = KeyboardButton::K; } break;
  case GLFW_KEY_L: { button = KeyboardButton::L; } break;
  case GLFW_KEY_M: { button = KeyboardButton::M; } break;
  case GLFW_KEY_N: { button = KeyboardButton::N; } break;
  case GLFW_KEY_O: { button = KeyboardButton::O; } break;
  case GLFW_KEY_P: { button = KeyboardButton::P; } break;
  case GLFW_KEY_Q: { button = KeyboardButton::Q; } break;
  case GLFW_KEY_R: { button = KeyboardButton::R; } break;
  case GLFW_KEY_S: { button = KeyboardButton::S; } break;
  case GLFW_KEY_T: { button = KeyboardButton::T; } break;
  case GLFW_KEY_U: { button = KeyboardButton::U; } break;
  case GLFW_KEY_V: { button = KeyboardButton::V; } break;
  case GLFW_KEY_W: { button = KeyboardButton::W; } break;
  case GLFW_KEY_X: { button = KeyboardButton::X; } break;
  case GLFW_KEY_Y: { button = KeyboardButton::Y; } break;
  case GLFW_KEY_Z: { button = KeyboardButton::Z; } break;
  case GLFW_KEY_0: { button = KeyboardButton::Zero; } break;
  case GLFW_KEY_1: { button = KeyboardButton::One; } break;
  case GLFW_KEY_2: { button = KeyboardButton::Two; } break;
  case GLFW_KEY_3: { button = KeyboardButton::Three; } break;
  case GLFW_KEY_4: { button = KeyboardButton::Four; } break;
  case GLFW_KEY_5: { button = KeyboardButton::Five; } break;
  case GLFW_KEY_6: { button = KeyboardButton::Six; } break;
  case GLFW_KEY_7: { button = KeyboardButton::Seven; } break;
  case GLFW_KEY_8: { button = KeyboardButton::Eight; } break;
  case GLFW_KEY_9: { button = KeyboardButton::Nine; } break;
  case GLFW_KEY_UP: { button = KeyboardButton::Up; } break;
  case GLFW_KEY_LEFT: { button = KeyboardButton::Left; } break;
  case GLFW_KEY_DOWN: { button = KeyboardButton::Down; } break;
  case GLFW_KEY_RIGHT: { button = KeyboardButton::Right; } break;
  case GLFW_KEY_SPACE: { button = KeyboardButton::Space; } break;
  case GLFW_KEY_LEFT_SHIFT: { button = KeyboardButton::LeftShift; } break;
  case GLFW_KEY_LEFT_CONTROL: { button = KeyboardButton::LeftControl; } break;
  case GLFW_KEY_ENTER: { button = KeyboardButton::Enter; } break;
  case GLFW_KEY_BACKSPACE: { button = KeyboardButton::Backspace; } break;
  case GLFW_KEY_ESCAPE: { button = KeyboardButton::Escape; } break;
  case GLFW_KEY_F11: { button = KeyboardButton::F11; } break;
  case GLFW_KEY_F9: { button = KeyboardButton::F9; } break;
  }

  auto *kbEvent = lnEmplaceAlloc<EventKeyboard>();

  switch (action) {
  case GLFW_PRESS: case GLFW_REPEAT: {
    kbEvent->keyboardEventType = KeyboardEventType::Press;
    kbEvent->press.button = button;
    kbEvent->press.isRepeat = (action == GLFW_REPEAT);
  } break;

  case GLFW_RELEASE: {
    kbEvent->keyboardEventType = KeyboardEventType::Release;
    kbEvent->release.button = button;
  } break;
  }

  mEventCallback(kbEvent);
}

void Window::mouseButtonCallback(int button, int action, int mods) const {
  auto *mbEvent = lnEmplaceAlloc<EventMouse>();

  MouseButton mouseButton;

  switch (button) {
  case GLFW_MOUSE_BUTTON_LEFT: { mouseButton = MouseButton::Left; } break;
  case GLFW_MOUSE_BUTTON_RIGHT: { mouseButton = MouseButton::Right; } break;
  case GLFW_MOUSE_BUTTON_MIDDLE: { mouseButton = MouseButton::Middle; } break;
  }

  switch (action) {
  case GLFW_PRESS: case GLFW_REPEAT: {
    mbEvent->mouseEventType = MouseEventType::Press;
    mbEvent->press.button = mouseButton;
  } break;

  case GLFW_RELEASE: {
    mbEvent->mouseEventType = MouseEventType::Release;
    mbEvent->release.button = mouseButton;
  } break;
  }

  mEventCallback(mbEvent);
}

void Window::charCallback(unsigned int codepoint) const {
  auto *kbEvent = lnEmplaceAlloc<EventKeyboard>();
  kbEvent->keyboardEventType = KeyboardEventType::Type;
  kbEvent->type.typedChar = codepoint;

  mEventCallback(kbEvent);
}

void Window::cursorMoveCallback(float x, float y) const {
  auto *mvEvent = lnEmplaceAlloc<EventMouse>();
  mvEvent->mouseEventType = MouseEventType::Move;
  mvEvent->move.x = x;
  mvEvent->move.y = y;

  mEventCallback(mvEvent);
}

void Window::resizeCallback(unsigned width, unsigned height) {
  auto *resizeEvent = lnEmplaceAlloc<EventResize>();
  resizeEvent->newResolution = {width, height};

  mEventCallback(resizeEvent);

  mResolution = resizeEvent->newResolution;
}

void Window::scrollCallback(float x, float y) const {
  auto *mvEvent = lnEmplaceAlloc<EventMouse>();
  mvEvent->mouseEventType = MouseEventType::Scroll;
  mvEvent->scroll.x = x;
  mvEvent->scroll.y = y;

  mEventCallback(mvEvent);
}

void Window::closeCallback() const {
  auto *closeEvent = lnEmplaceAlloc<EventClose>();
  mEventCallback(closeEvent);
}

void Window::createVulkanSurface(
  struct VkInstance_T *instance,
  struct VkSurfaceKHR_T **surface,
  void *windowHandle) {
  if (glfwCreateWindowSurface(
        instance,
        (GLFWwindow *)windowHandle,
        nullptr,
        surface) != VK_SUCCESS) {
    LOG_ERROR("Failed to create surface\n");
    PANIC_AND_EXIT();
  }
}

}
