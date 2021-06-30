#version 450

#include "Sky.glsl"
#include "Utils.glsl"
#include "Lighting.glsl"
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

vec4 getPointRadianceBRDF(
  float roughness, float metal,
  in GBufferData gbuffer) {
  vec3 skyIrradiance, sunIrradiance, pointRadiance;
  { // Calculate sun and sky irradiance which will contribute to the final BRDF
    vec3 point = gbuffer.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter;
    vec3 normal = gbuffer.wNormal.xyz;
    vec3 sunDirection = uLighting.lighting.sunDirection;
    vec3 moonDirection = uLighting.lighting.moonDirection;

    float r = length(point);
    float muSun = dot(point, sunDirection) / r;
    float muMoon = dot(point, moonDirection) / r;

    skyIrradiance = getIrradiance(uSky.sky, uIrradianceTexture, r, muSun) *
      (1.0 + dot(normal, point) / r) * 0.5;

    vec3 moonIrradiance = getIrradiance(uSky.sky, uIrradianceTexture, r, muMoon) *
      (1.0 + dot(normal, point) / r) * 0.5;

    float incidentIntensity = max(dot(normal, sunDirection), 0.0);
    float toonIntensity = toonShadingIncidentIntensity(incidentIntensity);

    vec3 accumulatedRadiance = vec3(0.0);

    accumulatedRadiance += directionalRadianceBRDF(
      gbuffer,
      mix(vec3(0.04), gbuffer.albedo.rgb, metal),
      roughness,
      metal,
      normalize(gbuffer.wPosition.xyz - uCamera.camera.wPosition),
      uSky.sky.solarIrradiance * getTransmittanceToSun(
        uSky.sky, uTransmittanceTexture, r, muSun),
      uLighting.lighting.sunDirection);

    accumulatedRadiance += directionalRadianceBRDF(
      gbuffer,
      mix(vec3(0.04), gbuffer.albedo.rgb, metal),
      roughness,
      metal,
      normalize(gbuffer.wPosition.xyz - uCamera.camera.wPosition),
      uSky.sky.solarIrradiance * getTransmittanceToSun(
        uSky.sky, uTransmittanceTexture, r, muMoon),
      uLighting.lighting.moonDirection) * uLighting.lighting.moonStrength * 5.0;

    pointRadiance = accumulatedRadiance +
      gbuffer.albedo.rgb * (1.0 / PI) * skyIrradiance +
      gbuffer.albedo.rgb * (1.0 / PI) * moonIrradiance *
      uLighting.lighting.moonStrength * 5.0;
  }

  /* How much is scattered towards us */
  vec3 transmittance;
  vec3 inScatter = getSkyRadianceToPoint(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    uCamera.camera.wPosition / 1000.0 - uSky.sky.wPlanetCenter,
    gbuffer.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter, 0.0,
    uLighting.lighting.sunDirection,
    transmittance);

  pointRadiance = pointRadiance * transmittance + inScatter;

  // Moon
  inScatter = getSkyRadianceToPoint(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    uCamera.camera.wPosition / 1000.0 - uSky.sky.wPlanetCenter,
    gbuffer.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter, 0.0,
    uLighting.lighting.moonDirection,
    transmittance) * uLighting.lighting.moonStrength * 5.0;

  pointRadiance = pointRadiance * transmittance + inScatter;

  return vec4(pointRadiance, 1.0);
}

void main() {
  /* Get all the inputs */
  vec4 wNormal = texture(uNormal, inUVs);
  float depth = texture(uDepth, inUVs).r;
  vec4 wPosition = texture(uPosition, inUVs);
  vec4 albedo = texture(uAlbedo, inUVs).rgba;
  vec3 viewRay = normalize(inViewRay);

  GBufferData gbuffer = GBufferData(
    wNormal,
    depth,
    wPosition,
    albedo);

  /* Light contribution from the surface */
  float pointAlpha = 0.0;
  vec3 pointRadiance = vec3(0.0);

  vec3 radianceBaseColor = vec3(0.0);

  if (wPosition.a == 1.0) {
    vec4 radiance = getPointRadianceBRDF(0.8, 0.0, gbuffer);
    pointRadiance = radiance.rgb;
    pointAlpha = radiance.a;

    /*
    vec3 skyIrradiance;

    vec3 sunIrradiance = getSunAndSkyIrradiance(
      uSky.sky, uTransmittanceTexture, uIrradianceTexture,
      wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter,
      wNormal.xyz, uLighting.lighting.sunDirection, skyIrradiance);

    pointRadiance = albedo.rgb * (1.0 / PI) * (sunIrradiance + skyIrradiance);

    vec3 transmittance;
    vec3 inScatter = getSkyRadianceToPoint(
      uSky.sky, uTransmittanceTexture,
      uScatteringTexture, uSingleMieScatteringTexture,
      uCamera.camera.wPosition / 1000.0 - uSky.sky.wPlanetCenter,
      wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter, 0.0,
      uLighting.lighting.sunDirection,
      transmittance);

    viewRay = normalize(wPosition.xyz - uCamera.camera.wPosition.xyz);

    pointRadiance = pointRadiance * transmittance + inScatter;
    pointAlpha = 1.0;
    */
  }
  else {
    radianceBaseColor = albedo.rgb;
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

  radiance += getSkyRadiance(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    (uCamera.camera.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter),
    viewRay, 0.0, uLighting.lighting.moonDirection,
    transmittance) * uLighting.lighting.moonStrength * 5.0;

  // Moon always visible
  if (dot(viewRay, uLighting.lighting.moonDirection) >
      uLighting.lighting.sunSize.y * 0.9999) {
    radiance = radiance + (transmittance * getSolarRadiance(uSky.sky));
  }

  radiance = mix(radiance, pointRadiance, pointAlpha) + radianceBaseColor;

  vec3 one = vec3(1.0);
  vec3 expValue =
    exp(-radiance / uLighting.lighting.white * uLighting.lighting.exposure);
  vec3 diff = one - expValue;
  vec3 gamma = vec3(1.0 / 2.2);

  outColor.rgb = pow(diff, gamma);

  outColor.a = 1.0;
}
