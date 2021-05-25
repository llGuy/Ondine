#version 450

// #include "sky_def.glsl"
// #include "sky_utils.glsl"
#include "utils4.glsl"

layout (location = 0) out vec3 outDeltaIrradiance;
layout (location = 1) out vec3 outIrradiance;

layout (set = 0, binding = 0) uniform SkyUniform {
  // SkyProperties sky;
  int a;
} uSky;

layout (set = 1, binding = 0) uniform sampler3D uSingleRayleighScattering;
layout (set = 2, binding = 0) uniform sampler3D uSingleMieScattering;
layout (set = 3, binding = 0) uniform sampler3D uMultipleScattering;

layout (push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

void main() {
  outDeltaIrradiance = ComputeIndirectIrradianceTexture(
    ATMOSPHERE, uSingleRayleighScattering, uSingleMieScattering,
    uMultipleScattering, gl_FragCoord.xy, uPushConstant.scatteringOrder);

  outIrradiance = outDeltaIrradiance;
}
