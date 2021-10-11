#version 450

#include "CameraDef.glsl"

layout (push_constant) uniform PushConstant {
  vec4 position;
  vec4 color;
  float fade;
  float starSize;
} uPushConstant;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

void main() {
  gl_Position.xy = uPushConstant.position.xy;
  gl_Position.w = 1.0;

  gl_Position.y *= -1.0;
  gl_PointSize = uPushConstant.starSize;
}
