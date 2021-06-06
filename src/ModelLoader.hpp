#pragma once

#include <assimp/Importer.hpp>

namespace Ondine::Graphics {

class ModelLoader {
public:
  void init();

private:
  Assimp::Importer mImporter;
};

}
