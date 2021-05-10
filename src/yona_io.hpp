#pragma once

#include <stdint.h>

namespace Yona {

enum class MouseButton {
  Left,
  Right,
  Middle
};

enum class KeyboardButton {
  A, B, C, D, E, F, G, H, I, J,
  K, L, M, N, O, P, Q, R, S, T,
  U, V, W, X, Y, Z,

  Zero, One, Two, Three, Four, Five, Six,
  Seven, Eight, Nine, Up, Left, Down, Right,
  Space, LeftShift, LeftControl, Enter, Backspace,
  Escape, F1, F2, F3, F4, F5, F9, F11,
};

struct Resolution {
  uint32_t width;
  uint32_t height;
};

}
