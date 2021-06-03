#version 450

#include "Sky.glsl"
#include "Utils.glsl"
#include "CameraDef.glsl"

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uAlbedo;
layout (set = 0, binding = 1) uniform sampler2D uNormal;
layout (set = 0, binding = 2) uniform sampler2D uDepth;

layout (set = 1, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

layout(set = 2, binding = 0) uniform PlanetUniform {
  PlanetProperties sky;
} uSky;

layout (set = 3, binding = 0) uniform sampler2D uTransmittanceTexture;
layout (set = 3, binding = 1) uniform sampler3D uScatteringTexture;
layout (set = 3, binding = 2) uniform sampler3D uSingleMieScatteringTexture;
layout (set = 3, binding = 3) uniform sampler2D uIrradianceTexture;

void main() {
  /* Get all the inputs */
  vec3 wNormal = texture(uNormal, inUVs).xyz;
  vec3 cPosition = vec3(inUVs * 2.0 - 1.0, texture(uDepth, inUVs).r);
  vec3 vPosition = getVPositionFromDepth(
    cPosition.xy, cPosition.z, uCamera.camera.inverseProjection);
  vec3 wPosition = vec3(uCamera.camera.inverseView * vec4(vPosition, 1.0));
  vec3 albedo = texture(uAlbedo, inUVs).rgb;

  /* Calculate atmosphere contribution to light */
  vec3 skyIrradiance;
  vec3 sunIrradiance = getSunAndSkyIrradiance(
    uSky.sky, uTransmittanceTexture,
    uIrradianceTexture, wPosition, wNormal, vec3(0), skyIrradiance);

  outColor = vec4(albedo, 1.0);
}
