#include "IO.hpp"
#include "IOEvent.hpp"

namespace Yona {

InputTracker::InputTracker()
  : mKeyboardButtons{},
    mMouseButtons{},
    mCursor{},
    mPressedKeyCount(0) ,
    mPressedKeys{},
    mReleasedKeyCount(0),
    mReleasedKeys{},
    mPressedMouseButtonCount(0),
    mPressedMouseButtons{},
    mReleasedMouseButtonCount(0),
    mReleasedMouseButtons{} {
  
}

void InputTracker::handleKeyboardEvent(EventKeyboard *ev, const Tick &tick) {
  switch (ev->keyboardEventType) {
  case KeyboardEventType::Press: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.downAmount += tick.dt;
    pressedKey.isDown = 1;
    pressedKey.didInstant = 1;
    pressedKey.didRelease = 1;

    mPressedKeys[mPressedKeyCount++] = (int)ev->press.button;
  } break;

  case KeyboardEventType::Release: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.downAmount += tick.dt;
    pressedKey.isDown = 1;
    pressedKey.didInstant = 1;
    pressedKey.didRelease = 1;

    mReleasedKeys[mReleasedKeyCount++] = (int)ev->press.button;
  } break;

  case KeyboardEventType::Type: {
    // TODO: When typing
  } break;
  }
}

void InputTracker::handleMouseEvent(EventMouse *ev, const Tick &tick) {
  switch (ev->mouseEventType) {
  case MouseEventType::Press: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.downAmount += tick.dt;
    pressedKey.isDown = 1;
    pressedKey.didInstant = 1;
    pressedKey.didRelease = 1;

    mPressedKeys[mPressedKeyCount++] = (int)ev->press.button;
  } break;

  case MouseEventType::Release: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.downAmount += tick.dt;
    pressedKey.isDown = 1;
    pressedKey.didInstant = 1;
    pressedKey.didRelease = 1;

    mReleasedKeys[mReleasedKeyCount++] = (int)ev->press.button;
  } break;

  case MouseEventType::Move: {
    
  } break;

  case MouseEventType::Scroll: {
    
  } break;
  }
}

ButtonState &InputTracker::key(KeyboardButton button) {
  return mKeyboardButtons[(int)button];
}

ButtonState &InputTracker::mouseButton(MouseButton button) {
  return mMouseButtons[(int)button];
}

Cursor &InputTracker::cursor() {
  return mCursor;
}

}
