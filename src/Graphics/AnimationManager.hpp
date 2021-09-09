#pragma once

#include <stdint.h>
#include "Skeleton.hpp"
#include "AnimationCycle.hpp"

#include <vector>
#include <unordered_map>
#include "AssimpImporter.hpp"

namespace Ondine::Graphics {

class ModelConfig;
class VulkanContext;

using SkeletonHandle = uint32_t;
using AnimationCycleGroupHandle = uint32_t;

/* 
   Perhaps we could use a VulkanArenaAllocator to allocate matrix transforms??
   (please future-self, do this)
*/
class AnimationManager {
public:
  void init();

  void loadSkeleton(
    const aiScene *scene, ModelConfig &config, VulkanContext &context);

  Skeleton &getSkeleton(SkeletonHandle handle);
  AnimationCycleGroup &getAnimationCycleGroup(AnimationCycleGroupHandle handle);

  const Skeleton &getSkeleton(SkeletonHandle handle) const;
  const AnimationCycleGroup &getAnimationCycleGroup(
    AnimationCycleGroupHandle handle) const;

private:
  void loadBones(
    const aiScene *scene,
    Skeleton &skeleton,
    std::unordered_map<std::string, uint32_t> &boneNameMap,
    std::vector<glm::vec4> &weights,
    std::vector<glm::ivec4> &boneIDs);

private:
  static constexpr uint32_t MAX_ANIMATION_COUNT = 100;
  static constexpr uint32_t MAX_SKELETON_COUNT = 100;

  Array<Skeleton, AllocationType::Freelist> mSkeletons;
  Array<AnimationCycleGroup, AllocationType::Freelist> mAnimationCycleGroups;
};

}
