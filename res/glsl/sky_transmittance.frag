#version 450

// #include "sky_def.glsl"
// #include "sky_utils.glsl"
#include "utils4.glsl"

layout(location = 0) out vec3 outTransmittance;

layout(set = 0, binding = 0) uniform SkyUniform {
  // SkyProperties sky;
  int a;
} uSky;

void main() {
  outTransmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(
    ATMOSPHERE, gl_FragCoord.xy);
}
