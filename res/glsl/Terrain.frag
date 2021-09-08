#version 450

#include "Utils.glsl"
#include "Clipping.glsl"
#include "CameraDef.glsl"
#include "PlanetDef.glsl"

layout (location = 0) in VS_DATA {
  // World space
  vec4 wPosition;
  vec4 wNormal;
} inFS;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

layout (set = 1, binding = 0) uniform PlanetUniform {
  PlanetProperties planet;
} uPlanet;

layout (set = 2, binding = 0) uniform ClippingUniform {
  Clipping clipping;
} uClipping;

const float ROUGHNESS = 0.8;
const float METALNESS = 0.0;

void main() {
  // Get distance from center of planet
  vec3 diff = inFS.wPosition.xyz / 1000.0 - uPlanet.planet.wPlanetCenter;
  float radius2 = dot(diff, diff);
  float radius2Diff =
    radius2 - uClipping.clipping.clippingRadius *
    uClipping.clipping.clippingRadius;

  if (radius2Diff * uClipping.clipping.clipUnderPlanet < 0.0) {
    discard;
  }
  else {
    outAlbedo = vec4(0.05, 1.3, 0.1, 0.0) * 0.3;
    outNormal = vec4(nanSafeNormalize(inFS.wNormal.xyz), ROUGHNESS);
    outPosition = inFS.wPosition;
    outPosition.a = 1.0 + METALNESS;
  }
}
