#include "InputTranslator.hpp"

namespace Ondine::Game {

UserInput InputTranslator::translate(const Core::InputTracker &inputTracker) {
  UserInput result = {};

  result.actions[(int)ActionType::MoveForward].bIsTriggered = inputTracker.key(Core::KeyboardButton::W).isDown;
  result.actions[(int)ActionType::MoveLeft].bIsTriggered = inputTracker.key(Core::KeyboardButton::A).isDown;
  result.actions[(int)ActionType::MoveBackward].bIsTriggered = inputTracker.key(Core::KeyboardButton::S).isDown;
  result.actions[(int)ActionType::MoveRight].bIsTriggered = inputTracker.key(Core::KeyboardButton::D).isDown;
  result.actions[(int)ActionType::MoveUp].bIsTriggered = inputTracker.key(Core::KeyboardButton::Space).isDown;
  result.actions[(int)ActionType::MoveDown].bIsTriggered = inputTracker.key(Core::KeyboardButton::LeftShift).isDown;
  result.actions[(int)ActionType::SpeedMultiply].bIsTriggered = inputTracker.key(Core::KeyboardButton::R).isDown;

  const auto &cursor = inputTracker.cursor();
  if (cursor.didCursorMove) {
    result.cursorDelta = glm::vec2(cursor.cursorPos) - glm::vec2(cursor.previousPos);
  }

  result.bDidCursorMove = cursor.didCursorMove;

  result.cursorPosition = cursor.cursorPos;

  if (cursor.didScroll) {
    result.wheelAmount = cursor.scroll.y;
  }

  result.bDidScroll = cursor.didScroll;

  return result;
}

}
