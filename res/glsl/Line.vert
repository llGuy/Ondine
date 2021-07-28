#version 450

#include "CameraDef.glsl"

layout (push_constant) uniform PushConstant {
  vec4 translateAndScale;
  vec4 positions[2];
  vec4 color;
} uPushConstant;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

void main() {
  vec3 pos = uPushConstant.translateAndScale.xyz +
    uPushConstant.positions[gl_VertexIndex].xyz;
  vec4 wPosition = vec4(pos * uPushConstant.translateAndScale.w, 1.0);

  gl_Position = uCamera.camera.viewProjection * wPosition;
  gl_Position.y *= -1.0;
}
