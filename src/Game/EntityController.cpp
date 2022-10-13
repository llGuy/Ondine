#include "Entity.hpp"
#include "EntityController.hpp"

namespace Ondine::Game {

void EntityController::init() {
  mControlledEntity = kInvalidEntityHandle;
}

void EntityController::tick(
  const Core::Tick &tick,
  Simulation &sim,
  const UserInput &userInput) {
  Entity &entity = sim.getEntity(mControlledEntity);

  auto up = kGlobalUp;
  auto right = glm::normalize(glm::cross(
    entity.viewDirection, kGlobalUp));
  auto forward = glm::normalize(glm::cross(up, right));

  float speedMultiplier = 30.0f;
  if (userInput.actions[(int)ActionType::SpeedMultiply].bIsTriggered) {
    speedMultiplier *= 10.0f;
  }

  if (userInput.actions[(int)ActionType::MoveForward].bIsTriggered) {
    entity.position += forward * tick.dt * speedMultiplier;
  }
  if (userInput.actions[(int)ActionType::MoveLeft].bIsTriggered) {
    entity.position -= right * tick.dt * speedMultiplier;
  }
  if (userInput.actions[(int)ActionType::MoveBackward].bIsTriggered) {
    entity.position -= forward * tick.dt * speedMultiplier;
  }
  if (userInput.actions[(int)ActionType::MoveRight].bIsTriggered) {
    entity.position += right * tick.dt * speedMultiplier;
  }
  if (userInput.actions[(int)ActionType::MoveUp].bIsTriggered) {
    entity.position += kGlobalUp * tick.dt * speedMultiplier;
  }
  if (userInput.actions[(int)ActionType::MoveDown].bIsTriggered) {
    entity.position -= kGlobalUp * tick.dt * speedMultiplier;
  }

  if (userInput.bDidCursorMove) {
    static constexpr float SENSITIVITY = 15.0f;

    auto delta = userInput.cursorDelta;
    auto res = entity.viewDirection;

    float xAngle = glm::radians(-delta.x) * SENSITIVITY * tick.dt;
    float yAngle = glm::radians(-delta.y) * SENSITIVITY * tick.dt;
                
    res = glm::mat3(glm::rotate(xAngle, kGlobalUp)) * res;
    auto rotateY = glm::cross(res, kGlobalUp);
    res = glm::mat3(glm::rotate(yAngle, rotateY)) * res;

    res = glm::normalize(res);

    entity.viewDirection = res;
  }
}

EntityHandle &EntityController::getControlledEntity() {
  return mControlledEntity;
}

}
