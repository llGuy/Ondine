#version 450

#include "Sky.glsl"

layout(location = 0) out vec3 outTransmittance;

layout(set = 0, binding = 0) uniform PlanetUniform {
  PlanetProperties sky;
} uSky;

void main() {
  outTransmittance = computeTransmittanceToSkyBoundaryTexture(
    uSky.sky, gl_FragCoord.xy);
}
