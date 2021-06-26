#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uTexture;

layout (push_constant) uniform PushConstant {
  float pixelationStrength;
  float width;
  float height;
} uPushConstant;

void main() {
  float dx = uPushConstant.pixelationStrength * (1.0 / uPushConstant.width);
  float dy = uPushConstant.pixelationStrength * (1.0 / uPushConstant.height);
  vec2 uvs = vec2(dx * floor(inUVs.x / dx), dy * floor(inUVs.y / dy));
  outColor = texture(uTexture, uvs);
}
