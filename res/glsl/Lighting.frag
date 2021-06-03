#version 450

#include "Sky.glsl"
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

void main() {
  outColor = texture(uAlbedo, inUVs);
}
