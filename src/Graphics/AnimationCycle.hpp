#pragma once

#include <vector>
#include <stdint.h>
#include "Buffer.hpp"
#include <glm/gtx/quaternion.hpp>

namespace Ondine::Graphics {

/* A bunch of structs needed for skeletal animation */
struct BonePositionKeyFrame {
  glm::vec3 position;
  float timeStamp;
};

struct BoneRotationKeyFrame {
  glm::quat rotation;
  float timeStamp;
};

struct BoneScaleKeyFrame {
  glm::vec3 scale;
  float timeStamp;
};

// Index of the joint_key_frames_t in the array = joint id
// Each bone has its own set of keyframes
struct BoneAnimation {
  std::vector<BonePositionKeyFrame> positions;
  std::vector<BoneRotationKeyFrame> rotations;
  std::vector<BoneScaleKeyFrame> scales;
};

struct AnimationCycle {
  const char *name;
  float duration;
  uint32_t keyFrameCount;
  Array<BoneAnimation> boneAnimations;
};

struct AnimationCycleGroup {
  uint32_t cycleCount;
  Array<AnimationCycle> cycles;
};

}
