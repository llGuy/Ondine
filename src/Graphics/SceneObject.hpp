#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "RenderMethod.hpp"

namespace Ondine::Graphics {

using SceneObjectPushConstantProc = void (*)(
  const VulkanFrame &frame,
  struct SceneObject *obj);

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

  // Named for push constant
  struct {
    glm::mat4 transform;
    glm::vec3 color;
  } pushConstant;

  RenderMethodHandle renderMethod;

  inline void constructTransform() {
    pushConstant.transform = glm::translate(position) *
      glm::mat4_cast(rotation) *
      glm::scale(scale);
  }
};

}
