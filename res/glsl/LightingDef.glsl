#ifndef LIGHTING_DEF_GLSL
#define LIGHTING_DEF_GLSL

struct LightingProperties {
  vec3 sunDirection;
  vec3 moonDirection;
  vec3 sunSize;
  vec3 white;
  vec3 waterSurfaceColor;
  float exposure;
  float time;
  float dt;
  float moonStrength;
  float continuous;
};

#endif
