#version 450

#include "Sky.glsl"
#include "Utils.glsl"
#include "CameraDef.glsl"
#include "LightingDef.glsl"

layout (location = 0) in vec3 inViewRay;
layout (location = 1) in vec2 inUVs;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uAlbedo;
layout (set = 0, binding = 1) uniform sampler2D uNormal;
layout (set = 0, binding = 2) uniform sampler2D uDepth;

layout (set = 1, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

layout (set = 2, binding = 0) uniform PlanetUniform {
  PlanetProperties sky;
} uSky;

layout (set = 3, binding = 0) uniform LightingUniform {
  LightingProperties lighting;
} uLighting;

layout (set = 4, binding = 0) uniform sampler2D uTransmittanceTexture;
layout (set = 4, binding = 1) uniform sampler3D uScatteringTexture;
layout (set = 4, binding = 2) uniform sampler3D uSingleMieScatteringTexture;
layout (set = 4, binding = 3) uniform sampler2D uIrradianceTexture;

void main() {
  /* Get all the inputs */
  vec3 wNormal = texture(uNormal, inUVs).xyz;
  float depth = texture(uDepth, inUVs).r;
  vec3 cPosition = vec3(inUVs * 2.0 - 1.0, depth);
  vec3 vPosition = getVPositionFromDepth(
    cPosition.xy, cPosition.z, uCamera.camera.inverseProjection);
  vec3 wPosition = vec3(uCamera.camera.inverseView * vec4(vPosition, 1.0));
  vec3 albedo = texture(uAlbedo, inUVs).rgb;
  vec3 viewRay = normalize(inViewRay);

  vec3 transmittance;
  vec3 radiance = getSkyRadiance(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    uCamera.camera.wPosition - uSky.sky.wPlanetCenter, inViewRay, 0.0,
    uLighting.lighting.sunDirection, transmittance);

  if (dot(viewRay, uLighting.lighting.sunDirection) >
      uLighting.lighting.sunSize.y) {
    radiance = radiance + transmittance * getSolarRadiance(uSky.sky);
  }

  outColor.rgb = pow(
    vec3(1.0) -
      exp(-radiance / uLighting.lighting.white * uLighting.lighting.exposure),
    vec3(1.0 / 2.2));
  outColor.a = 1.0;

  outColor = vec4(viewRay, 1.0);
}
