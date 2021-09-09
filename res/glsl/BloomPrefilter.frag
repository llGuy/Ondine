#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uTexture;

#if 0
layout (push_constant) uniform PushConstant {
} uPushConstant;
#endif

void main() {
  outColor = texture(uTexture, inUVs);
}
