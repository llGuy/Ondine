#pragma once

#include "Model.hpp"
#include "AssimpImporter.hpp"

namespace Ondine::Graphics {

using ModelHandle = uint32_t;
constexpr ModelHandle MODEL_HANDLE_INVALID = 0xFFFFFFFF;

/* Stores all mesh information for all models */
class ModelManager {
public:
  void init();

  ModelHandle loadModel(
    const char *path, VulkanContext &context,
    ModelConfig &config);
  ModelHandle loadModel(
    const aiScene *scene, VulkanContext &context,
    ModelConfig &config);
  Model &getModel(ModelHandle modelHandle);
  const Model &getModel(ModelHandle modelHandle) const;

private:
  static constexpr uint32_t MAX_MODEL_COUNT = 100;

  Array<Model, AllocationType::Freelist> mModels;
};

}
