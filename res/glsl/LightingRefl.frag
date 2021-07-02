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

layout (set = 5, binding = 0) uniform sampler2D uReflectionTexture;
layout (set = 6, binding = 0) uniform sampler2D uWaterNormalMapTexture;
layout (set = 6, binding = 1) uniform sampler2D uWaterDistortionTexture;

layout (set = 7, binding = 0) uniform sampler2D uBRDFLutTexture;

vec3 accumulateSunAndNightRadianceBRDF(
  in GBufferData gbuffer,
  float roughness, float metal,
  float r, float muSun, float muMoon) {
  vec3 ret = directionalRadianceBRDF(
    gbuffer,
    mix(vec3(0.04), gbuffer.albedo.rgb, metal),
    roughness,
    metal,
    normalize(gbuffer.wPosition.xyz - uCamera.camera.wPosition),
    uSky.sky.solarIrradiance * getTransmittanceToSun(
      uSky.sky, uTransmittanceTexture, r, muSun),
    uLighting.lighting.sunDirection);

  ret += directionalRadianceBRDF(
    gbuffer,
    mix(vec3(0.04), gbuffer.albedo.rgb, metal),
    roughness,
    metal,
    normalize(gbuffer.wPosition.xyz - uCamera.camera.wPosition),
    uSky.sky.solarIrradiance * getTransmittanceToSun(
      uSky.sky, uTransmittanceTexture, r, muMoon),
    uLighting.lighting.moonDirection) * uLighting.lighting.moonStrength * 8.0;

  return ret;
}

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

    vec3 moonIrradiance =
      getIrradiance(uSky.sky, uIrradianceTexture, r, muMoon) *
      (1.0 + dot(normal, point) / r) * 0.5;

    float incidentIntensity = max(dot(normal, sunDirection), 0.0);
    float toonIntensity = toonShadingIncidentIntensity(incidentIntensity);

    vec3 accumulatedRadiance = accumulateSunAndNightRadianceBRDF(
      gbuffer, roughness, metal, r, muSun, muMoon);

    pointRadiance = accumulatedRadiance +
      gbuffer.albedo.rgb * (1.0 / PI) * skyIrradiance +
      gbuffer.albedo.rgb * (1.0 / PI) * moonIrradiance *
      uLighting.lighting.moonStrength * 8.0;
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

  return vec4(pointRadiance, 1.0);
}

vec4 getReflectivePointRadiancePseudoBRDF(
  float roughness, float metal,
  in vec3 reflectedColor,
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

    vec3 moonIrradiance =
      getIrradiance(uSky.sky, uIrradianceTexture, r, muMoon) *
      (1.0 + dot(normal, point) / r) * 0.5;

    float incidentIntensity = max(dot(normal, sunDirection), 0.0);
    float toonIntensity = toonShadingIncidentIntensity(incidentIntensity);

    vec3 accumulatedRadiance = accumulateSunAndNightRadianceBRDF(
      gbuffer, roughness, metal, r, muSun, muMoon);

    pointRadiance = accumulatedRadiance +
      gbuffer.albedo.rgb * (1.0 / PI) * skyIrradiance +
      gbuffer.albedo.rgb * (1.0 / PI) * moonIrradiance *
      uLighting.lighting.moonStrength * 8.0;
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

  return vec4(pointRadiance, 1.0);
}

vec4 getPointRadiance(in GBufferData gbuffer) {
  vec3 skyIrradiance;

  /* Radiance that the surface will reflect */
  vec3 sunIrradiance = getSunAndSkyIrradiance(
    uSky.sky, uTransmittanceTexture, uIrradianceTexture,
    gbuffer.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter,
    gbuffer.wNormal.xyz, uLighting.lighting.sunDirection, skyIrradiance);

  vec3 pointRadiance = gbuffer.albedo.rgb * (1.0 / PI) *
    (sunIrradiance + skyIrradiance);

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

  return vec4(pointRadiance, 1.0);
}

vec4 getPointRadianceToon(in GBufferData gbuffer) {
  vec3 skyIrradiance;

  /* Radiance that the surface will reflect */
  vec3 sunIrradiance = getSunAndSkyIrradianceToon(
    uSky.sky, uTransmittanceTexture, uIrradianceTexture,
    gbuffer.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter,
    gbuffer.wNormal.xyz, uLighting.lighting.sunDirection, skyIrradiance);

  vec3 pointRadiance = gbuffer.albedo.rgb * (1.0 / PI) *
    (sunIrradiance + skyIrradiance);

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

  return vec4(pointRadiance, 1.0);
}

struct RayIntersection {
  bool didIntersect;
  vec3 wIntersectionPoint;
  vec3 wNormal;
};

RayIntersection raySphereIntersection(
  in vec3 viewRay,
  in vec3 sphereCenter,
  in float sphereRadius) {
  vec3 planetCenterKm = sphereCenter;
  vec3 camPosKm = uCamera.camera.wPosition / 1000.0;

  vec3 p = (camPosKm - planetCenterKm);
  float pDotV = dot(p, viewRay);
  float pDotP = dot(p, p);

  float rayPlanetCenterDist2 = pDotP - pDotV * pDotV;

  float distToIntersection = -pDotV - sqrt(
    sphereRadius * sphereRadius -
    rayPlanetCenterDist2);

  RayIntersection intersection = RayIntersection(false, vec3(0.0), vec3(0.0));

  // Ray is in front of us
  if (distToIntersection > 0.0) {
    intersection.didIntersect = true;
    intersection.wIntersectionPoint = camPosKm + viewRay * distToIntersection;
    intersection.wNormal = normalize(
      intersection.wIntersectionPoint - planetCenterKm);

    vec3 intersectionM = intersection.wIntersectionPoint;

    // Depends on depth of the water on screen
    float displacementFactor = 1.0 - smoothstep(
      0.0, 1.0, distToIntersection / 0.5);
  }

  return intersection;
}

