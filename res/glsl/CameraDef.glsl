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
  vec3 wUp;
  float fov;
  float aspectRatio;
  float near;
  float far;

  /* 
     For ocean planar reflections
     1.0 = everything under the planet gets discarded,
     -1.0 = everything above the planet gets discarded
  */
  float clipUnderPlanet;
  float clippingRadius;
};

#endif
