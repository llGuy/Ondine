#include "Model.hpp"
#include "VulkanContext.hpp"
#include "AnimationManager.hpp"

namespace Ondine::Graphics {

void AnimationManager::init() {
  mSkeletons.init(MAX_SKELETON_COUNT);
  mAnimationCycleGroups.init(MAX_ANIMATION_COUNT);
}

SkeletonHandle AnimationManager::loadSkeleton(
  const aiScene *scene, ModelConfig &config, VulkanContext &context) {
  SkeletonHandle handle = mSkeletons.size++;
  auto &skeleton = mSkeletons[handle];

  std::vector<glm::vec4> weights;
  std::vector<glm::ivec4> boneIDs;

  weights.resize(config.mVertexCount);
  boneIDs.resize(config.mVertexCount);

  std::unordered_map<std::string, uint32_t> boneNameMap;

  loadBones(scene, skeleton, boneNameMap, weights, boneIDs);
  loadHierarchy(boneNameMap, skeleton.bones, scene->mRootNode, scene);

  return handle;
}

Skeleton &AnimationManager::getSkeleton(SkeletonHandle handle) {
  return mSkeletons[handle];
}

AnimationCycleGroup &AnimationManager::getAnimationCycleGroup(
  AnimationCycleGroupHandle handle) {
  return mAnimationCycleGroups[handle];
}

const Skeleton &AnimationManager::getSkeleton(SkeletonHandle handle) const {
  return mSkeletons[handle];
}

const AnimationCycleGroup &AnimationManager::getAnimationCycleGroup(
  AnimationCycleGroupHandle handle) const {
  return mAnimationCycleGroups[handle];
}

void AnimationManager::loadBones(
  const aiScene *scene,
  Skeleton &skeleton,
  std::unordered_map<std::string, uint32_t> &boneNameMap,
  std::vector<glm::vec4> &weights,
  std::vector<glm::ivec4> &boneIDs) {
  aiMesh *mesh = scene->mMeshes[0];

  skeleton.bones.reserve(mesh->mNumBones);

  for (uint32_t i = 0; i < mesh->mNumBones; ++i) {
    uint32_t boneIndex = 0;
    std::string boneName = mesh->mBones[i]->mName.data;

    if (boneNameMap.find(boneName) == boneNameMap.end()) {
      boneIndex = skeleton.bones.size();
      Bone bone = {};
      bone.boneName = boneName;
      skeleton.bones.push_back(bone);
    }
    else {
      boneIndex = boneNameMap[boneName];
    }

    boneNameMap[boneName] = boneIndex;
    skeleton.bones[boneIndex].inverseBindTransform =
      aiToGlmMat4(mesh->mBones[i]->mOffsetMatrix);

    for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
      uint32_t vertex_id = mesh->mBones[i]->mWeights[j].mVertexId;
      float weight = mesh->mBones[i]->mWeights[j].mWeight;

      bool filled = 0;
      for (uint32_t w = 0; w < 4; ++w) {
        if (weights[vertex_id][w] == 0) {
          weights[vertex_id][w] = weight;
          boneIDs[vertex_id][w] = boneIndex;
          filled = 1;
          break;
        }
      }
    }
  }
}

void AnimationManager::loadHierarchy(
  const std::unordered_map<std::string, uint32_t> &boneNameMap,
  std::vector<Bone> &bones,
  const aiNode *node,
  const aiScene *scene) {
  if (boneNameMap.find(node->mName.data) != boneNameMap.end()) {
    uint32_t index = boneNameMap.at(node->mName.data);
    Bone &bone = bones[index];

    for (uint32_t i = 0; i < node->mNumChildren && i < MAX_CHILD_BONES; ++i) {
      std::string childName = node->mChildren[i]->mName.data;
      if (boneNameMap.find(childName) != boneNameMap.end()) {
        uint32_t childIndex = boneNameMap.at(childName);
        bone.childrenIDs[bone.childrenCount++] = childIndex;

        loadHierarchy(boneNameMap, bones, node->mChildren[i], scene);
      }
    }
  }
  else {
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
      loadHierarchy(boneNameMap, bones, node->mChildren[i], scene);
    }
  }
}

}