const float OCEAN_HEIGHT = 0.05;
const float OCEAN_RADIANCE_FACTOR = 0.015;

vec4 getOceanColor(
  in vec3 position,
  in vec3 normal) {
  vec2 distortion = vec2(normal.x, normal.z) * 0.1;

  vec2 coords = vec2(1.0 - inUVs.x, inUVs.y) + distortion;
  coords = clamp(coords, vec2(0.0001), vec2(0.9999));

  // return texture(uReflectionTexture, coords) * OCEAN_RADIANCE_FACTOR *
  return vec4(uLighting.lighting.waterSurfaceColor, 1.0);
}

vec3 getOceanNormal(in vec3 pointPosition) {
  vec2 displacement = vec2(uLighting.lighting.continuous) * 0.3;

  vec3 normal = readNormalFromMap(
    vec2(pointPosition.x, pointPosition.z) * 0.01 + displacement,
    uWaterNormalMapTexture);

  normal += readNormalFromMap(
    vec2(pointPosition.x, -pointPosition.z) * 0.005 + displacement,
    uWaterNormalMapTexture);

  float distToPoint = length(pointPosition - uCamera.camera.wPosition);
  distToPoint = clamp(distToPoint / 550.0, 0.0, 1.0);

  float progress = dot(
    vec3(0.0, 1.0, 0.0),
    normalize(pointPosition - uCamera.camera.wPosition));

  progress = pow(progress, 1.0);
  
  return mix(
    normalize(normal * vec3(0.2, 1.0, 0.2)),
    vec3(0.0, 1.0, 0.0),
    1.0 - progress);
}

void main() {
  /* Get all the inputs */
  GBufferData gbuffer = GBufferData(
    texture(uNormal, inUVs),
    texture(uDepth, inUVs).r,
    texture(uPosition, inUVs),
    texture(uAlbedo, inUVs));

  vec3 viewRay = normalize(inViewRay);

  /* Light contribution from the surface */
  float pointAlpha = 0.0;
  vec3 pointRadiance = vec3(0.0);

  RayIntersection oceanIntersection = raySphereIntersection(
    viewRay, uSky.sky.wPlanetCenter, uSky.sky.bottomRadius + OCEAN_HEIGHT);

  GBufferData oceanGBuffer = GBufferData(
    vec4(oceanIntersection.wNormal, 1.0),
    0.0,
    vec4(oceanIntersection.wIntersectionPoint, 1.0),
    vec4(0.0));

  float oceanAlpha = 0.0;
  vec3 oceanRadiance = vec3(0.0);

  vec3 radianceBaseColor = vec3(0.0);

  if (gbuffer.wPosition.a == 1.0) {
    // Check if this point is further away than the ocean
    vec4 vPosition = uCamera.camera.view * vec4(
      gbuffer.wPosition.xyz / 1000.0, 1.0);
    vec4 vOceanPosition = uCamera.camera.view * vec4(
      oceanIntersection.wIntersectionPoint, 1.0);

    if (vPosition.z < vOceanPosition.z && oceanIntersection.didIntersect) {
      // This is the ocean
      oceanGBuffer.wPosition *= 1000.0;

      pointAlpha = 0.0;
      pointRadiance = vec3(0.0);

      oceanAlpha = 1.0;
      oceanGBuffer.wNormal =
        vec4(getOceanNormal(oceanGBuffer.wPosition.xyz), 1.0);
      oceanGBuffer.albedo = getOceanColor(
        oceanGBuffer.wPosition.xyz,
        oceanGBuffer.wNormal.xyz);

      oceanRadiance = getReflectivePointRadiancePseudoBRDF(
        0.3, 0.0, vec3(0.0), oceanGBuffer).rgb;
    }
    else {
      // This is a rendered object
      vec4 radiance = getPointRadianceBRDF(0.8, 0.0, gbuffer);
      pointRadiance = radiance.rgb;
      pointAlpha = radiance.a;
    }
  }
  else if (oceanIntersection.didIntersect) {
    oceanGBuffer.wPosition *= 1000.0;

    oceanAlpha = 1.0;

    oceanGBuffer.wNormal =
      vec4(getOceanNormal(oceanGBuffer.wPosition.xyz), 1.0);
    oceanGBuffer.albedo = getOceanColor(
      oceanGBuffer.wPosition.xyz,
      oceanGBuffer.wNormal.xyz);

    oceanRadiance = getReflectivePointRadiancePseudoBRDF(
      0.3, 0.0, vec3(0.0), oceanGBuffer).rgb;
  }
  else {
    radianceBaseColor = gbuffer.albedo.rgb;
  }

  /* Light contribution from sky */
  vec3 transmittance;
  vec3 radiance = getSkyRadiance(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    (uCamera.camera.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter),
    viewRay, 0.0, uLighting.lighting.sunDirection,
    transmittance) + radianceBaseColor;

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

  if (dot(viewRay, uLighting.lighting.moonDirection) >
      uLighting.lighting.sunSize.y * 0.9999) {
    radiance = radiance + (transmittance * getSolarRadiance(uSky.sky));
  }

  radiance = mix(radiance, pointRadiance, pointAlpha);
  radiance = mix(radiance, oceanRadiance, oceanAlpha);

  vec3 one = vec3(1.0);
  vec3 expValue =
    exp(-radiance / uLighting.lighting.white * uLighting.lighting.exposure);
  vec3 diff = one - expValue;
  vec3 gamma = vec3(1.0 / 2.2);

  outColor.rgb = pow(diff, gamma);

  outColor.a = texture(uBRDFLutTexture, vec2(0.0)).r;
}
