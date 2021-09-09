#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include "VulkanBuffer.hpp"
#include "VulkanUniform.hpp"
#include "VulkanContext.hpp"
#include "AnimationManager.hpp"

namespace Ondine::Graphics {

using SkeletonHandle = uint32_t;
using AnimationCycleGroupHandle = uint32_t;

// Each animated entity / scene object will need this
class AnimatedRig {
public:
  AnimatedRig() = default;
  ~AnimatedRig() = default;

  void init(
    SkeletonHandle skeleton,
    AnimationCycleGroupHandle cycleGroup,
    const AnimationManager &animations,
    VulkanContext &graphicsContext);

private:
  float mCurrentAnimationTime;
  float mInBetweenInterpolationTime;
  bool mIsInterpolatingBetweenCycles;

  SkeletonHandle mSkeleton;
  uint32_t mPrevBoundCycle;
  uint32_t mNextBoundCycle;
  AnimationCycleGroupHandle mCycleGroup;

  Array<glm::mat4> mInterpolatedTransforms;

  VulkanBuffer mInterpolatedTransformsBuffer;
  VulkanUniform mInterpolatedTransformsUniform;

  // NOTE: Should we put this in an array of structs instead of this mess?
  Array<uint32_t> mCurrentPositionIndices;
  Array<glm::vec3> mCurrentPositions;
  Array<uint32_t> mCurrentRotationIndices;
  Array<glm::quat> mCurrentRotations;
  Array<uint32_t> mCurrentScaleIndices;
  Array<glm::vec3> mCurrentScales;
};

}
