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

layout (set = 5, binding = 0) uniform sampler2D uReflectionTexture;

struct GBufferData {
  vec4 wNormal;
  float depth;
  vec4 wPosition;
  vec4 albedo;
};

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
  }

  return intersection;
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
    viewRay, uSky.sky.wPlanetCenter, uSky.sky.bottomRadius + 0.1);

  GBufferData oceanGBuffer = GBufferData(
    vec4(oceanIntersection.wNormal, 1.0),
    0.0,
    vec4(oceanIntersection.wIntersectionPoint, 1.0),
    vec4(0.0));

  float oceanAlpha = 0.0;
  vec3 oceanRadiance = vec3(0.0);

  if (gbuffer.wPosition.a == 1.0) {
    // Check if this point is further away than the ocean
    vec4 vPosition = uCamera.camera.view * vec4(
      gbuffer.wPosition.xyz / 1000.0, 1.0);
    vec4 vOceanPosition = uCamera.camera.view *
      vec4(oceanIntersection.wIntersectionPoint, 1.0);

    if (vPosition.z < vOceanPosition.z && oceanIntersection.didIntersect) {
      pointAlpha = 0.0;
      pointRadiance = vec3(0.0);

      oceanAlpha = 1.0;
      oceanGBuffer.albedo = texture(
        uReflectionTexture, vec2(1.0 - inUVs.x, inUVs.y)) *
        vec4(0.9, 0.9, 1.1, 1.0);

      oceanRadiance = getPointRadiance(oceanGBuffer).rgb;
    }
    else {
      vec4 radiance = getPointRadiance(gbuffer);
      pointRadiance = radiance.rgb;
      pointAlpha = radiance.a;
    }
  }
  else if (oceanIntersection.didIntersect) {
    oceanAlpha = 1.0;

    oceanGBuffer.albedo = texture(
      uReflectionTexture, vec2(1.0 - inUVs.x, inUVs.y)) *
      vec4(0.9, 0.9, 1.1, 1.0);

    oceanRadiance = getPointRadiance(oceanGBuffer).rgb;
  }

  /* Light contribution from sky */
  vec3 transmittance;
  vec3 radiance = getSkyRadiance(
    uSky.sky, uTransmittanceTexture,
    uScatteringTexture, uSingleMieScatteringTexture,
    (uCamera.camera.wPosition.xyz / 1000.0 - uSky.sky.wPlanetCenter),
    viewRay, 0.0, uLighting.lighting.sunDirection,
    transmittance);

  if (dot(viewRay, uLighting.lighting.sunDirection) >
      uLighting.lighting.sunSize.y) {
    radiance = radiance + transmittance * getSolarRadiance(uSky.sky);
  }

  radiance = mix(radiance, pointRadiance, pointAlpha);
  radiance = mix(radiance, oceanRadiance, oceanAlpha);

  vec3 one = vec3(1.0);
  vec3 expValue =
    exp(-radiance / uLighting.lighting.white * uLighting.lighting.exposure);
  vec3 diff = one - expValue;
  vec3 gamma = vec3(1.0 / 2.2);

  outColor.rgb = pow(diff, gamma);

  outColor.a = 1.0;
}
