#include <new>
#include "FileSystem.hpp"
#include "ModelManager.hpp"
#include "AssimpImporter.hpp"

namespace Ondine::Graphics {

void ModelManager::init() {
  /* Possibly load models loaded from previous session or something idk */
  mModels.init(MAX_MODEL_COUNT);
}

ModelHandle ModelManager::loadModel(
  const char *path, VulkanContext &graphicsContext,
  ModelConfig &modelConfig) {
  const aiScene *scene = importScene(path);

  /* We assume just one mesh for now */
  const aiMesh *mesh = scene->mMeshes[0];

  new(&modelConfig) ModelConfig(mesh->mNumVertices);

  modelConfig.pushAttribute(
    {sizeof(glm::vec3), VK_FORMAT_R32G32B32_SFLOAT},
    {(uint8_t *)mesh->mVertices, sizeof(glm::vec3) * mesh->mNumVertices});

  if (mesh->mNormals) {
    modelConfig.pushAttribute(
      {sizeof(glm::vec3), VK_FORMAT_R32G32B32_SFLOAT},
      {(uint8_t *)mesh->mNormals, sizeof(glm::vec3) * mesh->mNumVertices});
  }

  uint32_t indexCount = mesh->mNumFaces * 3;
  uint32_t *indices = lnAllocv<uint32_t>(indexCount);

  for (int i = 0; i < mesh->mNumFaces; ++i) {
    indices[i * 3] = mesh->mFaces[i].mIndices[0];
    indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
    indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
  }

  modelConfig.configureIndices(
    indexCount, VK_INDEX_TYPE_UINT32,
    {(uint8_t *)indices, sizeof(uint32_t) * indexCount});
  
  /* TODO: Rest */
  ModelHandle handle = mModels.size++;
  Model &model = mModels[handle];
  model.init(modelConfig, graphicsContext);

  return handle;
}

ModelHandle ModelManager::loadModel(
  const aiScene *scene, VulkanContext &context,
  ModelConfig &modelConfig) {
  /* We assume just one mesh for now */
  const aiMesh *mesh = scene->mMeshes[0];

  new(&modelConfig) ModelConfig(mesh->mNumVertices);

  modelConfig.pushAttribute(
    {sizeof(glm::vec3), VK_FORMAT_R32G32B32_SFLOAT},
    {(uint8_t *)mesh->mVertices, sizeof(glm::vec3) * mesh->mNumVertices});

  if (mesh->mNormals) {
    modelConfig.pushAttribute(
      {sizeof(glm::vec3), VK_FORMAT_R32G32B32_SFLOAT},
      {(uint8_t *)mesh->mNormals, sizeof(glm::vec3) * mesh->mNumVertices});
  }

  uint32_t indexCount = mesh->mNumFaces * 3;
  uint32_t *indices = lnAllocv<uint32_t>(indexCount);

  for (int i = 0; i < mesh->mNumFaces; ++i) {
    indices[i * 3] = mesh->mFaces[i].mIndices[0];
    indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
    indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
  }

  modelConfig.configureIndices(
    indexCount, VK_INDEX_TYPE_UINT32,
    {(uint8_t *)indices, sizeof(uint32_t) * indexCount});
  
  /* TODO: Rest */
  ModelHandle handle = mModels.size++;
  Model &model = mModels[handle];
  model.init(modelConfig, context);

  return handle;
}

Model &ModelManager::getModel(ModelHandle modelHandle) {
  assert(modelHandle != MODEL_HANDLE_INVALID);
  return mModels[modelHandle];
}

const Model &ModelManager::getModel(ModelHandle modelHandle) const {
  assert(modelHandle != MODEL_HANDLE_INVALID);
  return mModels[modelHandle];
}

}
