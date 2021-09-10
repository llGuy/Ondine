#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uTextureLowRes;
layout (set = 1, binding = 0) uniform sampler2D uTextureHighRes;

layout (push_constant) uniform PushConstant {
  vec4 intensity;
  vec4 scale;
  float threshold;
  bool horizontal;
} uPushConstant;

void main() {
  outColor = texture(uTextureLowRes, inUVs) + texture(uTextureHighRes, inUVs);
}
