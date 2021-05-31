#ifndef CAMERA_DEF_GLSL
#define CAMERA_DEF_GLSL

struct CameraProperties {
  mat4 projection;
  mat4 view;
  mat4 inverseProjection;
  mat4 inverseView;
  mat4 viewProjection;
  vec3 wPosition;
  vec3 wViewDirection;
  float fov;
  float aspectRatio;
  float near;
  float far;
};

#endif
