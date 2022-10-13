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

  AABB aabb;

  // Rendering
  Graphics::SceneObjectHandle draw;
};

inline glm::mat4 constructTransform(const Entity &entity) {
  return glm::translate(entity.position) *
    glm::mat4_cast(entity.rotation) *
    glm::scale(entity.scale);
}

template <typename T> struct BehaviorID { static int32_t id; };
template <typename T> int32_t BehaviorID<T>::id = -1;

struct PhysicsBehavior;

}
