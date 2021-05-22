#version 450

#include "sky_utils3.glsl"
// #include "sky_def.glsl"

layout(location = 0) out vec3 outTransmittance;

layout(set = 0, binding = 0) uniform SkyUniform {
  AtmosphereParameters sky;
} uSky;

void main() {
  vec2 fragCoordXY = gl_FragCoord.xy;
  fragCoordXY.y = TRANSMITTANCE_TEXTURE_HEIGHT - fragCoordXY.y;
  outTransmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(
    ATMOSPHERE, fragCoordXY);
}
