#pragma once

#include "Entity.hpp"
#include "Simulation.hpp"

namespace Ondine::Game {

enum class EntityCameraType {
  // This is no view model rendering whatsoever
  FirstPerson,
  // First person with viewmodel rendering
  FirstPersonViewModel,
  // Third person
  ThirdPerson,
  // Cinematic third person
  CinematicThirdPerson
};

class EntityCamera {
public:
  void init();
  void setCameraType(EntityCameraType);

  void tick(const Core::Tick &tick, Simulation &sim);

  EntityHandle &getAttachedEntity();

private:
  EntityCameraType mType;
  EntityHandle mAttachedEntity;

  glm::vec3 mPosition;
  glm::vec3 mViewDirection;
  glm::vec3 mUp;

  // Perhaps some other parameters like wobble etc...
};

}
