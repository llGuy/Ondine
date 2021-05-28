#version 450

#include "sky_def.glsl"
#include "sky.glsl"

layout (location = 0) out vec3 outDeltaIrradiance;
layout (location = 1) out vec3 outIrradiance;

layout (set = 0, binding = 0) uniform SkyUniform {
  SkyProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittanceTexture;

void main() {
  outDeltaIrradiance = computeDirectIrradianceTexture(
    uSky.sky, uTransmittanceTexture, gl_FragCoord.xy);

  outIrradiance = vec3(0.0);
}
