#version 450

#include "sky_def.glsl"
#include "sky_utils.glsl"

/* These are all texture 3D */
layout (location = 0) out vec4 outDeltaRayleigh;
layout (location = 1) out vec4 outDeltaMie;
layout (location = 2) out vec4 outScattering;

layout (set = 0, binding = 0) uniform SkyUniform {
  SkyProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittanceTexture;

layout (push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

void main() {
  computeSingleScatteringTexture(
    uSky.sky, uTransmittanceTexture,
    vec3(gl_FragCoord.xy, uPushConstant.layer + 0.5),
    outDeltaRayleigh.rgb, outDeltaMie.rgb);

  outDeltaRayleigh.a = 1.0;
  outDeltaMie.a = 1.0;

  outScattering = vec4(outDeltaRayleigh.rgb, outDeltaMie.r);

}
