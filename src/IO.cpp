#include "IO.hpp"
#include "Log.hpp"
#include "IOEvent.hpp"

namespace Ondine {

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

void InputTracker::tick(const Tick &tick) {
  for (int i = 0; i < mPressedKeyCount; ++i) {
    int idx = mPressedKeys[i];
    mKeyboardButtons[idx].didInstant = false;
  }

  for (int i = 0; i < mReleasedKeyCount; ++i) {
    int idx = mPressedKeys[i];
    mKeyboardButtons[idx].didRelease = false;
  }

  mPressedKeyCount = 0;
  mReleasedKeyCount = 0;
  mCursor.didCursorMove = false;
  mCursor.didScroll = false;
}

void InputTracker::handleKeyboardEvent(EventKeyboard *ev) {
  switch (ev->keyboardEventType) {
  case KeyboardEventType::Press: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.isDown = true;
    pressedKey.didInstant = true;
    pressedKey.didRelease = false;

    mPressedKeys[mPressedKeyCount++] = (int)ev->press.button;
  } break;

  case KeyboardEventType::Release: {
    auto &pressedKey = key(ev->press.button);
    pressedKey.isDown = false;
    pressedKey.didInstant = false;
    pressedKey.didRelease = true;

    mReleasedKeys[mReleasedKeyCount++] = (int)ev->press.button;
  } break;

  case KeyboardEventType::Type: {
    // TODO: When typing
  } break;
  }
}

void InputTracker::handleMouseEvent(EventMouse *ev) {
  switch (ev->mouseEventType) {
  case MouseEventType::Press: {
    auto &pressedMB = mouseButton(ev->press.button);
    pressedMB.isDown = true;
    pressedMB.didInstant = true;
    pressedMB.didRelease = false;

    mPressedMouseButtons[mPressedMouseButtonCount++] = (int)ev->press.button;
  } break;

  case MouseEventType::Release: {
    auto &pressedKey = mouseButton(ev->press.button);
    pressedKey.isDown = false;
    pressedKey.didInstant = false;
    pressedKey.didRelease = true;

    mReleasedMouseButtons[mReleasedMouseButtonCount++] = (int)ev->press.button;
  } break;

  case MouseEventType::Move: {
    mCursor.didCursorMove = true;
    mCursor.previousPos = mCursor.cursorPos;
    mCursor.cursorPos = glm::ivec2((int)ev->move.x, (int)ev->move.y);
  } break;

  case MouseEventType::Scroll: {
    mCursor.didScroll = true;
    mCursor.scroll = glm::vec2(ev->scroll.x, ev->scroll.y);
  } break;
  }
}

const ButtonState &InputTracker::key(KeyboardButton button) const {
  return mKeyboardButtons[(int)button];
}

const ButtonState &InputTracker::mouseButton(MouseButton button) const {
  return mMouseButtons[(int)button];
}

const Cursor &InputTracker::cursor() const {
  return mCursor;
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
