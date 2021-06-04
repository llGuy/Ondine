#pragma once

#include "Tick.hpp"
#include <stdint.h>
#include "Event.hpp"
#include <glm/glm.hpp>

namespace Ondine {

enum class MouseButton {
  Left,
  Right,
  Middle,
  Unhandled
};

enum class KeyboardButton {
  A, B, C, D, E, F, G, H, I, J,
  K, L, M, N, O, P, Q, R, S, T,
  U, V, W, X, Y, Z,

  Zero, One, Two, Three, Four, Five, Six,
  Seven, Eight, Nine, Up, Left, Down, Right,
  Space, LeftShift, LeftControl, Enter, Backspace,
  Escape, F1, F2, F3, F4, F5, F9, F11,
  Unhandled
};

struct Resolution {
  uint32_t width;
  uint32_t height;
};

struct ButtonState {
  uint32_t isDown : 1;
  /* Was the key pressed at this instant (only true for one frame) */
  uint32_t didInstant : 1;
  uint32_t didRelease : 1;
  uint32_t pad : 5;
};

struct Cursor {
  uint8_t didCursorMove : 4;
  uint8_t didScroll : 4;
  glm::ivec2 cursorPos;
  glm::ivec2 previousPos;
  glm::vec2 scroll;
};

struct EventMouse;
struct EventKeyboard;

class InputTracker {
public:
  InputTracker();

  // Need to call this before window input polling
  void tick(const Tick &tick);

  // Gets called after input polling
  void handleKeyboardEvent(EventKeyboard *ev);
  void handleMouseEvent(EventMouse *ev);

  const ButtonState &key(KeyboardButton button) const;
  const ButtonState &mouseButton(MouseButton button) const;
  const Cursor &cursor() const;

private:
  ButtonState &key(KeyboardButton button);
  ButtonState &mouseButton(MouseButton button);
  Cursor &cursor();

private:
  ButtonState mKeyboardButtons[(int)KeyboardButton::Unhandled];
  // Keep track of this so that we can update the downAmount for each frame
  size_t mPressedKeyCount;
  int mPressedKeys[(int)KeyboardButton::Unhandled];
  // Keep track of this so that in the next frame, we can clear the released
  size_t mReleasedKeyCount;
  int mReleasedKeys[(int)KeyboardButton::Unhandled];

  ButtonState mMouseButtons[(int)MouseButton::Unhandled];
  size_t mPressedMouseButtonCount;
  int mPressedMouseButtons[(int)MouseButton::Unhandled];
  size_t mReleasedMouseButtonCount;
  int mReleasedMouseButtons[(int)MouseButton::Unhandled];

  Cursor mCursor;
};

}
