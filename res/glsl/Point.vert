#version 450

#include "CameraDef.glsl"

layout (location = 0) in vec2 inPosition;

layout (push_constant) uniform PushConstant {
  vec4 color;
} uPushConstant;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

void main() {
  gl_Position.xy = inPosition;
  gl_Position.w = 1.0;

  gl_Position.y *= -1.0;
  gl_PointSize = 1.0;
}
