#version 450

#include "sky_utils.glsl"

layout (location = 0) out vec3 outDeltaRayleigh;
layout (location = 1) out vec3 outDeltaMie;
layout (location = 2) out vec4 outScattering;
layout (location = 3) out vec3 outSingleMieScattering;

layout (set = 0, binding = 0) uniform SkyUniform {
  SkyProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittanceTexture;

layout(push_constant) uniform PushConstant {
  int layer;
} uPushConstant;

void main() {
  computeSingleScatteringTexture(
    uSky.sky, uTransmittanceTexture,
    vec3(gl_FragCoord.xy, uPushConstant.layer + 0.5),
    outDeltaRayleigh, outDeltaMie);

  outScattering = vec4(outDeltaRayleigh.rgb, outDeltaMie.r);

  outSingleMieScattering = outDeltaMie;
}
