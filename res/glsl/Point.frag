#version 450

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

layout (push_constant) uniform PushConstant {
  vec4 color;
} uPushConstant;

void main() {
  outAlbedo = vec4(uPushConstant.color);
  outNormal = vec4(-1.0);
  outPosition = vec4(0.0);
}
