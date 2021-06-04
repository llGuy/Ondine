#version 450

#include "Utils.glsl"
#include "PlanetDef.glsl"
#include "CameraDef.glsl"

layout (location = 0) in vec3 inViewRay;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

layout (set = 1, binding = 0) uniform PlanetUniform {
  PlanetProperties planet;
} uPlanet;

void main() {
  vec3 viewDirection = normalize(inViewRay);

  vec3 p = uCamera.camera.wPosition - uPlanet.planet.wPlanetCenter;
  float pDotV = dot(p, viewDirection);
  float pDotP = dot(p, p);

  float rayPlanetCenterDist2 = pDotP - pDotV * pDotV;

  float distToIntersection = -pDotV - sqrt(
    uPlanet.planet.wPlanetCenter.y * uPlanet.planet.wPlanetCenter.y -
    rayPlanetCenterDist2);

  // Ray is in front of us
  if (distToIntersection > 0.0) {
    vec3 point = uCamera.camera.wPosition + viewDirection * distToIntersection;
    vec3 normal = normalize(point - uPlanet.planet.wPlanetCenter);

    outAlbedo = vec4(uPlanet.planet.groundAlbedo, 1.0);
    outNormal = vec4(normal, 1.0);
    outPosition = vec4(point, 1.0);

    // Set fragment depth
    gl_FragDepth = calculateFragDepth(
      uCamera.camera.far, uCamera.camera.near,
      uCamera.camera.viewProjection,
      point);
  }
  else {
    discard;
  }
}
