#include "FileSystem.hpp"
#include "AssimpImporter.hpp"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>

namespace Ondine::Graphics {

Assimp::Importer *gAssimpImporter = nullptr;

const aiScene *importScene(const char *path) {
  Core::File modelFile = Core::gFileSystem->createFile(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    path, Core::FileOpenType::Binary | Core::FileOpenType::In);

  Buffer modelData = modelFile.readBinary();

  return gAssimpImporter->ReadFileFromMemory(
    modelData.data, modelData.size, aiProcess_Triangulate);
}

}
