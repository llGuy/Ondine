#version 450

// #include "sky_def.glsl"
// #include "sky_utils2.glsl"
#include "utils4.glsl"

layout (location = 0) out vec3 outScatteringDensity;

layout (set = 0, binding = 0) uniform SkyUniform {
  // SkyProperties sky;
  int a;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittance;

layout (set = 2, binding = 0) uniform sampler3D uSingleRayleighScattering;
layout (set = 3, binding = 0) uniform sampler3D uSingleMieScattering;
layout (set = 4, binding = 0) uniform sampler3D uMultipleScattering;

layout (set = 5, binding = 0) uniform sampler2D uIrradiance;

layout(push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

void main() {
  outScatteringDensity = ComputeScatteringDensityTexture(
    ATMOSPHERE, uTransmittance, uSingleRayleighScattering, uSingleMieScattering,
    uMultipleScattering, uIrradiance,
    vec3(gl_FragCoord.xy, uPushConstant.layer), uPushConstant.scatteringOrder);
}
