#pragma once

#include "Math.hpp"

namespace Ondine::Game {

enum class ActionType {
  MoveForward,
  MoveLeft,
  MoveBackward,
  MoveRight,
  MoveUp,
  MoveDown,
  SpeedMultiply,
  Invalid
};

struct Action {
  bool bIsTriggered;

  // May add more information later on like duration etc...
};

struct UserInput {
  Action actions[(int)ActionType::Invalid];

  bool bDidCursorMove;
  bool bDidScroll;

  // In pixel coordinates
  glm::vec2 cursorDelta;
  glm::vec2 cursorPosition;

  // Scrolling
  float wheelAmount;
};

}
