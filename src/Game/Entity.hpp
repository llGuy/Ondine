#pragma once

#include <tuple>
#include "Math.hpp"
#include "Scene.hpp"

namespace Ondine::Game {

using EntityHandle = uint32_t;
constexpr EntityHandle kInvalidEntityHandle = ~(0U);

struct Entity {
  struct {
    uint32_t id : 31;
    uint32_t bSubmitForRender : 1;
  };

  // For now just dump everything that an entity would need
  glm::vec3 position;
  glm::vec3 viewDirection;
  glm::vec3 scale;
  glm::vec3 velocity;
  glm::quat rotation;

  // Rendering
  Graphics::SceneObjectHandle draw;
};

template <typename T> struct BehaviorID { static int32_t id; };
template <typename T> int32_t BehaviorID<T>::id = -1;

struct PhysicsBehavior;

}
