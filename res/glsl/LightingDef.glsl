#ifndef LIGHTING_DEF_GLSL
#define LIGHTING_DEF_GLSL

struct WaveProfile {
  float zoom;
  float displacementSpeed;
  float strength;
};

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
  float moonLightingStrength;
  float continuous;
  float waveStrength;
  float waterRoughness;
  float waterMetal;
  WaveProfile waveProfiles[4];
};

#endif
