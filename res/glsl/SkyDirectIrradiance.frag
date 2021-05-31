#version 450

#include "Sky.glsl"

layout (location = 0) out vec3 outDeltaIrradiance;
layout (location = 1) out vec3 outIrradiance;

layout (set = 0, binding = 0) uniform PlanetUniform {
  PlanetProperties sky;
} uSky;

layout (set = 1, binding = 0) uniform sampler2D uTransmittanceTexture;

void main() {
  outDeltaIrradiance = computeDirectIrradianceTexture(
    uSky.sky, uTransmittanceTexture, gl_FragCoord.xy);

  outIrradiance = vec3(0.0);
}
