#pragma once

#include "Model.hpp"
#include "AssimpImporter.hpp"

#include "Memory.hpp"

namespace Ondine::Graphics {

using ModelHandle = uint32_t;
constexpr ModelHandle MODEL_HANDLE_INVALID = 0xFFFFFFFF;

struct ModelInfo {
  uint32_t offset;
};

/* Stores all mesh information for all models */
class ModelManager {
public:
  void init(VulkanContext &context);

  ModelConfig loadModelConfig(const char *path, VulkanContext &context);
  ModelConfig loadModelConfig(const aiScene *scene, VulkanContext &context);

  ModelHandle createModel(ModelConfig &config, VulkanContext &context);

  Model &getModel(ModelHandle modelHandle);
  const Model &getModel(ModelHandle modelHandle) const;

private:
  static constexpr uint32_t MAX_MODEL_COUNT = 100;
  static constexpr uint32_t VERTEX_POOL_SIZE = megabytes(50);

  Array<Model, AllocationType::Freelist> mModels;

  // Store all vertex and index data for all models here
  uint32_t mCurrentOffset;
  VulkanBuffer mVertexPool;
  VulkanBuffer mIndexPool;
};

}
