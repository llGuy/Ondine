#pragma once

#include <glm/glm.hpp>

namespace Yona {

struct CameraProperties {
  glm::mat4 mProjection;
  glm::mat4 mView;
  glm::mat4 mInverseProjection;
  glm::mat4 mInverseView;
  glm::mat4 mViewProjection;
  alignas(16) glm::vec3 mWPosition;
  alignas(16) glm::vec3 mWViewDirection;
  float mFOV;
  float mAspectRatio;
  float mNear;
  float mFar;
};

class WorldCamera {
public:

private:
  CameraProperties mProperties;
};

}
