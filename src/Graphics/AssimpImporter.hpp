#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Ondine::Graphics {

/* This is temporary - wait until we implement custom mesh format */
extern Assimp::Importer *gAssimpImporter;
const aiScene *importScene(const char *path);

inline glm::mat4 aiToGlmMat4(const aiMatrix4x4 &matrix) {
  return glm::transpose(glm::make_mat4(&matrix.a1));
}

inline glm::vec3 aiToGlmVec3(const aiVector3D &vector) {
  return glm::vec3(vector.x, vector.y, vector.z);
}

inline glm::quat aiToGlmQuat(const aiQuaternion &quaternion) {
  return glm::quat(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
}

}
