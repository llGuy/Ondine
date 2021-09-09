#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <glm/glm.hpp>

namespace Ondine::Graphics {

constexpr uint32_t MAX_CHILD_BONES = 5;

struct Bone {
  uint32_t boneId = 0;
  // Eventually won't need bone name
  std::string boneName;
  uint32_t childrenCount = 0;
  uint32_t childrenIDs[MAX_CHILD_BONES] = {};
  /* 
     Inverse of transform going from model space origin to "bind" position of bone
     "bind" = position of bone by default (without any animations affecting it)
  */
  glm::mat4 inverseBindTransform;
};

struct Skeleton {
  std::vector<Bone> bones;
};

}
