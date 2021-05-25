#version 450

#include "sky_def.glsl"
#include "sky_utils.glsl"

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform SkyUniform {
  SkyProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittance;
layout (set = 2, binding = 0) uniform sampler3D uScatteringTexture;
layout (set = 3, binding = 0) uniform sampler2D uIrradiance;
layout (set = 4, binding = 0) uniform sampler3D uSingleRayleighScattering;
layout (set = 5, binding = 0) uniform sampler3D uSingleMieScattering;
layout (set = 6, binding = 0) uniform sampler2D uDeltaIrradiance;
layout (set = 7, binding = 0) uniform sampler3D uScatteringDensity;
layout (set = 8, binding = 0) uniform sampler3D uDeltaMultipleScattering;

void main() {
  float a = texture(uTransmittance, vec2(0.0)).r;
  float b = texture(uSingleRayleighScattering, vec3(0.0)).g;
  float c = texture(uSingleMieScattering, vec3(0.0)).b;
  float d = texture(uDeltaMultipleScattering, vec3(0.0)).a;
  float e = texture(uDeltaIrradiance, vec2(0.0)).a;
  float f = texture(uScatteringTexture, vec3(0.0)).a;
  float g = texture(uDeltaIrradiance, vec2(0.0)).a;
  float h = texture(uScatteringDensity, vec3(0.0)).a;
  float i = texture(uIrradiance, vec2(0.0)).a;

  float j = uSky.sky.miePhaseFunctionG;

  outColor = vec4(1.0, 0.0, 0.0, 0.0);
}
