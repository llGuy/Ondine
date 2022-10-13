#pragma once

#include "Entity.hpp"
#include "GameInput.hpp"
#include "Simulation.hpp"

namespace Ondine::Game {

// Controls a designated entity with user input
class EntityController {
public:
  void init();

  void tick(
    const Core::Tick &tick,
    Simulation &sim,
    const UserInput &userInput);

  EntityHandle &getControlledEntity();

private:
  EntityHandle mControlledEntity;
};

}
