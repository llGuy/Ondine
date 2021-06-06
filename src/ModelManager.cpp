#include "FileSystem.hpp"
#include "Application.hpp"
#include "ModelManager.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace Ondine::Graphics {

void ModelManager::init() {
  /* Possibly load models loaded from previous session or something idk */
  mStaticModels.init(MAX_MODEL_COUNT);
}

StaticModelHandle ModelManager::loadStaticModel(
  const char *path, VulkanContext &graphicsContext) {
  Core::File modelFile = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    path, Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer modelData = modelFile.readBinary();

  const aiScene *scene = mImporter.ReadFileFromMemory(
    modelData.data, modelData.size, aiProcess_Triangulate);

  /* We assume just one mesh for now */
  aiMesh *mesh = scene->mMeshes[0];

  ModelConfig modelConfig(mesh->mNumVertices);

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
  StaticModelHandle handle = mStaticModels.size++;
  Model &model = mStaticModels[handle];
  model.init(modelConfig, graphicsContext);

  return handle;
}

Model &ModelManager::getStaticModel(StaticModelHandle modelHandle) {
  assert(modelHandle != MODEL_HANDLE_INVALID);
  return mStaticModels[modelHandle];
}

}
