#version 450

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

layout (push_constant) uniform PushConstant {
  vec4 position;
  vec4 color;
  float fade;
  float starSize;
} uPushConstant;

void main() {
  outAlbedo = vec4(uPushConstant.color * uPushConstant.fade);
  outNormal = vec4(0.0);
  outPosition = vec4(0.0);
}
