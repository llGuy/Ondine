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
layout (set = 0, binding = 2) uniform sampler2D uPosition;
layout (set = 0, binding = 3) uniform sampler2D uDepth;

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
  vec4 wNormal = texture(uNormal, inUVs);
  float depth = texture(uDepth, inUVs).r;
  vec4 wPosition = texture(uPosition, inUVs);
  vec4 albedo = texture(uAlbedo, inUVs).rgba;
  vec3 viewRay = normalize(inViewRay);

  /* Light contribution from the surface */
  float pointAlpha = 0.0;
  vec3 pointRadiance = vec3(0.0);
  if (wPosition.a == 1.0) {
    vec3 skyIrradiance;

    /* Radiance that the surface will reflect */
    vec3 sunIrradiance = getSunAndSkyIrradiance(
      uSky.sky, uTransmittanceTexture, uIrradianceTexture,
      wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter,
      wNormal.xyz, uLighting.lighting.sunDirection, skyIrradiance);

    // TODO
    /*
    pointRadiance = albedo * (1.0 / PI) * (
      sunIrradiance * getSunVisibility(
        wPosition.xyz / 1000.0, uLighting.lighting.sunDirection) +
      skyIrradiance * getSkyVisibility(
        wPosition.xyz / 1000.0));
        */

    /* How much is scattered towards us */
    vec3 transmittance;
    vec3 inScatter = getSkyRadianceToPoint(
      uSky.sky, uTransmittanceTexture,
      uScatteringTexture, uSingleMieScatteringTexture,
      uCamera.camera.wPosition / 1000.0 - uSky.sky.wPlanetCenter,
      wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter, 0.0,
      uLighting.lighting.sunDirection,
      transmittance);

    pointRadiance = pointRadiance * transmittance + inScatter;
    pointAlpha = 1.0;
  }

  /* Light contribution from sky */
  vec3 transmittance;
  vec3 radiance = getSkyRadiance(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    (uCamera.camera.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter),
    viewRay, 0.0, uLighting.lighting.sunDirection, transmittance);

  if (dot(viewRay, uLighting.lighting.sunDirection) >
      uLighting.lighting.sunSize.y) {
    radiance = radiance + transmittance * getSolarRadiance(uSky.sky);
  }

  radiance = mix(radiance, pointRadiance, pointAlpha);

  outColor.rgb = pow(
    vec3(1.0) -
      exp(-radiance / uLighting.lighting.white * uLighting.lighting.exposure),
    vec3(1.0 / 2.2));
  outColor.a = 1.0;
}
