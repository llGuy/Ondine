#version 450

#include "CameraDef.glsl"

layout (location = 0) out vec3 outViewRay;
layout (location = 1) out vec2 outUVs;

layout (set = 1, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

const vec4 POSITIONS[] = vec4[](
  vec4(-1.0, -1.0, 0.0, 1.0),
  vec4(-1.0, 1.0, 0.0, 1.0),
  vec4(1.0, -1.0, 0.0, 1.0),
  vec4(1.0, 1.0, 0.0, 1.0)
);

void main() {
  vec4 vertex = POSITIONS[gl_VertexIndex];
  gl_Position = vertex;

  // Vulkan FLIP
  vertex.y *= -1.0;

  outViewRay =
    (uCamera.camera.inverseView *
     vec4((uCamera.camera.inverseProjection *
     vertex).xyz, 0.0)).xyz;

  outUVs = POSITIONS[gl_VertexIndex].xy / 2.0 + 0.5;
}
