#pragma once

#include "yona_io.hpp"
#include "yona_event.hpp"

namespace Yona {

enum class MouseEventType {
  Press,
  Release,
  Move,
  Scroll
};

struct EventMouse : Event {
  EVENT_TO_STRING(EventMouse);

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
  EVENT_TO_STRING(EventKeyboard);

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
      char typedChar;
    } type;
  };
};

struct EventResize : Event {
  EVENT_TO_STRING(EventResize);

  Resolution newResolution;
};

}
