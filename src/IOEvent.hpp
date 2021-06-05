#pragma once

#include "IO.hpp"
#include "Utils.hpp"
#include "Event.hpp"

namespace Ondine::Core {

enum class MouseEventType {
  Press,
  Release,
  Move,
  Scroll
};

struct EventMouse : Event {
  EVENT_DEF(EventMouse, Input, Mouse);

  MouseEventType mouseEventType;

  union {
    struct {
      MouseButton button;
    } press;

    struct {
      MouseButton button;
    } release;

    struct {
      float x, y;
    } move;

    struct {
      float x, y;
    } scroll;
  };
};

enum class KeyboardEventType {
  Press,
  Release,
  Type
};

struct EventKeyboard : Event {
  EVENT_DEF(EventKeyboard, Input, Keyboard);

  KeyboardEventType keyboardEventType;

  union {
    struct {
      KeyboardButton button;
      bool isRepeat;
    } press;

    struct {
      KeyboardButton button;
    } release;

    struct {
      unsigned int typedChar;
    } type;
  };
};

struct EventResize : Event {
  EVENT_DEF(EventResize, Input, Resize);

  Resolution newResolution;
};

struct EventClose : Event {
  EVENT_DEF(EventClose, Input, Close);
};

struct EventCursorDisplayChange : Event {
  EVENT_DEF(EventCursorDisplayChange, Input, CursorDisplayChange);

  bool show;
};

}
