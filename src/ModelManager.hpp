#pragma once

#include "Model.hpp"
#include <assimp/Importer.hpp>

namespace Ondine::Graphics {

using StaticModelHandle = uint32_t;
constexpr StaticModelHandle MODEL_HANDLE_INVALID = 0xFFFFFFFF;

class ModelManager {
public:
  void init();

  StaticModelHandle loadStaticModel(const char *path, VulkanContext &context);
  Model &getStaticModel(StaticModelHandle modelHandle);

private:
  static constexpr uint32_t MAX_MODEL_COUNT = 100;

  Assimp::Importer mImporter;
  Array<Model, AllocationType::Freelist> mStaticModels;
};

}
