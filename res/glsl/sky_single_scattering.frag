#version 450

#include "sky_utils3.glsl"

/* These are all texture 3D */
layout (location = 0) out vec4 outDeltaRayleigh;
layout (location = 1) out vec4 outDeltaMie;
layout (location = 2) out vec4 outScattering;

layout (set = 0, binding = 0) uniform SkyUniform {
  AtmosphereParameters sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittanceTexture;

layout (push_constant) uniform PushConstant {
  int layer;
} uPushConstant;

void main() {
  vec2 fragCoordXY = gl_FragCoord.xy;
  fragCoordXY.y = SCATTERING_TEXTURE_MU_SIZE - fragCoordXY.y;

  ComputeSingleScatteringTexture(
    uSky.sky, uTransmittanceTexture,
    vec3(fragCoordXY, uPushConstant.layer + 0.5),
    outDeltaRayleigh.rgb, outDeltaMie.rgb);

  outDeltaRayleigh.a = 1.0;
  outDeltaMie.a = 1.0;

  outScattering = vec4(outDeltaRayleigh.rgb, outDeltaMie.r);

}
