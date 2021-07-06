#version 450

#include "CameraDef.glsl"

layout (location = 0) in vec3 inData;
layout (location = 0) out float outBrightness;

layout (push_constant) uniform PushConstant {
  mat4 transform;
  float fade;
  float starSize;
} uPushConstant;

layout (set = 0, binding = 0) uniform CameraUniform {
  CameraProperties camera;
} uCamera;

void main() {
  float azimuthAngle = inData.x;
  float zenithAngle = inData.y;
  float brightness = inData.z;

  vec3 pos = vec3(
    sin(zenithAngle) * cos(azimuthAngle),
    sin(azimuthAngle) * sin(zenithAngle),
    cos(zenithAngle)
  ) * 1000.0;

  outBrightness = brightness * uPushConstant.fade;
  vec4 viewPos = uCamera.camera.view * uPushConstant.transform * vec4(pos, 0.0);

  gl_Position = uCamera.camera.projection * vec4(viewPos.xyz, 1.0);
  gl_Position.y *= -1.0;
  gl_PointSize = uPushConstant.starSize;
}
