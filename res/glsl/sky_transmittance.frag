#version 450

#include "sky_utils.glsl"

layout(location = 0) out vec3 outTransmittance;

layout(set = 0, binding = 0) uniform SkyUniform{
  SkyProperties sky;
} uSky;

void main() {

  outTransmittance = computeTransmittanceToSkyBoundaryTexture(
    uSky.sky, gl_FragCoord.xy);
}
