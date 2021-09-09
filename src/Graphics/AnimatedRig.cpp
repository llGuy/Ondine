#include <string.h>
#include "AnimatedRig.hpp"

namespace Ondine::Graphics {

void AnimatedRig::init(
  SkeletonHandle skeletonHandle,
  AnimationCycleGroupHandle cycleGroupHandle,
  const AnimationManager &animations,
  VulkanContext &graphicsContext) {
  const auto &skeleton = animations.getSkeleton(skeletonHandle);
  const auto &cycles = animations.getAnimationCycleGroup(cycleGroupHandle);

  mCurrentAnimationTime = 0.0f;
  mIsInterpolatingBetweenCycles = 0;
  mSkeleton = skeletonHandle;
  mNextBoundCycle = 0;

  mCycleGroup = cycleGroupHandle;
  mInBetweenInterpolationTime = 0.2f;

  mInterpolatedTransforms.init(skeleton.bones.size());
  mCurrentPositions.init(skeleton.bones.size());
  mCurrentRotations.init(skeleton.bones.size());
  mCurrentScales.init(skeleton.bones.size());
  mCurrentPositionIndices.init(skeleton.bones.size());
  mCurrentRotationIndices.init(skeleton.bones.size());
  mCurrentScaleIndices.init(skeleton.bones.size());

  mInterpolatedTransforms.size = skeleton.bones.size();
  mCurrentPositions.size = skeleton.bones.size();
  mCurrentRotations.size = skeleton.bones.size();
  mCurrentScales.size = skeleton.bones.size();
  mCurrentPositionIndices.size = skeleton.bones.size();
  mCurrentRotationIndices.size = skeleton.bones.size();
  mCurrentScaleIndices.size = skeleton.bones.size();

  mCurrentPositionIndices.zero();
  mCurrentRotationIndices.zero();
  mCurrentScaleIndices.zero();

  for (uint32_t i = 0; i < skeleton.bones.size(); ++i) {
    mInterpolatedTransforms[i] = glm::mat4(1.0f);
  }

  mInterpolatedTransformsBuffer.init(
    graphicsContext.device(),
    sizeof(glm::mat4) * skeleton.bones.size(),
    (int)VulkanBufferFlag::UniformBuffer);

  mInterpolatedTransformsBuffer.fillWithStaging(
    graphicsContext.device(),
    graphicsContext.commandPool(),
    {(uint8_t *)mInterpolatedTransforms.data, mInterpolatedTransforms.memSize()});

  mInterpolatedTransformsUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mInterpolatedTransformsBuffer));
}

}
