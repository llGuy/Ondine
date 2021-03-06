#version 450

#include "CameraDef.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
// Add normals in future

layout (location = 0) out VS_DATA {
  vec4 wPosition;
  vec4 wNormal;
} outVS;

layout (push_constant) uniform PushConstant {
  mat4 modelMatrix;
} uPushConstant;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

void main() {
  outVS.wPosition = uPushConstant.modelMatrix * vec4(inPosition, 1.0);
  outVS.wNormal = uPushConstant.modelMatrix * vec4(inNormal, 0.0);

  gl_Position = uCamera.camera.viewProjection * outVS.wPosition;

  gl_Position.y *= -1.0;
}
