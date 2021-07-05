#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "RenderMethod.hpp"

namespace Ondine::Graphics {

struct SceneObject {
  union {
    uint32_t flagBits;
    struct {
      uint32_t isInitialised : 1;
    };
  };

  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;
  glm::mat4 transform;
  RenderMethodHandle renderMethod;

  inline void constructTransform() {
    transform = glm::translate(position) *
      glm::mat4_cast(rotation) *
      glm::scale(scale);
  }
};

}
