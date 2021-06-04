#ifndef LIGHTING_DEF_GLSL
#define LIGHTING_DEF_GLSL

struct LightingProperties {
  vec3 sunDirection;
  vec3 sunSize;
  vec3 white;
  float exposure;
};

#endif
