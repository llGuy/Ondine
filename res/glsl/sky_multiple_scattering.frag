#version 450

#include "sky_def.glsl"
#include "sky.glsl"

layout(location = 0) out vec3 outDeltaMultipleScattering;
layout(location = 1) out vec4 outScattering;

layout (set = 0, binding = 0) uniform SkyUniform {
  SkyProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittance;
layout (set = 2, binding = 0) uniform sampler3D uScatteringDensity;

layout (push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

void main() {
  float nu;
  outDeltaMultipleScattering = computeMultipleScatteringTexture(
    uSky.sky, uTransmittance, uScatteringDensity,
    vec3(gl_FragCoord.xy, uPushConstant.layer + 0.5),
    nu);

  outScattering = vec4(
    outDeltaMultipleScattering.rgb / rayleighPhase(nu),
    0.0);
}
